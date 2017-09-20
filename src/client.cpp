#include "jamim/Client.hpp"
#include <boost/thread.hpp>
#include <string>
using boost::asio::ip::tcp;


int main( int argc, char* argv[] )
{
    if( argc != 4 ){
        std::cerr << "Usage: client <host> <port1> <port2>" << std::endl;
        return 1;
    }

    try{
        boost::asio::io_service io_service;
        boost::asio::io_service io_file_service;
        tcp::resolver resolver( io_service );
        tcp::resolver::iterator endpoint_iterator = resolver.resolve( {argv[1], argv[2]} );
        tcp::resolver fresolver( io_file_service );
        tcp::resolver::iterator file_endpoint_iterator = fresolver.resolve( {argv[1], argv[3]} );
        
        Client chat( io_service, endpoint_iterator
                   , io_file_service, file_endpoint_iterator );
        boost::thread_group thread_group;
        thread_group.create_thread( [&io_service](){ io_service.run(); } );
        thread_group.create_thread( [&io_file_service](){ io_file_service.run(); } );

        std::string line;
        while( std::getline( std::cin, line ) ){
            Message msg = message_from_string( line );
            switch( msg.msg_type() ){
                case MessageType::CmdStartFile :
                    chat.start_file( msg ); break;
                default:
                    chat.write( msg ); break;
            }
            // chat.write( msg );
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
