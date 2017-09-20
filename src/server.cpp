#include <cstdlib>
#include "jamim/Server.hpp"
#include <boost/asio.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/thread.hpp>


using boost::asio::ip::tcp;

int main( int argc, char* argv[] )
{
    if( argc < 3 ){
        std::cerr << "Usage: Server <port1> <port2> " << std::endl;
        return 1;
    }

    try{
    tcp::endpoint endpoint( tcp::v4(), std::atoi(argv[1]) );
    tcp::endpoint file_endpoint( tcp::v4(), std::atoi(argv[2]) );
    Server chat_server( endpoint, file_endpoint );
    boost::this_thread::sleep(boost::posix_time::milliseconds(1000));
    chat_server.run();
    }
    catch( std::exception& e ){
        std::cerr << "Exception: " << e.what() << std::endl;
    }
}
