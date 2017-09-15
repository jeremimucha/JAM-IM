#include "jamim/Server.hpp"
#include <boost/asio.hpp>


int main()
{
    try{
        boost::asio::io_service io_service;
        tcp_server server(io_service);
        io_service.run();
    }
    catch( std::exception& e ){
        std::cerr << e.what() << std::endl;
    }

}
