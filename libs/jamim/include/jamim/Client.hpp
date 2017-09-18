/* Client -- synchronous server query */
#ifndef CLIENT_HPP_
#define CLIENT_HPP_

#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include "Message.hpp"

/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */

/* Client */
/* ------------------------------------------------------------------------- */
class Client
{
public:
    Client( boost::asio::io_service& io_service
          , boost::asio::ip::tcp::resolver::iterator  endpoint_iterator )
        : io_service_( io_service )
        , io_strand_( io_service )
        , socket_( io_service )
        {
            do_connect( endpoint_iterator );
        }

    void write( const Message& msg );
    void close( );

private:
    void do_connect( boost::asio::ip::tcp::resolver::iterator endpoint_iterator );
    void handle_connect( const boost::system::error_code& ec
                       , boost::asio::ip::tcp::resolver::iterator /* it */ );

    void do_read_header();
    void handle_read_header(const boost::system::error_code& ec, std::size_t /*length*/);

    void do_read_body();
    void handle_read_body(const boost::system::error_code& ec, std::size_t /*length*/);

    void do_read_command();
    void handle_read_command(const boost::system::error_code& ec, std::size_t /*length*/);

    void do_write();
    void handle_write(const boost::system::error_code& ec, std::size_t /*length*/);

    /* TODO : handle MessageType::FileMsg */

    /* TODO : handle unknown MessageType */

    void handle_error( const boost::system::error_code& ec );
private:
    boost::asio::io_service&        io_service_;
    boost::asio::strand             io_strand_;
    boost::asio::ip::tcp::socket    socket_;
    Message                         read_msg_;
    std::deque< Message >           write_msg_queue_;
}; //Client
/* ------------------------------------------------------------------------- */

#endif /* CLIENT_HPP_ */
