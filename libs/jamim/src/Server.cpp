#include "Server.hpp"
#include "Message.hpp"
#include <boost/bind.hpp>
#include <functional>
#include <boost/thread.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


using boost::mutex;
#ifndef NDEBUG
static mutex debug_mutex;
#endif /* NDEBUG */

/* ChatRoom */
/* ------------------------------------------------------------------------- */
void ChatRoom::join( ptr_ChatParticipant participant )
{
    participants_.insert( participant );
}

void ChatRoom::leave( ptr_ChatParticipant participant )
{
    participants_.erase( participant );
}

std::size_t ChatRoom::participant_count() const
{
    return participants_.size();
}

void ChatRoom::deliver( const Message& msg, ptr_ChatParticipant sender )
{
    for( auto it = participants_.begin(); it != participants_.end(); ++it ){
        if( *it != sender ){
            (*it)->deliver( msg );
        }
    }
}

void ChatRoom::deliver( const Message& msg )
{
    for( auto participant : participants_ ){
        participant->deliver( msg );
    }
}

void ChatRoom::file_awaiting( const Message& msg, ptr_ChatParticipant sender )
{
    response_awaiters_.insert( sender );
    file_sender_readers_map_.insert( {sender, participants_} );
    sender->file_responses_remaining( participants_.size() );
    deliver( msg, sender );
}

/* Signaled by a ChatParticipant who recieved the expected number of 
 * responses */
void ChatRoom::file_awaiting_complete( ptr_ChatParticipant sender )
{
    response_awaiters_.erase( sender );
    auto it_awaiter = file_sender_readers_map_.find( sender );
    if( it_awaiter != file_sender_readers_map_.end() ){
        if( it_awaiter->second.empty() ){
            file_sender_readers_map_.erase( sender );
        }
    }
}

std::size_t ChatRoom::file_reader_count( ptr_ChatParticipant sender ) const
{
    auto it_awaiter = file_sender_readers_map_.find( sender );
    if( it_awaiter != file_sender_readers_map_.end() ){
        return it_awaiter->second.size();
    }
    else{
        return 0;
    }
}

/* Sent by a ChatParticipant (sender) accepting a file transfer */
void ChatRoom::file_accept( const Message& msg, ptr_ChatParticipant sender )
{
    for( ptr_ChatParticipant p : response_awaiters_ ){
        auto it_awaiter = file_sender_readers_map_.find( p );
        if( it_awaiter != file_sender_readers_map_.end() ){
            it_awaiter->first->file_accepted( msg, sender );
        }
    }
}

/* Sent by a ChatParticipant (sender) refusing a file transfer */
void ChatRoom::file_refuse( const Message& msg, ptr_ChatParticipant sender )
{
    for( ptr_ChatParticipant p : response_awaiters_ ){
        auto it_awaiter = file_sender_readers_map_.find( p );
        if( it_awaiter != file_sender_readers_map_.end() ){
            it_awaiter->second.erase( sender );
            it_awaiter->first->file_refused( msg, sender );
        }
    }
}

void ChatRoom::file_cancel( const Message& msg, ptr_ChatParticipant sender )
{
    file_msg_deliver( msg, sender );
    file_sender_readers_map_.erase( sender );
}

void ChatRoom::file_cancel_all( const Message& msg, ptr_ChatParticipant sender )
{
    file_msg_deliver( msg, sender );
    file_sender_readers_map_.erase( sender );
}

void ChatRoom::file_done( const Message& msg, ptr_ChatParticipant sender )
{
    file_msg_deliver( msg, sender );
    file_sender_readers_map_.erase( sender );
}


void ChatRoom::file_deliver( const std::vector<char>& data
                           , ptr_ChatParticipant sender )
{
    auto it = file_sender_readers_map_.find( sender );
    if( it != file_sender_readers_map_.end() ){
        for( ptr_ChatParticipant reader : it->second ){
            reader->file_deliver( data );
        }
    }
}

