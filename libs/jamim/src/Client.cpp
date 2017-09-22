#include "Client.hpp"
#include <boost/bind.hpp>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>

#ifndef NDEBUG
static boost::mutex debug_mutex;
#endif /* NDEBUG */

namespace fs = boost::filesystem;

/* Client */
/* ------------------------------------------------------------------------- */

/* static variables */
const std::unordered_map<MessageType, Client::Handler>  Client::s_handler_map_{
      { MessageType::EmptyMsg         , &Client::handle_empty }
    , { MessageType::ChatMsg          , &Client::handle_chat_message }
    , { MessageType::CmdQuit          , &Client::handle_quit }
    , { MessageType::FileCancel       , &Client::handle_file_cancel }
    , { MessageType::FileCancelAll    , &Client::handle_file_cancel_all }
    , { MessageType::FileMsg          , &Client::handle_file_read_start }
    , { MessageType::FileDone         , &Client::handle_file_done }
    , { MessageType::Unknown          , &Client::handle_unknown }
};

/* public */
void Client::write( const Message& msg )
{
    io_strand_.post(
        [this,msg]()
        {
            bool write_in_progress = !write_msg_queue_.empty();
            write_msg_queue_.push_back( msg );
            if( !write_in_progress ){
                do_write();
            }
        });
}

void Client::start_file( const Message& msg )
{
    /* #1 check if the file exists
     * #2 Open the file, get file size
     * #3 Build new file message with file size + file name
     */
    fs::path filepath( msg.body_to_string() );
    if( fs::is_regular_file( filepath ) ){
        #ifndef NDEBUG
        boost::mutex::scoped_lock lk(debug_mutex);
        std::cout << "file " << filepath << " exists, size: "
                  << file_size( filepath ) << std::endl;
        #endif /* NDEBUG */

        // do_start_file()
        io_file_strand_.post(
            [this,filepath]()
            {
                bool write_in_progress = !file_queue_.empty();
                file_queue_.push_back( filepath );
                if( !write_in_progress ){
                    do_file_send_start();
                }
            });
    }
    else{
        boost::mutex::scoped_lock lk(debug_mutex);
        std::cout << "[Error: File " << filepath << " not found.]" << std::endl;
    }
}

void Client::close()
{
    #ifndef NDEBUG
    { boost::mutex::scoped_lock lk(debug_mutex);
        std::cout << "[" << __FUNCTION__ << "]" << std::endl;
    }
    #endif /* NDEBUG */
    io_strand_.post( [this](){ socket_.close(); } );
}

/* private */

/* connecting */
/* ------------------------------------------------------------------------- */
void Client::do_connect( boost::asio::ip::tcp::resolver::iterator endpoint_iterator )
{
    boost::asio::async_connect( socket_, endpoint_iterator,
        boost::bind( &Client::handle_connect, this
                   , boost::asio::placeholders::error
                   , boost::asio::placeholders::iterator) );
}

void Client::handle_connect( const boost::system::error_code& ec
                           , boost::asio::ip::tcp::resolver::iterator /* it */ )
{
    if( !ec ){
        std::cout << "[Connected]" << std::endl;
        do_read_header();
    }
}

void Client::do_file_connect( boost::asio::ip::tcp::resolver::iterator endpoint_iterator )
{
    boost::asio::async_connect( file_socket_, endpoint_iterator
        , boost::bind( &Client::handle_file_connect, this
                     , boost::asio::placeholders::error
                     , boost::asio::placeholders::iterator ) );
}
void Client::handle_file_connect( const boost::system::error_code& ec
                                , boost::asio::ip::tcp::resolver::iterator /*it*/ )
{   
    #ifndef NDEBUG
    {   boost::mutex::scoped_lock lk(debug_mutex);
        std::cout << "[" << __FUNCTION__ << ", ec: " << ec << "]" << std::endl;    
    if( !ec ){
        std::cout << "[File transfer socket connected]" << std::endl;
    }
    }
    #endif /* NDEBUG */
}
/* ------------------------------------------------------------------------- */

