#include "Server.hpp"


std::string make_daytime_string()
{
    using namespace std;    // for time_t and ctime
    time_t now = time(0);
    return ctime(&now);
}


/* ------------------------------------------------------------------------- */
void tcp_connection::start()
{
    // Keep the data in a member variable to make sure it's valid
    // until the asynchronous operation is complete
    message_ = make_daytime_string();

    boost::asio::async_write(socket_, boost::asio::buffer(message_),
        boost::bind(&tcp_connection::handle_write, shared_from_this(),
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred));
}

void tcp_connection::handle_write( const boost::system::error_code& err
                                 , std::size_t len )
{
    if( err ){
        std::cerr << "handle_write -- error occured: " << err << std::endl;
    }
    else
        std::cout << "handle_write -- bytes writen: " << len << std::endl;
}
/* ------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------- */
void tcp_server::start_accept()
{   
    // Create a socket
    tcp_connection::pointer new_connection =
        tcp_connection::create(acceptor_.get_io_service());

    // initiate asynchronous accept operation to wait for a new connection
    acceptor_.async_accept( new_connection->socket(),
        boost::bind(&tcp_server::handle_accept, this, new_connection,
            boost::asio::placeholders::error) );
    std::cerr << "awaiting connection..." << std::endl;
}


// Called when the asynchronous accept operation initiated
// by start_accept() finishes.
void tcp_server::handle_accept( tcp_connection::pointer new_connection,
                                const boost::system::error_code& error )
{
    // Service the client request
    if( !error ){
        new_connection->start();
        std::cerr << "new connection started" << std::endl;
    }
    // initiate next accept operation
    start_accept();
}
/* ------------------------------------------------------------------------- */
