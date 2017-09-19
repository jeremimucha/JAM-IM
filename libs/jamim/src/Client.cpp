#include "Client.hpp"
#include <boost/bind.hpp>


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
