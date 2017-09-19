#include <cstdlib>
#include "jamim/Server.hpp"
#include <boost/asio.hpp>


using boost::asio::ip::tcp;

int main( int argc, char* argv[] )
{
    if( argc < 2 ){
        std::cerr << "Usage: Server <port1> " << std::endl;
        return 1;
    }

    try{
    tcp::endpoint endpoint( tcp::v4(), std::atoi(argv[1]) );
    Server chat_server( endpoint );
    chat_server.run();
    }
    catch( std::exception& e ){
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}
