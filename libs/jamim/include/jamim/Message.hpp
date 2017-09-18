#ifndef MESSAGE_HPP_
#define MESSAGE_HPP_

#include <iostream>
#include <array>
#include <vector>
#include <utility>
#include <boost/asio.hpp>


int dummy();


inline uint16_t make_uint16( uint8_t msb, uint8_t lsb )
{
    return ( ( static_cast<uint16_t>( msb ) << 8 )
             | static_cast<uint16_t>( lsb ) );
}

/* ------------------------------------------------------------------------- */
using boost::uint16_t;
using boost::uint8_t;
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
enum MessageType : uint8_t { ChatMsg = 10, CommandMsg = 20, FileMsg = 30 };
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

    explicit Message( const MessageHeader& header )
        : msg_body_( header.begin(), header.end() )
        , header_( header )
        {
            msg_body_.resize( header.msg_length() + header.length() );
        }

    Message( MessageType type = MessageType::ChatMsg
           , uint16_t len = MessageSize::Empty )
        : Message( MessageHeader(type, len) )
        { }
    

    uint16_t header_length() const
        { return header_.length(); }

    uint16_t body_length() const
        { return header_.msg_length(); }

    void body_length( uint16_t len )
        { 
            header_.msg_length( len );
            msg_body_[MessageHeader::length_msb_offset] = header_.length_msb();
            msg_body_[MessageHeader::length_lsb_offset] = header_.length_lsb();
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
        { return msg_body_.data() + header_.length(); }

    const uint8_t* msg_body() const
        { return msg_body_.data() + header_.length(); }

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
    
/* friends */
    friend std::ostream& operator<<( std::ostream& os, const Message& msg );
    friend Message message_from_string( std::string&& rv );
    
protected:
    Message( std::string&& rv );
private:
    std::vector<uint8_t>    msg_body_;
    MessageHeader           header_;
};
/* ------------------------------------------------------------------------- */



#endif /* MESSAGE_HPP_ */