/* general communitaction */
/* ------------------------------------------------------------------------- */
void Client::do_read_header()
{
    boost::asio::async_read( socket_
        , boost::asio::buffer( read_msg_.data(), read_msg_.header_length() )
        , io_strand_.wrap(
            boost::bind( &Client::handle_read_header, this
                       , boost::asio::placeholders::error
                       , boost::asio::placeholders::bytes_transferred )
        ));
}
void Client::handle_read_header( const boost::system::error_code& ec
                               , std::size_t /*length*/ )
{
    if( !ec ){
        read_msg_.sync();
        do_read_body();
    }
    else{
        handle_error( ec );
    }
}

void Client::do_read_body()
{
    auto it = s_handler_map_.find( read_msg_.msg_type() );
    Handler handler = ( (it!=s_handler_map_.end()) ? it->second 
                                                   : &Client::handle_unknown );
    boost::asio::async_read( socket_
        , boost::asio::buffer( read_msg_.msg_body(), read_msg_.body_length() )
        , io_strand_.wrap(
            boost::bind( handler, this
                       , boost::asio::placeholders::error
                       , boost::asio::placeholders::bytes_transferred )
        ));
}

void Client::handle_chat_message( const boost::system::error_code& ec
                             , std::size_t /*length*/ )
{
    if( !ec ){
        std::cout << read_msg_ << std::endl;
        do_read_header();
    }
    else{
        handle_error( ec );
    }
}

void Client::handle_empty(const boost::system::error_code& ec
                         , std::size_t /*length*/)
{
    #ifndef NDEBUG
    { boost::mutex::scoped_lock lk(debug_mutex);
        std::cout << "[" << __FUNCTION__ << "]" << std::endl;
    }
    #endif /* NDEBUG */
}

void Client::handle_quit( const boost::system::error_code& ec
                                , std::size_t /*length*/ )
{
    if( !ec ){
        std::cout << read_msg_ << std::endl;
        do_read_header();
    }
    else{
        handle_error( ec );
    }
}

void Client::handle_unknown(const boost::system::error_code& /*ec*/
                           , std::size_t /*length*/)
{
    std::cout << "[Unknown message type] " << read_msg_ << std::endl;
    do_read_header();
}