void ChatRoom::file_msg_deliver( const Message& msg
                               , ptr_ChatParticipant sender )
{
    auto it = file_sender_readers_map_.find( sender );
    if( it != file_sender_readers_map_.end() ){
        for( ptr_ChatParticipant reader : it->second ){
            reader->file_msg_deliver( msg );
        }
    }
}
/* ------------------------------------------------------------------------- */

/* ChatSession */
/* ------------------------------------------------------------------------- */

/* static variables */
const std::unordered_map<MessageType, ChatSession::Handler>
ChatSession::s_handler_map_{
      { MessageType::EmptyMsg         , &ChatSession::handle_empty }
    , { MessageType::ChatMsg          , &ChatSession::handle_read_body }
    , { MessageType::CmdQuit          , &ChatSession::handle_quit }
    , { MessageType::FileStart        , &ChatSession::handle_file_start }
    , { MessageType::FileAccept       , &ChatSession::handle_file_accept }
    , { MessageType::FileRefuse       , &ChatSession::handle_file_refuse }
    , { MessageType::FileCancel       , &ChatSession::handle_file_cancel }
    , { MessageType::FileCancelAll    , &ChatSession::handle_file_cancel_all }
    , { MessageType::FileDone         , &ChatSession::handle_file_done}
    , { MessageType::Unknown          , &ChatSession::handle_unknown }
};


/* public */

void ChatSession::start()
{
    room_.join( shared_from_this() );
    do_read_header();
}

void ChatSession::deliver( const Message& msg )
{
    io_strand_.post(
        [this,msg]()
        {
            bool write_in_progress = !write_msg_queue_.empty();
            write_msg_queue_.push_back( msg );
            if( !write_in_progress ){
                do_write();
            }
        }
    );
}

/* private */

void ChatSession::do_read_header()
{
    boost::asio::async_read( socket_
        , boost::asio::buffer( read_msg_.data(), MessageHeader::HeaderLength )
        , io_strand_.wrap(
            boost::bind( &ChatSession::handle_read_header, shared_from_this()
                , boost::asio::placeholders::error
                , boost::asio::placeholders::bytes_transferred
                /* , shared_from_this() */ )
        ));
}

void ChatSession::handle_read_header( const boost::system::error_code& ec
                                    , std::size_t /*length*/
                                    /* , ptr_ChatParticipant sender */ )
{
    #ifndef NDEBUG
    {   mutex::scoped_lock lk(debug_mutex);
        std::cout << __FUNCTION__ << ", ec: " << ec << std::endl;
    }
    #endif /* NDEBUG */

    if( !ec ){
        read_msg_.sync();
        do_read_body();
    }
    else{
        handle_error( ec );
    }
}

void ChatSession::do_read_body()
{
    auto it = s_handler_map_.find( read_msg_.msg_type() );
    Handler handler = ( (it!=s_handler_map_.end()) ? it->second 
                                                   : &ChatSession::handle_unknown );
    boost::asio::async_read( socket_
        , boost::asio::buffer( read_msg_.msg_body(), read_msg_.body_length() )
        , io_strand_.wrap(
            boost::bind( handler, shared_from_this()
                , boost::asio::placeholders::error
                , boost::asio::placeholders::bytes_transferred
                /* , shared_from_this() */ )
        ));
}

void ChatSession::handle_read_body( const boost::system::error_code& ec
                                  , std::size_t /*length*/
                                  /* , ptr_ChatParticipant sender */ )
{
    #ifndef NDEBUG
    {   mutex::scoped_lock lk(debug_mutex);
        std::cout << __FUNCTION__ << ", ec: " << ec << std::endl;
    }
    #endif /* NDEBUG */

    if( !ec ){
        room_.deliver( read_msg_, shared_from_this() );
        do_read_header();
    }
    else{
        handle_error( ec );
    }
}


