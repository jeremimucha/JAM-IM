#ifndef SERVER_HPP_
#define SERVER_HPP_

#include <iostream>
#include <ctime>
#include <string>
#include <memory>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
using boost::asio::ip::tcp;


std::string make_daytime_string();


class tcp_server;
class tcp_connection;


// Use shared_ptr and enable_shared_from_this because we want to keep the
// tcp_connection object alive as long as there is an operation that refers to it
class tcp_connection : public std::enable_shared_from_this<tcp_connection>
{

public:
    typedef std::shared_ptr<tcp_connection> pointer;

    static pointer create(boost::asio::io_service& io_service)
    {
        return std::make_shared<tcp_connection>(io_service);
    }

    tcp::socket& socket()
    {
        return socket_;
    }

    void start();
    
    tcp_connection(boost::asio::io_service& io_service)
        : socket_(io_service)
        { }
private:
    
    void handle_write(const boost::system::error_code& /*error*/
                     ,std::size_t /*bytes_transferred*/);

    tcp::socket socket_;
    std::string message_;
};

class tcp_server
{
public:
    // initialize an acceptor to listen on TCP port 13
    tcp_server(boost::asio::io_service& io_service)
        : acceptor_(io_service, tcp::endpoint(tcp::v4(), 13))
        {
            start_accept();
        }

private:
    void start_accept();

    // Called when the asynchronous accept operation initiated
    // by start_accept() finishes.
    void handle_accept(tcp_connection::pointer new_connection,
                       const boost::system::error_code& error);

/*--- member variables ---*/
    tcp::acceptor acceptor_;
};


#endif /* SERVER_HPP_ */
