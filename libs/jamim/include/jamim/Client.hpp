/* Client -- synchronous server query */
#ifndef CLIENT_HPP_
#define CLIENT_HPP_

#include <iostream>
#include <boost/array.hpp>
#include <boost/asio.hpp>
using boost::asio::ip::tcp;


class Client
{
public:
    Client( const char* ip, const char* service );

    bool connect();
    void read();
private:
    boost::asio::io_service io_service_;
    tcp::resolver           resolver_;
    tcp::socket             socket_;
    tcp::resolver::iterator endpoint_it_;
};


#endif /* CLIENT_HPP_ */
