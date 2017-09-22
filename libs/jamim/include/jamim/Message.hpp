#ifndef MESSAGE_HPP_
#define MESSAGE_HPP_

#include <iostream>
#include <array>
#include <vector>
#include <string>
#include <utility>
#include <boost/asio.hpp>


int dummy();


inline uint16_t make_uint16( uint8_t msb, uint8_t lsb )
{
    return ( ( static_cast<uint16_t>( msb ) << 8 )
             | static_cast<uint16_t>( lsb ) );
}

inline uint32_t make_uint32( uint8_t b31_24, uint8_t b23_16
                           , uint8_t b15_8, uint8_t b7_0 )
{
    return ( (static_cast<uint16_t>(b31_24) << 24 )
           | (static_cast<uint16_t>(b23_16) << 16 )
           | (static_cast<uint16_t>(b15_8)  <<  8 )
           | (static_cast<uint16_t>(b7_0) ) );
}

static const std::string QUIT_MSG{ "User has left the room." };
static const std::string CANCEL_CURRENT_MSG{ "File transfer cancelled by the user." };
static const std::string CANCEL_ALL_MSG{ "Files transfer cancelled by the user." };

/* ------------------------------------------------------------------------- */
using boost::uint32_t;
using boost::uint16_t;
using boost::uint8_t;
using boost::int8_t;
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
enum MessageType : uint8_t { EmptyMsg         = 0
                           , ChatMsg          = 10
                           , CmdQuit          = 20
                           , CmdStartFile     = 30
                           , CmdCancelCurrent = 40
                           , CmdCancelAll     = 41
                           , FileMsg          = 60
                           , FileAccept       = 61
                           , FileRefuse       = 62
                           , FileCancel       = 63
                           , FileCancelAll    = 64
                           , FileDone         = 65
                           , Unknown          = 255
                           };
enum MessageSize : uint16_t { Empty = 0, Default = 4096 };
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
class MessageHeader
{
public:
    enum { type_offset = 0, length_msb_offset = 1, length_lsb_offset = 2 };
    enum { HeaderLength = 3 };

    typedef std::array<uint8_t,HeaderLength>                    self;
    typedef std::array<uint8_t,HeaderLength>::iterator          iterator;
    typedef std::array<uint8_t,HeaderLength>::const_iterator    const_iterator;


    MessageHeader( MessageType type = MessageType::ChatMsg
                 , uint16_t len = MessageSize::Empty )
        {
            header_[type_offset] = type;
            header_[length_msb_offset] = static_cast<uint8_t>(len >> 8);
            header_[length_lsb_offset] = static_cast<uint8_t>(len);
        }

    MessageHeader& operator=( std::pair<MessageType,uint16_t> type_len )
        {
            using std::swap;
            MessageHeader temp(type_len.first, type_len.second);
            swap( header_, temp.header_);
            return *this;
        }

    size_t length() const
        { return HeaderLength; }

    self& data()
        { return header_; }
    const self& data() const
        { return header_; }

    MessageType msg_type() const
        { return static_cast<MessageType>(header_[type_offset]); }

    void msg_type( MessageType type )
        { header_[type_offset] = type; }

    uint16_t msg_length() const
    {
        return make_uint16( header_[length_msb_offset]
                          , header_[length_lsb_offset] );
    }

    void msg_length( uint16_t len )
    {
        header_[length_msb_offset] = static_cast<uint8_t>(len >> 8);
        header_[length_lsb_offset] = static_cast<uint8_t>(len);
    }

    uint8_t length_msb() const
        { return header_[length_msb_offset]; }
    uint8_t length_lsb() const
        { return header_[length_lsb_offset]; }
    
    iterator begin()
      { return header_.begin(); }
    iterator end()
        { return header_.end(); }
    const_iterator begin() const
        { return header_.cbegin(); }
    const_iterator end() const
        { return header_.cend(); }
    const_iterator cbegin() const
        { return header_.cbegin(); }
    const_iterator cend() const
        { return header_.cend(); }


private:
    std::array<uint8_t,HeaderLength>    header_;
};
/* ------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------- */
class Message
{
public:
    enum { filesize_offset_31=3, filesize_offset_23=4, filesize_offset_15=5,
           filesize_offset_7=6, FileMsgHeaderSize=7};

    explicit Message( const MessageHeader& header )
        : msg_body_( header.begin(), header.end() )
        , header_( header )
        {
            msg_body_.resize( header.msg_length() + header.length() );
        }

    explicit Message( MessageHeader&& header )
        : msg_body_( header.begin(), header.end() )
        , header_( std::move(header) )
        {
            msg_body_.resize( header.msg_length() + header.length() );
        }

    Message( MessageType type = MessageType::ChatMsg
           , uint16_t len = MessageSize::Empty )
        : Message( MessageHeader(type, len) )
        { }
    

    uint16_t header_length() const
        { 
            if( msg_type() == FileMsg )
                return FileMsgHeaderSize;
            else
                return header_.length();
        }

    uint16_t body_length() const
        { return header_.msg_length(); }

    void body_length( uint16_t len )
        { 
            header_.msg_length( len );
            msg_body_[MessageHeader::length_msb_offset] = header_.length_msb();
            msg_body_[MessageHeader::length_lsb_offset] = header_.length_lsb();
        }

    uint32_t file_size() const
        {
            // only valid for FileMsg
            return make_uint32( msg_body_[filesize_offset_31]
                              , msg_body_[filesize_offset_23]
                              , msg_body_[filesize_offset_15]
                              , msg_body_[filesize_offset_7] );
        }

    uint16_t total_length() const
        { return msg_body_.size(); }

    MessageType msg_type() const
        { return header_.msg_type(); }

    void msg_type( MessageType t )
        { 
            header_.msg_type( t );
            msg_body_[MessageHeader::type_offset] = t;
        }

    uint8_t* msg_body()
        { return msg_body_.data() + header_length(); }

    const uint8_t* msg_body() const
        { return msg_body_.data() + header_length(); }

    uint8_t* data()
        {
            return msg_body_.data();
        }

    const uint8_t* data() const
        {
            return msg_body_.data();
        }
    
    void sync()
        {
            header_ = MessageHeader( static_cast<MessageType>(msg_body_[MessageHeader::type_offset])
                                  , make_uint16(msg_body_[MessageHeader::length_msb_offset]
                                               ,msg_body_[MessageHeader::length_lsb_offset]
                                               ));
            msg_body_.resize( header_.length() + header_.msg_length() );
        }

    std::string data_to_string() const
        { return std::string(msg_body_.cbegin(), msg_body_.cend() ); }
    std::string body_to_string() const
        { return std::string( msg_body(), msg_body() + body_length() ); }
    
/* friends */
    friend std::ostream& operator<<( std::ostream& os, const Message& msg );
    friend Message message_from_string( const std::string& str );
    friend Message command_from_string( const std::string& str );
    friend Message make_file_message( uint32_t file_size, const Message& msg );
    friend Message make_file_message( uint32_t file_size
                                    , const std::string& str );
protected:
    Message( MessageType type, const std::string& str );
    explicit Message( std::vector<uint8_t>&& body );
private:
    std::vector<uint8_t>    msg_body_;
    MessageHeader           header_;

    // static const std::unordered_map<MessageType,std::string> s_type_string_map_;
};
/* ------------------------------------------------------------------------- */

Message message_from_string( const std::string& str );
Message command_from_string( const std::string& str );
Message make_file_message( uint32_t file_size, const Message& msg );
Message make_file_message( uint32_t file_size, const std::string& str );

#endif /* MESSAGE_HPP_ */