void ChatSession::handle_empty( const boost::system::error_code& ec
                              , std::size_t /*length*/
                              /* , ptr_ChatParticipant sender */ )
{
    #ifndef NDEBUG
    {   mutex::scoped_lock lk(debug_mutex);
        std::cout << __FUNCTION__ << ", ec: " << ec << std::endl;
    }
    #endif /* NDEBUG */

    { mutex::scoped_lock lk(debug_mutex);
        std::cout << "[Server]: Empty message received" << std::endl;
    }
    
    do_read_header();
}

void ChatSession::handle_quit( const boost::system::error_code& ec
                             , std::size_t /*length*/
                             /* , ptr_ChatParticipant sender */ )
{
    #ifndef NDEBUG
    {   mutex::scoped_lock lk(debug_mutex);
        std::cout << __FUNCTION__ << ", ec: " << ec << std::endl;
    }
    #endif /* NDEBUG */

    room_.deliver( message_from_string("[Server] User "
                  + string_id() + " has left the room." ),
                    shared_from_this() );
    room_.leave( shared_from_this() );
}

void ChatSession::handle_unknown( const boost::system::error_code& ec
                                , std::size_t /*length*/
                                /* , ptr_ChatParticipant sender */ )
{
    #ifndef NDEBUG
    {   mutex::scoped_lock lk(debug_mutex);
        std::cout << __FUNCTION__ << ", ec: " << ec << std::endl;
    }
    #endif /* NDEBUG */

    deliver( message_from_string("[Server] Unknown command "
                      + read_msg_.body_to_string() ) );

    do_read_header();
}

void ChatSession::do_write()
{
    #ifndef NDEBUG
    {   mutex::scoped_lock lk(debug_mutex);
        std::cout << __FUNCTION__ << std::endl;
    }
    #endif /* NDEBUG */

    boost::asio::async_write( socket_
        , boost::asio::buffer( write_msg_queue_.front().data()
                             , write_msg_queue_.front().total_length() )
        , io_strand_.wrap(
            boost::bind( &ChatSession::handle_write, shared_from_this()
                , boost::asio::placeholders::error
                , boost::asio::placeholders::bytes_transferred
                /* , shared_from_this() */ )
        ));
}

void ChatSession::handle_write( const boost::system::error_code& ec
                              , std::size_t /*length*/
                              /* , ptr_ChatParticipant sender */ )
{
    #ifndef NDEBUG
    {   mutex::scoped_lock lk(debug_mutex);
        std::cout << __FUNCTION__ << ", ec: " << ec << std::endl;
    }
    #endif /* NDEBUG */

    if( !ec ){
        write_msg_queue_.pop_front();
        if( !write_msg_queue_.empty() ){
            do_write();
        }
    }
    else{
        handle_error( ec );
    }
}

void ChatSession::handle_error( const boost::system::error_code& ec )
{
    #ifndef NDEBUG
    {   mutex::scoped_lock lk(debug_mutex);
        std::cout << __FUNCTION__ << ", ec: " << ec << std::endl;
    }
    #endif /* NDEBUG */

    // room_.deliver( message_from_string( "[Server] Error occured. Shutting down") );
    close();
}

/* file transfer */
/* ------------------------------------------------------------------------- */
void ChatSession::handle_file_start( const boost::system::error_code& ec
                                   , std::size_t /*length*/ )
{
    #ifndef NDEBUG
    {   mutex::scoped_lock lk(debug_mutex);
    std::cout << __FUNCTION__ << ", ec: " << ec << std::endl;
    }
    #endif /* NDEBUG */

    // file_recievers_.clear();
    // file_responses_remaining_ = 0;
    // signal all other participants that a file transfer is about to start
    room_.file_awaiting( read_msg_, shared_from_this() );
    do_read_header();
}

void ChatSession::handle_file_accept( const boost::system::error_code& ec
                                    , std::size_t /*length*/ )
{
    #ifndef NDEBUG
    {   mutex::scoped_lock lk(debug_mutex);
    std::cout << __FUNCTION__ << ", ec: " << ec << std::endl;
    }
    #endif /* NDEBUG */

    room_.file_accept( read_msg_, shared_from_this() );
    do_read_header();
}

