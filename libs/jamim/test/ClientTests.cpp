#include "jamim/Client.hpp"
#include "gtest/gtest.h"


namespace
{

TEST(ClientTest, Constructor){
    Client client("127.0.0.1", "42");
    EXPECT_EQ( true, client.connect() );
}

} // namespace
