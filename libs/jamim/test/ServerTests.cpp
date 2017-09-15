#include "jamim/Server.hpp"
#include <gtest/gtest.h>


namespace
{

TEST(ServerTest, Constructor){
    boost::asio::io_service io_service;
    tcp_server server(io_service);
    SUCCEED();
}

} // namespace