void Client::do_write()
{
    boost::asio::async_write( socket_
        , boost::asio::buffer( write_msg_queue_.front().data()
                             , write_msg_queue_.front().total_length())
        , io_strand_.wrap(
            boost::bind( &Client::handle_write, this
                       , boost::asio::placeholders::error
                       , boost::asio::placeholders::iterator )
        ));
}
void Client::handle_write( const boost::system::error_code& ec
                         , std::size_t /*length*/ )
{
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

void Client::handle_error( const boost::system::error_code& ec )
{
    // std::cout << "Error occured: " << ec << std::endl;
    close();
}
/* ------------------------------------------------------------------------- */

/* file transfer */
/* ------------------------------------------------------------------------- */
void Client::do_start_file()
{

}

void Client::do_file_send_start()
{
    // signal the intent to transfer a file
    // file_msg_ = 
    write( make_file_message( file_size(file_queue_.front())
                            , file_queue_.front().string() ) );
    // wait for the file transfer to be accepted on the receiving end
    boost::asio::async_read( file_socket_
        , boost::asio::buffer( file_msg_.data(), file_msg_.header_length() )
        , io_file_strand_.wrap(
            boost::bind( &Client::handle_file_send_start, this
                       , boost::asio::placeholders::error
                       , boost::asio::placeholders::bytes_transferred )
        ));
}

void Client::handle_file_send_start( const boost::system::error_code& ec
                              , std::size_t /*length*/ )
{
    if( !ec ){
        file_msg_.sync();
        if( MessageType::FileAccept == file_msg_.msg_type() ){
            do_file_send();
        }
        else{
            boost::mutex::scoped_lock lk(debug_mutex);
            std::cout << "[File transfer refused " << file_queue_.front() 
                      << "]" << std::endl;
            file_queue_.pop_front();
            if( !file_queue_.empty() ){
                do_file_send_start();
            }
        }
    }
    else{
        #ifndef NDEBUG
        {   boost::mutex::scoped_lock lk(debug_mutex);
            std::cout << __FUNCTION__ << ", ec: " << ec << std::endl;
        }
        #endif /* NDEBUG */
        handle_error( ec );
    }
}

void Client::do_file_send( )
{
    // Open file in binary read mode
    send_file_.open( file_queue_.front().string(), std::ios_base::binary );

    if( send_file_ ){
        send_file_.read( send_file_buf_.data(), send_file_buf_.size() );
        if( send_file_.gcount() <= 0 ){
            handle_file_send_error();
        }
        boost::asio::async_write( file_socket_
            , boost::asio::buffer( send_file_buf_.data(), send_file_.gcount() )
            , io_file_strand_.wrap(
                boost::bind( &Client::handle_file_send, this
                           , boost::asio::placeholders::error
                           , boost::asio::placeholders::bytes_transferred ) ) );
    }
    else{
        #ifndef NDEBUG
        { boost::mutex::scoped_lock lk(debug_mutex);
            std::cout << "Failed to open the file " << file_queue_.front()
                      << std::endl;
        }
        #endif /* NDEBUG */
        handle_file_send_error();
    }
}

void Client::handle_file_send( const boost::system::error_code& ec
                              , std::size_t /*length*/ )
{
    if( !ec ){
        if( send_file_ ){
            send_file_.read( send_file_buf_.data(), send_file_buf_.size() );
            if( send_file_.gcount() <= 0 ){
                handle_file_send_error();
            }
            boost::asio::async_write( file_socket_
                , boost::asio::buffer( send_file_buf_.data(), send_file_.gcount() )
                , io_file_strand_.wrap(
                    boost::bind( &Client::handle_file_send, this
                               , boost::asio::placeholders::error
                               , boost::asio::placeholders::bytes_transferred ) ) );
        }
        else if( send_file_.eof() ){
            #ifndef NDEBUG
            { boost::mutex::scoped_lock lk(debug_mutex);
                std::cout << "[File " << file_queue_.front() << " EOF reached.]"
                          << std::endl;
            }
            #endif /* NDEBUG */
            send_file_.close();
            file_queue_.pop_front();
            if( !file_queue_.empty() ){
                do_file_send_start();
            }
        }
        else{
            #ifndef NDEBUG
            { boost::mutex::scoped_lock lk(debug_mutex);
                std::cout << "[File " << file_queue_.front() << " error.]";
            }
            handle_file_send_error();
            file_queue_.pop_front();
            if( !file_queue_.empty() ){
                do_file_send_start();
            }
            #endif /* NDEBUG */
        }
    }else{
        handle_error( ec );
    }
}

void Client::handle_file_send_error()
{
    #ifndef NDEBUG
    { boost::mutex::scoped_lock lk(debug_mutex);
        std::cout << __FUNCTION__ << std::endl;
    }
    #endif /* NDEBUG */
    /* TODO: signal file cancel */
    write( message_from_string("[File send error]\n") );
    write( Message( MessageType::FileCancel, MessageSize::Empty ) );
    send_file_.close();
}

void Client::handle_file_read_start( const boost::system::error_code& ec
                                , std::size_t /*length*/ )
{
    if( !ec ){
        /* TODO : implement handle_file_read_start */
        read_file_size_ = read_msg_.file_size();
        std::cout << "Request to start file transfer: "
                  << read_msg_.body_to_string()
                  << "(" << (read_file_size_ / 8 )  <<" bytes)."
                  << "\nAccept [y\\n]? " << std::endl;
        std::string line;
        std::getline( std::cin, line );
        if( !line.empty() && std::toupper(line[0]) == 'Y' ){
            std::cout << "Enter path to save the file" <<std::endl;
            while( std::cout<<"> " && std::getline( std::cin, line ) ){
                if( fs::is_regular_file( fs::path(line) ) ){
                    std::cout << line << " exists. Please select a different filename."
                              << std::endl;
                    continue;
                }
                break;
            }
            /* TODO : handle_file_read_start */
            // file_msg_ = 
            write( Message( MessageType::FileAccept, MessageSize::Empty ) );
            // boost::asio::async_write( socket_
            //     , boost::asio::buffer( file_msg_.data(), file_msg.total_length() )
            //     , boost::bind( &Client::do_file_read, this ) );
            read_file_.open( line, std::ios_base::binary );
            if( read_file_ ){
                do_file_read();
            }
            else{
                std::cout << "Failed to open to file."
                << std::endl;
                handle_file_read_error();
            }
            do_read_header();
        }
        else{
            // file_msg_ = 
            write( Message( MessageType::FileRefuse, MessageSize::Empty ) );
            // boost::asio::async_write( file_socket_
            //     , boost::asio::buffer( file_msg_.data(), file_msg.total_length() )
            //     , boost::bind( &Client::do_read_header, this ) );
            do_read_header();
        }
        
    }
    else{
        // file_msg_ = ;
        write( Message( MessageType::FileRefuse, MessageSize::Empty ) );
        // boost::asio::async_write( file_socket_
        //     , boost::asio::buffer( file_msg_.data(), file_msg_.total_length() )
        //     , boost::bind( &Client::handle_error, this
        //                  , boost::asio::placeholders::error ) );
        handle_error( ec );
    }
}

void Client::do_file_read()
{
    boost::asio::async_read( file_socket_
        , boost::asio::buffer( read_file_buf_.data(), read_file_buf_.size() )
        , io_file_strand_.wrap(
            boost::bind( &Client::handle_file_read, this
                       , boost::asio::placeholders::error
                       , boost::asio::placeholders::bytes_transferred ) ) );
}

void Client::handle_file_read( const boost::system::error_code& ec
                             , std::size_t bytes_transferred )
{
     
    if( 0 < bytes_transferred ){
        read_file_.write( read_file_buf_.data(), bytes_transferred );
        if( read_file_.tellp() < read_file_size_ ){
            if(!ec){
            boost::asio::async_read( file_socket_
                , boost::asio::buffer( read_file_buf_.data(), read_file_buf_.size() )
                , io_file_strand_.wrap(
                    boost::bind( &Client::handle_file_read, this
                               , boost::asio::placeholders::error
                               , boost::asio::placeholders::bytes_transferred )
                ));
            }
        }
        else{
            do_file_read_done();
        }
    }
    if( ec ){
        handle_file_read_error();
    }
}

void Client::do_file_read_done()
{
    boost::mutex::scoped_lock lk(debug_mutex);
    std::cout << "[Transfer complete. (bytes expected: " << (read_file_size_/8)
              << ", got: " << (read_file_.tellp() / 8 ) << "]" <<std::endl;
    read_file_.flush();
    read_file_.close();
    write( Message( MessageType::FileDone, MessageSize::Empty ) );
}

void Client::handle_file_read_error()
{
    { boost::mutex::scoped_lock lk(debug_mutex);
        std::cout << "[File read error]" << std::endl;
    }
    read_file_.close();
    read_file_size_ = 0;
    write( message_from_string( "[File read error. Transfer cancelled.]" ) );
    write( Message( MessageType::FileCancel, MessageSize::Empty ) );
    do_read_header();
}

void Client::handle_file_done( const boost::system::error_code& ec
                             , std::size_t /*length*/)
{
    #ifndef NDEBUG
    { boost::mutex::scoped_lock lk(debug_mutex);
        std::cout << __FUNCTION__ << std::endl;
    }
    #endif /* NDEBUG */
    do_read_header();
}

void Client::handle_file_cancel( const boost::system::error_code& ec
                                , std::size_t /*length*/ )
{
    if( !ec ){
        /* TODO : implement handle_file_cancel */
        std::cout << "Current file transfer cancelled." << std::endl;
        do_read_header();
    }
    else{
        handle_error( ec );
    }
}

void Client::handle_file_cancel_all( const boost::system::error_code& ec
                                , std::size_t /*length*/ )
{
    if( !ec ){
        /* TODO : implement handle_file_cancel_all */
        std::cout << "All files cancelled." << std::endl;
        do_read_header();
    }
    else{
        handle_error( ec );
    }
}
/* ------------------------------------------------------------------------- */
