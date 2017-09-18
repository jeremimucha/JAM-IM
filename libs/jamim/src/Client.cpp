#include "Client.hpp"
#include <boost/bind.hpp>


/* Client */
/* ------------------------------------------------------------------------- */

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
                       , boost::asio::placeholders::iterator )
        ));
}
void Client::handle_read_header( const boost::system::error_code& ec
                               , std::size_t /*length*/ )
{
    // Need to update the internal representation of header
    if( !ec ){
        read_msg_.sync();
        switch( read_msg_.msg_type() ){
            case MessageType::ChatMsg : do_read_body();
                break;
            case MessageType::CommandMsg : do_read_command();
                break;
            case MessageType::FileMsg :     /* TODO */
                break;
            default :   /* TODO */
                break;
        }
    }
    else{
        handle_error( ec );
    }
}

void Client::do_read_body()
{
    boost::asio::async_read( socket_
        , boost::asio::buffer( read_msg_.msg_body(), read_msg_.body_length() )
        , io_strand_.wrap(
            boost::bind( &Client::handle_read_body, this
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

void Client::do_read_command()
{
    boost::asio::async_read( socket_
        , boost::asio::buffer( read_msg_.msg_body(), read_msg_.body_length() )
        , io_strand_.wrap(
            boost::bind( &Client::handle_read_command, this
                       , boost::asio::placeholders::error
                       , boost::asio::placeholders::iterator )
        ));
}
void Client::handle_read_command( const boost::system::error_code& ec
                                , std::size_t /*length*/ )
{
    if( !ec ){
        /* TODO : implement handle_read_command */
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

void Client::handle_error( const boost::system::error_code& ec )
{
    std::cout << "Error occured: " << ec << std::endl;
    close();
}
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
std::ostream& operator<<( std::ostream& os, const Message& msg )
{
    return os << "\t>>> " 
              << std::string( msg.msg_body(), msg.msg_body()+msg.body_length() )
              << std::endl;
}
/* ------------------------------------------------------------------------- */