void ChatSession::handle_file_refuse( const boost::system::error_code& ec
                                    , std::size_t /*length*/ )
{
    #ifndef NDEBUG
    {   mutex::scoped_lock lk(debug_mutex);
    std::cout << __FUNCTION__ << ", ec: " << ec << std::endl;
    }
    #endif /* NDEBUG */

    room_.file_refuse( read_msg_, shared_from_this() );
    do_read_header();
}

void ChatSession::handle_file_cancel( const boost::system::error_code& ec
                                    , std::size_t /*length*/ )
{
    #ifndef NDEBUG
    {   mutex::scoped_lock lk(debug_mutex);
    std::cout << __FUNCTION__ << ", ec: " << ec << std::endl;
    }
    #endif /* NDEBUG */

    room_.file_cancel( read_msg_, shared_from_this() );
    do_read_header();
}

void ChatSession::handle_file_cancel_all( const boost::system::error_code& ec
                                        , std::size_t /*length*/ )
{
    #ifndef NDEBUG
    {   mutex::scoped_lock lk(debug_mutex);
    std::cout << __FUNCTION__ << ", ec: " << ec << std::endl;
    }
    #endif /* NDEBUG */

    room_.file_cancel_all( read_msg_, shared_from_this() );
    do_read_header();
}

void ChatSession::handle_file_done( const boost::system::error_code& ec
                                  , std::size_t /*length*/ )
{
    #ifndef NDEBUG
    {   mutex::scoped_lock lk(debug_mutex);
    std::cout << __FUNCTION__ << ", ec: " << ec << std::endl;
    }
    #endif /* NDEBUG */

    room_.file_done( read_msg_, shared_from_this() );
    do_read_header();
}

/* file sending */
void ChatSession::do_file_send_start()
{
    // signal the file transfer requestor to start the transfer
    // start reading data from the file_socket_ -> handle_file_send()
    file_msg_ = Message( MessageType::FileAccept, MessageSize::Empty );
    boost::asio::async_write( file_socket_
        , boost::asio::buffer( file_msg_.data(), file_msg_.total_length() )
        , io_file_strand_.wrap(
            boost::bind( &ChatSession::handle_file_send_start, this
                , boost::asio::placeholders::error
                , boost::asio::placeholders::bytes_transferred )
        ));
}

/* file sending */
void ChatSession::handle_file_send_start( const boost::system::error_code& ec
                                  , std::size_t /* bytes_transferred */ )
{
    if( !ec ){
        do_file_send();
    }
    else{
        handle_file_error( ec );
    }
}

/* file sending */
/* Keep reading file chunks as long as they're available */
void ChatSession::do_file_send()
{
    boost::asio::async_read( file_socket_
        , boost::asio::buffer( file_send_buf_.data(), file_send_buf_.size() )
        , io_file_strand_.wrap(
            boost::bind( &ChatSession::handle_file_send, this
                , boost::asio::placeholders::error
                , boost::asio::placeholders::bytes_transferred )
        ));
}

/* file sending */
/* Add file chunks to the queue and start sending them to the
 * recieving participants */
void ChatSession::handle_file_send( const boost::system::error_code& ec
                                    , std::size_t bytes_transferred )
{
    if( !ec ){
        // push into the send data buffer
        // do_file_deliver( std::vector<char>( file_send_buf_.begin()
        //                                   , file_send_buf_.begin()
        //                                     + bytes_transferred ) );
        // io_file_strand_.post(
        //     [this, bytes_transferred]()
        //     {
        //         bool write_in_progress = !file_send_queue_.empty();
        //         file_send_queue_.emplace_back( file_send_buf_.begin()
        //                                        , file_send_buf_
        //                                         + bytes_transferred);
        //         if( !write_in_progress ){
        //             do_file_write();
        //         }
        //     }
        // )
        room_.file_deliver( std::vector<char>( file_send_buf_.begin()
                                             , (file_send_buf_.begin() 
                                               + bytes_transferred) )
                          , shared_from_this() );
        do_file_send();
    }
    else{
        handle_file_error( ec );
    }
}

