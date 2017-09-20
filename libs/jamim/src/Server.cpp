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
/* ------------------------------------------------------------------------- */

/* ChatSession */
/* ------------------------------------------------------------------------- */

/* static variables */
const std::unordered_map<MessageType, ChatSession::Handler>
ChatSession::s_handler_map_{
      { MessageType::EmptyMsg         , &ChatSession::handle_empty }
    , { MessageType::ChatMsg          , &ChatSession::handle_read_body }
    , { MessageType::CmdQuit          , &ChatSession::handle_quit }
    , { MessageType::CmdStartFile     , &ChatSession::handle_start_file }
    , { MessageType::CmdCancelCurrent , &ChatSession::handle_cancel_current }
    , { MessageType::CmdCancelAll     , &ChatSession::handle_cancel_all }
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

void ChatSession::handle_start_file( const boost::system::error_code& ec
                                   , std::size_t /*length*/
                                   /* , ptr_ChatParticipant sender */ )
{
    #ifndef NDEBUG
    {   mutex::scoped_lock lk(debug_mutex);
        std::cout << __FUNCTION__ << ", ec: " << ec << std::endl;
    }
    #endif /* NDEBUG */

    do_read_header();
}

void ChatSession::handle_cancel_current( const boost::system::error_code& ec
                                       , std::size_t /*length*/
                                       /* , ptr_ChatParticipant sender */ )
{
    #ifndef NDEBUG
    {   mutex::scoped_lock lk(debug_mutex);
        std::cout << __FUNCTION__ << ", ec: " << ec << std::endl;
    }
    #endif /* NDEBUG */

    do_read_header();
}

void ChatSession::handle_cancel_all( const boost::system::error_code& ec
                                   , std::size_t /*length*/
                                   /* , ptr_ChatParticipant sender */ )
{
    #ifndef NDEBUG
    {   mutex::scoped_lock lk(debug_mutex);
        std::cout << __FUNCTION__ << ", ec: " << ec << std::endl;
    }
    #endif /* NDEBUG */

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
                                     , std::move( socket_ )
                                     , std::move( file_socket_ )
                                     , room_, 1 )->start();

        do_accept();
    }
}
/* ------------------------------------------------------------------------- */
