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

    MessageHeader mh3( MessageType::CmdStartFile, 9999 );
    EXPECT_EQ( MessageHeader::HeaderLength, mh3.length() );
    EXPECT_EQ( MessageType::CmdStartFile, mh3.msg_type() );
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

    mh1_.msg_type( MessageType::CmdStartFile );
    EXPECT_EQ( MessageHeader::HeaderLength, mh1_.length() );
    EXPECT_EQ( MessageSize::Default, mh1_.msg_length() );
    EXPECT_EQ( MessageType::CmdStartFile, mh1_.msg_type() );

    mh1_.msg_type( MessageType::CmdCancelAll );
    EXPECT_EQ( MessageHeader::HeaderLength, mh1_.length() );
    EXPECT_EQ( MessageSize::Default, mh1_.msg_length() );
    EXPECT_EQ( MessageType::CmdCancelAll, mh1_.msg_type() );
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


TEST( command_from_string_Test, UnknownCommand ){
    MessageType unknown_msg_type{ MessageType::Unknown };
    const std::string un1_{"-"};
    const std::string un2_{"- "};
    const std::string un3_{"-abc"};
    const std::string un4_{"-quitabc"};
    const std::string un5_{"-sendabc/def"};

    EXPECT_EQ( unknown_msg_type, command_from_string(un1_).msg_type() );
    EXPECT_EQ( unknown_msg_type, command_from_string(un2_).msg_type() );
    EXPECT_EQ( unknown_msg_type, command_from_string(un3_).msg_type() );
    EXPECT_EQ( unknown_msg_type, command_from_string(un4_).msg_type() );
    EXPECT_EQ( unknown_msg_type, command_from_string(un5_).msg_type() );
}

TEST( command_from_string_Test, quitCommand ){
    MessageType quit_command{ MessageType::CmdQuit };
    const std::string quit1_{"-quit"};
    const std::string quit2_{"-quit "};
    const std::string quit3_{"-quit ignored/input"};

    Message cmd1_ = command_from_string(quit1_);
    Message cmd2_ = command_from_string(quit2_);
    Message cmd3_ = command_from_string(quit3_);

    EXPECT_EQ( quit_command, cmd1_.msg_type() );
    EXPECT_EQ( quit_command, cmd2_.msg_type() );
    EXPECT_EQ( quit_command, cmd3_.msg_type() );
    
    EXPECT_EQ( QUIT_MSG, std::string( cmd1_.msg_body(), cmd1_.msg_body()
                                    + cmd1_.body_length() ) );
    EXPECT_EQ( QUIT_MSG, std::string( cmd2_.msg_body(), cmd2_.msg_body()
                                    + cmd2_.body_length() ) );
    EXPECT_EQ( QUIT_MSG, std::string( cmd3_.msg_body(), cmd3_.msg_body()
                                    + cmd3_.body_length() ) );
}

TEST( command_from_string_Test, sendCommand ){
    MessageType send_command{ MessageType::CmdStartFile };
    const std::string send1_{"-send"};
    const std::string send2_{"-send "};
    const std::string send3_{"-send /some/directory/to/file"};

    EXPECT_EQ( send_command, command_from_string(send1_).msg_type() );
    EXPECT_EQ( send_command, command_from_string(send2_).msg_type() );
    EXPECT_EQ( send_command, command_from_string(send3_).msg_type() );
}

TEST( command_from_string_Test, cancelCommand ){
    MessageType cancel_file_command{ MessageType::CmdCancelCurrent };
    const std::string cancel1_{"-cancel"};
    const std::string cancel2_{"-cancel "};
    const std::string cancel3_{"-cancel ignored/input"};

    EXPECT_EQ( cancel_file_command, command_from_string(cancel1_).msg_type() );
    EXPECT_EQ( cancel_file_command, command_from_string(cancel2_).msg_type() );
    EXPECT_EQ( cancel_file_command, command_from_string(cancel3_).msg_type() );
}