/* file sending */
// void ChatSession::do_file_write()
// {
//     #ifndef NDEBUG
//     {   mutex::scoped_lock lk(debug_mutex);
//         std::cout << __FUNCTION__ << std::endl;
//     }
//     #endif /* NDEBUG */

//     while( !file_send_queue.empty() ){
//         room_.file_deliver( file_send_queue_.front(), shared_from_this() );
//         file_send_queue_.pop_front();
//     }

// }

/* file sending */
// void ChatSession::handle_file_write( const boost::system::error_code& ec
//                                    , std::size_t bytes_transferred )
// {
//     #ifndef NDEBUG
//     {   mutex::scoped_lock lk(debug_mutex);
//         std::cout << __FUNCTION__ << std::endl;
//     }
//     #endif /* NDEBUG */

//     if( !ec ){
//         file_send_queue_.pop_front();
//         if( !file_send_queue_.empty() ){
//             do_file_write();
//         }
//     }
//     else{
//         handle_file_error( ec );
//     }
// }

/* file sending */
// void ChatSession::do_file_deliver( std::vector<char>&& data )
// {
    // #ifndef NDEBUG
    // {   mutex::scoped_lock lk(debug_mutex);
    //     std::cout << __FUNCTION__ << std::endl;
    // }
    // #endif /* NDEBUG */

    // io_file_strand_.post(
    //     [this,data]()
    //     {
    //         bool write_in_progress = !file_send_queue_.empty();
    //         file_send_queue_.push_back( std::move(data) );
    //         if( !write_in_progress ){
    //             do_file_write();
    //         }
    //     }
    // );
// }

/* file recieving */
void ChatSession::file_deliver( const std::vector<char>& data )
{
    #ifndef NDEBUG
    {   mutex::scoped_lock lk(debug_mutex);
        std::cout << __FUNCTION__ << std::endl;
    }
    #endif /* NDEBUG */
    
    io_file_strand_.post(
        [this,data]()
        {
            bool write_in_progress = !file_read_queue_.empty();
            file_read_queue_.push_back( data );
            if( !write_in_progress ){
                do_file_read();
            }
        }
    );
}

/* file recieving */
void ChatSession::do_file_read()
{
    #ifndef NDEBUG
    {   mutex::scoped_lock lk(debug_mutex);
        std::cout << __FUNCTION__ << std::endl;
    }
    #endif /* NDEBUG */

    boost::asio::async_write( file_socket_
        , boost::asio::buffer( file_read_queue_.front()
                             , file_read_queue_.front().size() )
        , io_file_strand_.wrap(
            boost::bind( &ChatSession::handle_file_read, shared_from_this()
                , boost::asio::placeholders::error
                , boost::asio::placeholders::bytes_transferred )
        ));
}

/* file recieving */
void ChatSession::handle_file_read( const boost::system::error_code& ec
                                  , std::size_t bytes_transferred )
{
    #ifndef NDEBUG
    {   mutex::scoped_lock lk(debug_mutex);
        std::cout << __FUNCTION__ << "ec: " << ec
                  << ", bytes transferred: " << bytes_transferred
                  << std::endl;
    }
    #endif /* NDEBUG */

    if( !ec ){
        file_read_queue_.pop_front();
        if( !file_read_queue_.empty() ){
            do_file_read();
        }
    }
    else{
        handle_file_error( ec );
    }
}

/* Signal recieved from ChatRoom that the *sender* accepted the file transfer */
void ChatSession::file_accepted( const Message& msg
                               , ptr_ChatParticipant sender )
{
    #ifndef NDEBUG
    {   mutex::scoped_lock lk(debug_mutex);
        std::cout << __FUNCTION__ << std::endl;
    }
    #endif /* NDEBUG */

    file_recievers_.insert( sender );
    --file_responses_remaining_;
    if( file_responses_remaining_ == 0 ){
        room_.file_awaiting_complete( shared_from_this() );
        do_file_send_start();
    }
}

