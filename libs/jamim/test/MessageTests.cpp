#include "jamim/Message.hpp"
#include <gtest/gtest.h>
#include <algorithm>


namespace
{

class MessageHeaderTest : public ::testing::Test
{
public:
    MessageHeaderTest()
        : mh1_( MessageType::ChatMsg, MessageSize::Default )
        { }

    MessageHeader mh1_;
};

TEST( MessageHeaderConstrucor, DefaultConstructor ){
    MessageHeader mh1( MessageType::ChatMsg, 0 );
    EXPECT_EQ( MessageHeader::HeaderLength, mh1.length() );
    EXPECT_EQ( MessageType::ChatMsg, mh1.msg_type() );
    EXPECT_EQ( 0, mh1.msg_length() );

    MessageHeader mh2( MessageType::ChatMsg, 1234 );
    EXPECT_EQ( MessageHeader::HeaderLength, mh2.length() );
    EXPECT_EQ( MessageType::ChatMsg, mh2.msg_type() );
    EXPECT_EQ( 1234, mh2.msg_length() );

    MessageHeader mh3( MessageType::CommandMsg, 9999 );
    EXPECT_EQ( MessageHeader::HeaderLength, mh3.length() );
    EXPECT_EQ( MessageType::CommandMsg, mh3.msg_type() );
    EXPECT_EQ( 9999, mh3.msg_length() );
}

TEST_F( MessageHeaderTest, lengthChangesCorrectly ){
    EXPECT_EQ( MessageHeader::HeaderLength, mh1_.length() );
    EXPECT_EQ( MessageType::ChatMsg, mh1_.msg_type() );
    EXPECT_EQ( MessageSize::Default, mh1_.msg_length() );

    mh1_.msg_length( 0 );
    EXPECT_EQ( MessageHeader::HeaderLength, mh1_.length() );
    EXPECT_EQ( MessageType::ChatMsg, mh1_.msg_type() );
    EXPECT_EQ( 0, mh1_.msg_length() );

    mh1_.msg_length( 1 );
    EXPECT_EQ( MessageHeader::HeaderLength, mh1_.length() );
    EXPECT_EQ( MessageType::ChatMsg, mh1_.msg_type() );
    EXPECT_EQ( 1, mh1_.msg_length() );

    mh1_.msg_length( 0xFFFF );
    EXPECT_EQ( MessageHeader::HeaderLength, mh1_.length() );
    EXPECT_EQ( MessageType::ChatMsg, mh1_.msg_type() );
    EXPECT_EQ( 0xFFFF, mh1_.msg_length() );
}

TEST_F( MessageHeaderTest, typeChangesCorrectly ){
    EXPECT_EQ( MessageHeader::HeaderLength, mh1_.length() );
    EXPECT_EQ( MessageSize::Default, mh1_.msg_length() );
    EXPECT_EQ( MessageType::ChatMsg, mh1_.msg_type() );

    mh1_.msg_type( MessageType::CommandMsg );
    EXPECT_EQ( MessageHeader::HeaderLength, mh1_.length() );
    EXPECT_EQ( MessageSize::Default, mh1_.msg_length() );
    EXPECT_EQ( MessageType::CommandMsg, mh1_.msg_type() );

    mh1_.msg_type( MessageType::FileMsg );
    EXPECT_EQ( MessageHeader::HeaderLength, mh1_.length() );
    EXPECT_EQ( MessageSize::Default, mh1_.msg_length() );
    EXPECT_EQ( MessageType::FileMsg, mh1_.msg_type() );
}


class MessageTest : public ::testing::Test
{
public:
    MessageTest()
        : mh1_( MessageType::ChatMsg, TestMsg.size() )
        , m1_( mh1_ )
        , m2_( MessageType::ChatMsg, TestMsg.size() )
        { }
    
    virtual void SetUp()
    {
        std::copy( TestMsg.cbegin(), TestMsg.cend(), m1_.msg_body() );
        std::copy( TestMsg.cbegin(), TestMsg.cend(), m2_.msg_body() );
    }
    const std::string TestMsg{ "TEST" };
    MessageHeader mh1_;
    Message m1_;
    Message m2_;
};


TEST_F( MessageTest, Constructors ){
    EXPECT_EQ( mh1_.msg_length(), m1_.body_length() );
    EXPECT_EQ( mh1_.msg_length(), m2_.body_length() );
    EXPECT_EQ( mh1_.msg_type(), m1_.msg_type() );
    EXPECT_EQ( mh1_.msg_type(), m2_.msg_type() );

    EXPECT_EQ( TestMsg, std::string( m1_.msg_body(), m1_.msg_body() + m1_.body_length() ) );
    EXPECT_EQ( TestMsg, std::string( m2_.msg_body(), m2_.msg_body() + m2_.body_length() ) );
}


TEST_F( MessageTest, MessageBody ){
    std::vector<boost::uint8_t> TestData( TestMsg.cbegin(), TestMsg.cend() );
    std::vector<boost::uint8_t> m1_data( m1_.msg_body(), m1_.msg_body() + m1_.body_length() );
    std::vector<boost::uint8_t> m2_data( m2_.msg_body(), m2_.msg_body() + m2_.body_length() );

    EXPECT_EQ( TestData, m1_data );
    EXPECT_EQ( TestData, m2_data );
}

} // namespace