TEST( command_from_string_Test, cancelAllCommand ){
    MessageType cancel_all_command{ MessageType::CmdCancelAll };
    const std::string cancelall1_{"-cancel-all"};
    const std::string cancelall2_{"-cancel-all "};
    const std::string cancelall3_{"-cancel-all ignored/input"};

    EXPECT_EQ( cancel_all_command, command_from_string(cancelall1_).msg_type() );
    EXPECT_EQ( cancel_all_command, command_from_string(cancelall2_).msg_type() );
    EXPECT_EQ( cancel_all_command, command_from_string(cancelall3_).msg_type() );
}


TEST( message_from_string_Test, emptyLineTest ){
    MessageType empty_msg_type{ MessageType::EmptyMsg };
    uint16_t expected_msg_size{ 0 };
    const std::string empty_line{""};
    
    Message test_msg = message_from_string(empty_line);

    EXPECT_EQ( empty_msg_type, test_msg.msg_type() );
    EXPECT_EQ( expected_msg_size, test_msg.body_length() );
    EXPECT_EQ( MessageHeader::HeaderLength, test_msg.total_length() );
}

TEST( message_from_string_Test, chatMsgTest ){
    MessageType chat_msg_type{ MessageType::ChatMsg };
    const std::string line{"test line"};
    uint16_t expected_msg_size = static_cast<uint16_t>(line.size());

    Message test_msg = message_from_string( line );

    EXPECT_EQ( chat_msg_type, test_msg.msg_type() );
    EXPECT_EQ( expected_msg_size, test_msg.body_length() );
    EXPECT_EQ( line, std::string( test_msg.msg_body(), test_msg.msg_body() 
                                  + test_msg.body_length()) );
}

TEST( message_from_string_Test, sendMsgTest ){
    MessageType send_file_type{ MessageType::CmdStartFile };
    const std::string file_dir{"/some/file/directory/"};
    const std::string command{"-send"};
    const std::string line("-send /some/file/directory/");
    uint16_t expected_msg_size{ file_dir.size() };

    Message test_msg = message_from_string( line );
    EXPECT_EQ( send_file_type, test_msg.msg_type() );
    EXPECT_EQ( expected_msg_size, test_msg.body_length() );
    EXPECT_EQ( file_dir, std::string( test_msg.msg_body(), test_msg.msg_body() 
                                      + test_msg.body_length() ) );
}

TEST( message_from_string_Test, cmdTest ){
    const std::string cmd_quit{ "-quit" };
    const std::string cmd_cancel{ "-cancel" };
    const std::string cmd_cancelall{ "-cancel-all" };
    uint16_t expected_msg_size( 0 );

    Message msg_quit = message_from_string( cmd_quit );
    Message msg_cancel = message_from_string( cmd_cancel );
    Message msg_cancelall = message_from_string( cmd_cancelall );


    EXPECT_EQ( MessageType::CmdQuit, msg_quit.msg_type() );
    EXPECT_EQ( QUIT_MSG.size(), msg_quit.body_length() );
    
    EXPECT_EQ( MessageType::CmdCancelCurrent, msg_cancel.msg_type() );
    EXPECT_EQ( expected_msg_size, msg_cancel.body_length() );

    EXPECT_EQ( MessageType::CmdCancelAll, msg_cancelall.msg_type() );
    EXPECT_EQ( expected_msg_size, msg_cancelall.body_length() );
}

TEST( message_from_string_Test, unknownCmdTest ){
    MessageType unknown_msg_type{ MessageType::Unknown };
    const std::string un1_{"-"};
    const std::string un2_{"- "};
    const std::string un3_{"-abc"};
    const std::string un4_{"-quitabc"};
    const std::string un5_{"-sendabc/def"};

    EXPECT_EQ( unknown_msg_type, message_from_string(un1_).msg_type() );
    EXPECT_EQ( unknown_msg_type, message_from_string(un2_).msg_type() );
    EXPECT_EQ( unknown_msg_type, message_from_string(un3_).msg_type() );
    EXPECT_EQ( unknown_msg_type, message_from_string(un4_).msg_type() );
    EXPECT_EQ( unknown_msg_type, message_from_string(un5_).msg_type() );
}

} // namespace