/* Signal recieved from ChatRoom that the *sender* refused the file transfer */
void ChatSession::file_refused( const Message& msg
                              , ptr_ChatParticipant sender )
{
    #ifndef NDEBUG
    {   mutex::scoped_lock lk(debug_mutex);
        std::cout << __FUNCTION__ << std::endl;
    }
    #endif /* NDEBUG */

    --file_responses_remaining_;
    if( file_responses_remaining_ == 0 ){
        room_.file_awaiting_complete( shared_from_this() );
        if( room_.file_reader_count( shared_from_this() ) != 0 ){
            do_file_send_start();
        }
        else{
            do_file_cancel();
        }
    }
}

void ChatSession::file_responses_remaining( std::size_t count )
{
    file_responses_remaining_ = count;
}

void ChatSession::file_msg_deliver( const Message& msg
                                  /* , ptr_ChatParticipant sender */ )
{
    #ifndef NDEBUG
    {   mutex::scoped_lock lk(debug_mutex);
        std::cout << __FUNCTION__ << std::endl;
    }
    #endif /* NDEBUG */

    io_file_strand_.post(
        [this,msg]()
        {
            bool write_in_progress = !write_msg_queue_.empty();
            write_msg_queue_.push_back( msg );
            if( !write_in_progress ){
                do_write();
            }
        }
    );
}

void ChatSession::do_file_cancel()
{
    #ifndef NDEBUG
    {   mutex::scoped_lock lk(debug_mutex);
        std::cout << __FUNCTION__ << std::endl;
    }
    #endif /* NDEBUG */

    /* TODO : implement do_file_cancel */
}

void ChatSession::handle_file_error( const boost::system::error_code& ec )
{
    room_.deliver( Message( MessageType::ChatMsg
                          , "[File Transfer Error]" ) );
    file_read_queue_.clear();
    // file_send_buf_.clear();
    file_responses_remaining_ = 0;
    // io_file_service_.stop();
}

/* ------------------------------------------------------------------------- */

void ChatSession::close()
{
    #ifndef NDEBUG
    {   mutex::scoped_lock lk(debug_mutex);
        std::cout << __FUNCTION__ << std::endl;
    }
    #endif /* NDEBUG */
    boost::system::error_code ec;
    socket_.shutdown( boost::asio::ip::tcp::socket::shutdown_both, ec );
    socket_.close( ec );
    socket_.cancel( ec );
}
/* ------------------------------------------------------------------------- */

/* Server */
/* ------------------------------------------------------------------------- */
void Server::do_accept()
{
    acceptor_.async_accept( socket_
        , boost::bind( &Server::handle_accept, this
            , boost::asio::placeholders::error ) );
}

void Server::handle_accept( const boost::system::error_code& ec )
{
    static uint8_t session_id{0};
    #ifndef NDEBUG
    {   mutex::scoped_lock lk(debug_mutex);
        std::cout << __FUNCTION__ << ", ec: " << ec << std::endl;
    }
    #endif /* NDEBUG */

    if( !ec ){
        // std::make_shared<ChatSession>( io_service_
        //                              , std::move( socket_ )
        //                              , room_, session_id++ )->start();

        do_file_accept();
    }
}

void Server::do_file_accept()
{
    file_acceptor_.async_accept( file_socket_
        , boost::bind( &Server::handle_file_accept, this
                     , boost::asio::placeholders::error ) );
}
void Server::handle_file_accept( const boost::system::error_code& ec )
{
    #ifndef NDEBUG
    {   mutex::scoped_lock lk(debug_mutex);
        std::cout << __FUNCTION__ << ", ec: " << ec << std::endl;
    }
    #endif /* NDEBUG */
    if( !ec ){
        
        std::make_shared<ChatSession>( io_service_
                                     , io_file_service_
                                     , std::move( socket_ )
                                     , std::move( file_socket_ )
                                     , room_, 1 )->start();

        do_accept();
    }
}
/* ------------------------------------------------------------------------- */
