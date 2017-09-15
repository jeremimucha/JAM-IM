#include "Client.hpp"


Client::Client( const char* ip, const char* service )
    : io_service_()
    , resolver_( io_service_ )
    , socket_( io_service_ )
{
    tcp::resolver::query query( ip, service );
    endpoint_it_ = resolver_.resolve(query);
}

bool Client::connect()
{
    boost::asio::connect( socket_, endpoint_it_ );
    return socket_.is_open();
}

void Client::read()
{
    for(;;){
        boost::array<char,128> buf;
        boost::system::error_code error;
        size_t len = socket_.read_some( boost::asio::buffer(buf), error );

        if( error == boost::asio::error::eof )
            return;
        else if( error )
            throw boost::system::system_error( error );

        std::cout.write( buf.data(), len );
    }
}
