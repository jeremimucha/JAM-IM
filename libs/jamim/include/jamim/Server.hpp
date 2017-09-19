#ifndef SERVER_HPP_
#define SERVER_HPP_

#include <cstdlib>
#include <unordered_map>
#include <deque>
#include <list>
#include <set>
#include <string>
#include <memory>
#include <utility>
#include <iostream>
#include <boost/asio.hpp>
#include <boost/lexical_cast.hpp>
#include "Message.hpp"


/* ChatParticipant */
/* ------------------------------------------------------------------------- */
class ChatParticipant
{
public:
    ChatParticipant( uint8_t id )
        : id_( id )
        { }

    virtual ~ChatParticipant() { }
    virtual void deliver( const Message& msg ) = 0;
    uint8_t id()
        { return id_; }
    std::string string_id()
        { return std::to_string(id_); }

private:
    const uint8_t         id_;
};

typedef std::shared_ptr< ChatParticipant >  ptr_ChatParticipant;
/* ------------------------------------------------------------------------- */

/* ChatRoom */
/* ------------------------------------------------------------------------- */
class ChatRoom
{
    friend class ChatSession;
public:
    ChatRoom( boost::asio::io_service& io_service )
        : io_strand_( io_service )
        { }
    void join( ptr_ChatParticipant participant );
    void leave( ptr_ChatParticipant participant );
    void deliver( const Message& msg );
    void deliver( const Message& msg, ptr_ChatParticipant sender );

private:
    boost::asio::strand                 io_strand_;
    std::set< ptr_ChatParticipant >     participants_;
};
/* ------------------------------------------------------------------------- */

/* ChatSession */
class ChatSession
    : public ChatParticipant
    , public std::enable_shared_from_this< ChatSession >
{
public:
    typedef void(ChatSession::*Handler)( const boost::system::error_code&
                                       , std::size_t
                                       /* , ptr_ChatParticipant */ );

    ChatSession( boost::asio::io_service& io_service
               , boost::asio::ip::tcp::socket socket
               , ChatRoom& room
               , uint8_t id )
        : ChatParticipant( id )
        , socket_( std::move(socket) )
        , io_strand_( io_service )
        , room_( room )
        { }

    void start();
    void deliver( const Message& msg );

private:
    void do_read_header();
    void handle_read_header( const boost::system::error_code& ec
                           , std::size_t /*length*/
                           /* , ptr_ChatParticipant sender */ );
    void do_read_body();
    void handle_read_body( const boost::system::error_code& ec
                         , std::size_t /*length*/
                         /* , ptr_ChatParticipant sender */ );

    void handle_empty( const boost::system::error_code& ec
                     , std::size_t /*length*/
                     /* , ptr_ChatParticipant sender */ );                     
    // void do_start_file();
    void handle_start_file( const boost::system::error_code& ec
                          , std::size_t /*length*/
                          /* , ptr_ChatParticipant sender */ );
    // void do_cancel_current();
    void handle_cancel_current( const boost::system::error_code& ec
                              , std::size_t /*length*/
                              /* , ptr_ChatParticipant sender */ );
    // void do_cancel_all();
    void handle_cancel_all( const boost::system::error_code& ec
                          , std::size_t /*length*/
                          /* , ptr_ChatParticipant sender */ );
    // void do_quit();
    void handle_quit( const boost::system::error_code& ec
                    , std::size_t /*length*/
                    /* , ptr_ChatParticipant sender */ );

    void do_write();
    void handle_write( const boost::system::error_code& ec
                     , std::size_t /*length*/
                     /* , ptr_ChatParticipant sender */ );

    void handle_unknown( const boost::system::error_code& ec
                       , std::size_t /*length*/
                       /* , ptr_ChatParticipant sender */ );
    
    void handle_error( const boost::system::error_code& ec );

    void close();
private:
    boost::asio::ip::tcp::socket    socket_;
    boost::asio::strand             io_strand_;
    ChatRoom&                       room_;
    Message                         read_msg_;
    std::deque<Message>             write_msg_queue_;

    static const std::unordered_map<MessageType, Handler> s_handler_map_;
};
/* ------------------------------------------------------------------------- */


/* Server */
/* ------------------------------------------------------------------------- */
class Server
{
public:
    Server( const boost::asio::ip::tcp::endpoint& endpoint )
        : acceptor_( io_service_, endpoint )
        , socket_( io_service_ )
        , room_( io_service_ )
        {
            do_accept();
        }

    void run()
        { io_service_.run(); }

private:
    void do_accept();
    void handle_accept( const boost::system::error_code& ec );

private:
    boost::asio::io_service             io_service_;
    boost::asio::ip::tcp::acceptor      acceptor_;
    boost::asio::ip::tcp::socket        socket_;
    ChatRoom                            room_;
};
/* ------------------------------------------------------------------------- */

#endif /* SERVER_HPP_ */
