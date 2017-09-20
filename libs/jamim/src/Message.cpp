#include "Message.hpp"
#include <string>
#include <unordered_map>
#include <utility>
#include <iterator>

/* ------------------------------------------------------------------------- */
namespace
{
const char COMMAND_INDICATOR = '-';
const std::string CMD_QUIT{ "quit" };
const std::string CMD_START_FILE{ "send" };
const std::string CMD_CANCEL_CURRENT{ "cancel" };
const std::string CMD_CANCEL_ALL{ "cancel-all" };

std::unordered_map<std::string, MessageType> CommandMap{
        { CMD_QUIT              , MessageType::CmdQuit }
    ,   { CMD_START_FILE        , MessageType::CmdStartFile }
    ,   { CMD_CANCEL_CURRENT    , MessageType::CmdCancelCurrent }
    ,   { CMD_CANCEL_ALL        , MessageType::CmdCancelAll }
    };

inline MessageType command_type( std::string&& str );
    
} // namespace
/* ------------------------------------------------------------------------- */

// const std::unordered_map<MessageType,std::string> Message::s_type_string_map_{
//     {Message::EmptyMsg         ,}
//     {Message::ChatMsg          ,}
//     {Message::CmdQuit          ,}
//     {Message::CmdStartFile     ,}
//     {Message::CmdCancelCurrent ,}
//     {Message::CmdCancelAll     ,}
//     {Message::Unknown          ,}
// };

Message message_from_string( const std::string& str )
{
     if( str.empty() ){
        return Message( MessageType::EmptyMsg, MessageSize::Empty );
     }

     if( str.front() == COMMAND_INDICATOR ){
         return command_from_string( str );
     }
     
     return Message( MessageType::ChatMsg, str );
     
}

Message command_from_string( const std::string& str )
{
    std::string::size_type cmd_end = str.find( ' ' ) - 1;    // commands which attach a msg - CmdStartFile
    if( cmd_end == std::string::npos ){  
        cmd_end = str.size()-1;           // non-msg commands
    }

    MessageType cmd_type = command_type( str.substr( 1, cmd_end ) );
    if( cmd_type == MessageType::CmdStartFile ){
        return Message( cmd_type, str.substr( cmd_end+2 ) );
    }
    else if( cmd_type == MessageType::CmdQuit ){
        return Message( cmd_type, QUIT_MSG );
    }
    else{
        return Message( cmd_type, MessageSize::Empty );
    }
    
}

Message make_file_message( uint32_t file_size, const Message& msg )
{
    // MessageHeader header( MessageType::FileMsg, msg.body_length() );
    // std::vector<uint8_t> msgbody(header.begin(), header.end());
    // for( int i=24; i>=0; i-=8 ){
    //     msgbody.push_back( static_cast<uint8_t>(file_size >> i) );
    // }
    // std::copy( msg.msg_body(), msg.msg_body()+msg.body_length()
    //          , std::back_inserter(msgbody) );
            
    // return Message( std::move(msgbody) );
    return make_file_message( file_size, msg.body_to_string() );
}

Message make_file_message( uint32_t file_size, const std::string& str )
{
    MessageHeader header( MessageType::FileMsg, str.size() );
    std::vector<uint8_t> msgbody(header.begin(), header.end());
    for( int i=24; i>=0; i-=8 ){
        msgbody.push_back( static_cast<uint8_t>(file_size >> i) );
    }
    std::copy( msg.msg_body(), msg.msg_body()+msg.body_length()
             , std::back_inserter(msgbody) );
            
    return Message( std::move(msgbody) );
}


Message::Message( MessageType type, const std::string& str )
    : Message( type, str.size() )
{
    std::copy( str.cbegin(), str.cend(), (msg_body_.data() + header_.length()) );
}

Message::Message( std::vector<uint8_t>&& body )
    : msg_body_( std::move( body ) )
    , header_( static_cast<MessageType>(msg_body_[MessageHeader::type_offset])
             , make_uint16(msg_body_[MessageHeader::length_msb_offset]
                          ,msg_body_[MessageHeader::length_lsb_offset]
             ))
{
}


std::ostream& operator<<( std::ostream& os, const Message& msg )
{
    return os << ">>> " 
              << std::string( msg.msg_body(), msg.msg_body()+msg.body_length() );
}


/* ------------------------------------------------------------------------- */
namespace
{

inline MessageType command_type( std::string&& str )
{
    auto it_cmd = CommandMap.find( str );
    if( it_cmd != CommandMap.end() ){
        return it_cmd->second;
    }
    else{
        return MessageType::Unknown;
    }
}
    
} // namespace
/* ------------------------------------------------------------------------- */

int dummy()
{
    return 42;
}
