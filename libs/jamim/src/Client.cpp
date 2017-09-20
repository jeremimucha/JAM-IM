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
    , { MessageType::ChatMsg          , &Client::handle_read_body }
    , { MessageType::CmdQuit          , &Client::handle_quit }
    , { MessageType::CmdStartFile     , &Client::handle_start_file }
    , { MessageType::CmdCancelCurrent , &Client::handle_cancel_current }
    , { MessageType::CmdCancelAll     , &Client::handle_cancel_all }
    , { MessageType::Unknown          , &Client::handle_unknown }
};

/* public */
void Client::write( const Message& msg )
{
    io_strand_.post( [this,msg]()
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

        io_file_service_.post(
            [this,filepath]()
            {
                bool write_in_progress = !file_queue_.empty();
                file_queue_.push_back( filepath );
                if( !write_in_progress ){
                    do_start_send_file();
                }
            });
    }
    else{
        boost::mutex::scoped_lock lk(debug_mutex);
        std::cout << "[Error] File doesn't exist." << std::endl;
    }
}

void Client::close()
{
    io_strand_.post( [this](){ socket_.close(); } );
}

/* private */
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
        std::cout << "Connected" << std::endl;
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
        std::cout << __FUNCTION__ << ", ec: " << ec << std::endl;    
    if( !ec ){
        std::cout << "File transfer socket connected " << std::endl;
    }
    }
    #endif /* NDEBUG */
}

void Client::do_start_send_file()
{
    write( make_file_message( file_size(file_queue_.front())
                            , file_queue_.front().string() ) );
    // wait for the file transfer to be accepted on the receiving end
    boost::asio::async_read( file_socket_
        , boost::asio::buffer( read_msg_.data(), read_msg_.header_length() )
        , io_strand_.wrap(
            boost::bind( &Client::handle_start_send_file, this
                       , boost::asio::placeholders::error
                       , boost::asio::placeholders::bytes_transferred )
        ));
}

void Client::handle_start_send_file( const boost::system::error_code& ec
                              , std::size_t /*length*/ )
{
    if( !ec ){
        if( MessageType::FileAccept == read_msg_.msg_type() ){
            do_send_file();
        }
        else{
            boost::mutex::scoped_lock lk(debug_mutex);
            std::cout << "File transfer refused " << file_queue_.front() << std::endl;
            file_queue_.pop_front();
            if( !file_queue_.empty() ){
                do_start_send_file();
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

void Client::do_send_file( )
{
    write_file_.open( file_queue_.front().string(), std::ios_base::binary );
    if( write_file_ ){
        write_file_.read( file_buf_.array(), file_buf_.size() );
        if( write_file_.gcount() <= 0 ){
            handle_file_error();
        }
        boost::asio::async_write( file_socket_
            , boost::asio::buffer( file_buf_.data(), write_file_.gcount() )
            , boost::bind( &Client::handle_send_file, this
                         , boost::asio::placeholders::error
                         , boost::asio::placeholders::bytes_transferred ) );
    }
}

void Client::handle_send_file( const boost::system::error_code& ec
                              , std::size_t /*length*/
{
    if( !ec ){
        if( write_file_ ){
            write_file_.read( &(file_buf_.data()[0]), file_buf_.size() );
            if( write_file_.gcount() <= 0 ){
                handle_file_error();
            }
            boost::asio::async_write( file_socket_
                , boost::asio::buffer( file_buf_.data(), file_buf_.size() )
                , boost::bind( &Client::handle_send_file, this
                             , boost::asio::placeholders::error
                             , boost::asio::placeholders::bytes_transferred ) );
        }
        else if( write_file_.eof() ){
            #ifndef NDEBUG
            { boost::mutex::scoped_lock lk(debug_mutex);
                std::cout << "File " << file_queue_.front() << " successfully transferred.";
            }
            #endif /* NDEBUG */
            write_file_.close();
            file_queue_.pop_front();
            if( !file_queue_.empty() ){
                do_start_send_file();
            }
        }
        else{
            #ifndef NDEBUG
            { boost::mutex::scoped_lock lk(debug_mutex);
                std::cout << "File " << file_queue_.front() << " error.";
            }
            handle_file_error();
            file_queue_.pop_front();
            if( !file_queue_.empty() ){
                do_start_send_file();
            }
            #endif /* NDEBUG */
        }
    }
}

void Client::handle_file_error()
{
    #ifndef NDEBUG
    { boost::mutex::scoped_lock lk(debug_mutex);
        std::cout << __FUNCTION__ << std::endl;
    }
    #endif /* NDEBUG */
    write_file_.close();
}


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
                       , boost::asio::placeholders::iterator )
        ));
}
void Client::handle_read_body( const boost::system::error_code& ec
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

}

/* void Client::do_start_file()
{
    boost::asio::async_read( socket_
        , boost::asio::buffer( read_msg_.msg_body(), read_msg_.body_length() )
        , io_strand_.wrap(
            boost::bind( &Client::handle_start_file, this
                       , boost::asio::placeholders::error
                       , boost::asio::placeholders::iterator )
        ));
} */
void Client::handle_start_file( const boost::system::error_code& ec
                                , std::size_t /*length*/ )
{
    if( !ec ){
        /* TODO : implement handle_start_file */
        std::cout << "Request to start a file transfer: " << read_msg_ << std::endl;
        do_read_header();
    }
    else{
        handle_error( ec );
    }
}

/* void Client::do_cancel_current()
{
    boost::asio::async_read( socket_
        , boost::asio::buffer( read_msg_.msg_body(), read_msg_.body_length() )
        , io_strand_.wrap(
            boost::bind( &Client::handle_cancel_current, this
                       , boost::asio::placeholders::error
                       , boost::asio::placeholders::iterator )
        ));
} */
void Client::handle_cancel_current( const boost::system::error_code& ec
                                , std::size_t /*length*/ )
{
    if( !ec ){
        /* TODO : implement handle_cancel_current */
        std::cout << "Current file transfer cancelled." << std::endl;
        do_read_header();
    }
    else{
        handle_error( ec );
    }
}

/* void Client::do_cancel_all()
{
    boost::asio::async_read( socket_
        , boost::asio::buffer( read_msg_.msg_body(), read_msg_.body_length() )
        , io_strand_.wrap(
            boost::bind( &Client::handle_cancel_all, this
                       , boost::asio::placeholders::error
                       , boost::asio::placeholders::iterator )
        ));
} */
void Client::handle_cancel_all( const boost::system::error_code& ec
                                , std::size_t /*length*/ )
{
    if( !ec ){
        /* TODO : implement handle_cancel_all */
        std::cout << "All files cancelled." << std::endl;
        do_read_header();
    }
    else{
        handle_error( ec );
    }
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

/* void Client::do_quit()
{
    boost::asio::async_read( socket_
        , boost::asio::buffer( read_msg_.msg_body(), read_msg_.body_length() )
        , io_strand_.wrap(
            boost::bind( &Client::handle_quit, this
                       , boost::asio::placeholders::error
                       , boost::asio::placeholders::iterator )
        ));
} */
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
    std::cout << "Unknown message type: " << read_msg_ << std::endl;
    do_read_header();
}

void Client::handle_error( const boost::system::error_code& ec )
{
    // std::cout << "Error occured: " << ec << std::endl;
    close();
}
/* ------------------------------------------------------------------------- */
