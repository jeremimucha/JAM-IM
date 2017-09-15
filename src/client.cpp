#include "jamim/Client.hpp"


int main( int argc, char* argv[] )
{
    if( argc != 2 ){
        std::cerr << "Usage: client <host>" << std::endl;
        return 1;
    }

    try{
        Client client( argv[1], "daytime" );
        client.connect();
        client.read();
    }
    catch(  std::exception& e ){
        std::cerr << e.what() << std::endl;
    }
}
