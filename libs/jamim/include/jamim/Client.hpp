/* Client -- synchronous server query */
#ifndef CLIENT_HPP_
#define CLIENT_HPP_

#include <iostream>
#include <fstream>
#include <unordered_map>
#include <boost/array.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <boost/filesystem.hpp>
#include "Message.hpp"

/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */

/* Client */
/* ------------------------------------------------------------------------- */
class Client
{
public:
    typedef void (Client::*Handler)( const boost::system::error_code&
                                   , std::size_t );

    Client( boost::asio::io_service& io_service
          , boost::asio::ip::tcp::resolver::iterator  endpoint_iterator
          , boost::asio::io_service& io_file_service
          , boost::asio::ip::tcp::resolver::iterator file_endpoint_iterator )
        : io_service_( io_service )
        , io_file_service_( io_file_service )
        , io_strand_( io_service )
        , socket_( io_service )
        , file_socket_( io_file_service_ )
        {
            do_connect( endpoint_iterator );
            do_file_connect( file_endpoint_iterator );
        }

    void write( const Message& msg );
    void start_file( const Message& msg );
    void close( );

private:
    void do_connect( boost::asio::ip::tcp::resolver::iterator endpoint_iterator );
    void handle_connect( const boost::system::error_code& ec
                       , boost::asio::ip::tcp::resolver::iterator /* it */ );

    void do_file_connect( boost::asio::ip::tcp::resolver::iterator endpoint_iterator );
    void handle_file_connect( const boost::system::error_code& ec
                        , boost::asio::ip::tcp::resolver::iterator /* it */ );

    void do_read_header();
    void handle_read_header(const boost::system::error_code& ec, std::size_t /*length*/);

    void do_read_body();
    void handle_read_body(const boost::system::error_code& ec, std::size_t /*length*/);

    void handle_empty(const boost::system::error_code& ec, std::size_t /*length*/);

    // void do_start_file();
    void handle_start_file(const boost::system::error_code& ec, std::size_t /*lenght*/ );

    // void do_cancel_current();
    void handle_cancel_current(const boost::system::error_code& ec, std::size_t /*length*/);

    // void do_cancel_all();
    void handle_cancel_all(const boost::system::error_code& ec, std::size_t /*length*/);
    
    void do_write();
    void handle_write(const boost::system::error_code& ec, std::size_t /*length*/);

    // void do_quit();
    void handle_quit(const boost::system::error_code& ec, std::size_t /*length*/);

    void handle_unknown(const boost::system::error_code& /*ec*/, std::size_t /*length*/);

    void handle_error( const boost::system::error_code& ec );

    void do_start_send_file();
    void handle_start_send_file( const boost::system::error_code& ec
                          , std::size_t /*length*/);
    
    void do_send_file();
    void handle_send_file( const boost::system::error_code& ec
                          , std::size_t /*length*/);

    void handle_file_error();
private:
    boost::asio::io_service&        io_service_;
    boost::asio::io_service&        io_file_service_;
    boost::asio::strand             io_strand_;
    boost::asio::ip::tcp::socket    socket_;
    boost::asio::ip::tcp::socket    file_socket_;
    Message                         read_msg_;
    std::ifstream                   write_file_;
    std::ofstream                   read_file_;
    std::array<uint8_t, 4096>       file_buf_;
    std::deque< Message >           write_msg_queue_;
    std::deque< boost::filesystem::path >  file_queue_;

    static const std::unordered_map<MessageType, Handler>  s_handler_map_;

}; //Client
/* ------------------------------------------------------------------------- */

#endif /* CLIENT_HPP_ */
