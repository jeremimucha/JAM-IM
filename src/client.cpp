#include "jamim/Client.hpp"
#include <boost/thread.hpp>
#include <string>
using boost::asio::ip::tcp;


int main( int argc, char* argv[] )
{
    if( argc != 3 ){
        std::cerr << "Usage: client <host> <port1>" << std::endl;
        return 1;
    }

    try{
        boost::asio::io_service io_service;
        tcp::resolver resolver( io_service );
        tcp::resolver::iterator endpoint_iterator = resolver.resolve( {argv[1], argv[2]} );
        
        Client chat( io_service, endpoint_iterator );
        boost::thread_group thread_group;
        thread_group.create_thread( [&io_service](){ io_service.run(); } );

        std::string line;
        while( std::getline( std::cin, line ) ){
            Message msg = message_from_string( line );
            chat.write( msg );
            if( MessageType::CmdQuit == msg.msg_type() ){
                break;
            }
        }

        chat.close();
        thread_group.join_all();
    }
    catch(  std::exception& e ){
        std::cerr << e.what() << std::endl;
    }
}
