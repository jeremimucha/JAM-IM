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
#include <boost/thread.hpp>
#include "Message.hpp"


/* ChatParticipant */
/* ------------------------------------------------------------------------- */
class ChatParticipant;
typedef std::shared_ptr< ChatParticipant >  ptr_ChatParticipant;

class ChatParticipant
{
public:
    ChatParticipant( uint8_t id )
        : id_( id )
        { }

    virtual ~ChatParticipant() { }
    virtual void deliver( const Message& msg ) = 0;
    // virtual void file_recieve( const FileTransfer& file ) = 0;
    virtual void file_accepted( const Message& msg
                              , ptr_ChatParticipant sender ) = 0;
    virtual void file_refused( const Message& msg
                             , ptr_ChatParticipant sender ) = 0;
    virtual void file_deliver( const std::vector<char>& data ) = 0;
    virtual void file_msg_deliver( const Message& msg
                                 /* , ptr_ChatParticipant sender */ ) = 0;
    virtual void file_responses_remaining( std::size_t ) = 0;
    uint8_t id() { return id_; }
    std::string string_id() { return std::to_string(id_); }

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
    ChatRoom( boost::asio::io_service& io_service
            , boost::asio::io_service& io_file_service )
        : io_strand_( io_service )
        , io_file_strand_( io_file_service )
        { }

    void join( ptr_ChatParticipant participant );
    void leave( ptr_ChatParticipant participant );
    std::size_t participant_count() const;

    void deliver( const Message& msg );
    void deliver( const Message& msg, ptr_ChatParticipant sender );

    void file_awaiting( const Message& msg, ptr_ChatParticipant sender );
    void file_awaiting_complete( ptr_ChatParticipant sender );
    void file_accept( const Message& msg, ptr_ChatParticipant sender );
    void file_refuse( const Message& msg, ptr_ChatParticipant sender );
    void file_cancel( const Message& msg, ptr_ChatParticipant sender );
    void file_cancel_all( const Message& msg, ptr_ChatParticipant sender );
    void file_done( const Message& msg, ptr_ChatParticipant sender );
    std::size_t file_reader_count( ptr_ChatParticipant sender ) const;
    void file_deliver( const std::vector<char>& data
                     , ptr_ChatParticipant sender );
    void file_msg_deliver( const Message& msg, ptr_ChatParticipant sender );
private:
    boost::asio::strand                         io_strand_;
    boost::asio::strand                         io_file_strand_;
    std::set< ptr_ChatParticipant >             participants_;
    std::set< ptr_ChatParticipant >             response_awaiters_;
    std::unordered_map< ptr_ChatParticipant,
        std::set< ptr_ChatParticipant > >       file_sender_readers_map_;
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

    friend class ChatRoom;

    ChatSession( boost::asio::io_service& io_service
               , boost::asio::io_service& io_file_service
               , boost::asio::ip::tcp::socket socket
               , boost::asio::ip::tcp::socket file_socket
               , ChatRoom& room
               , uint8_t id )
        : ChatParticipant( id )
        , socket_( std::move(socket) )
        , file_socket_( std::move(file_socket) )
        , io_strand_( io_service )
        , io_file_strand_( io_file_service )
        , room_( room )
        { }

    void start();
    void deliver( const Message& msg );
    void file_accepted( const Message& msg, ptr_ChatParticipant sender );
    void file_refused( const Message& msg, ptr_ChatParticipant sender );
    void file_responses_remaining( std::size_t count );
    void file_deliver( const std::vector<char>& data );
    void file_msg_deliver( const Message& msg
                         /* , ptr_ChatParticipant sender */ );

private:
/* general communication */
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

    void handle_quit( const boost::system::error_code& ec
                    , std::size_t /*length*/
                    /* , ptr_ChatParticipant sender */ );

    void handle_unknown( const boost::system::error_code& ec
                       , std::size_t /*length*/
                       /* , ptr_ChatParticipant sender */ );

    void do_write();
    void handle_write( const boost::system::error_code& ec
                     , std::size_t /*length*/
                     /* , ptr_ChatParticipant sender */ );
    
    void handle_error( const boost::system::error_code& ec );


/* file transfer */
    void handle_file_start( const boost::system::error_code& ec
                          , std::size_t /*length*/ );

    void handle_file_accept( const boost::system::error_code& ec
                           , std::size_t /*length*/ );

    void handle_file_refuse( const boost::system::error_code& ec
                           , std::size_t /*length*/ );

    void handle_file_cancel( const boost::system::error_code& ec
                              , std::size_t /*length*/ );

    void handle_file_cancel_all( const boost::system::error_code& ec
                          , std::size_t /*length*/ );

    void handle_file_done( const boost::system::error_code& ec
                         , std::size_t /*length*/ );

    void do_file_send_start();
    void handle_file_send_start( const boost::system::error_code& ec
                               , std::size_t bytes_transferred );
    void do_file_send();
    void handle_file_send( const boost::system::error_code& ec
                         , std::size_t bytes_transferred );

    void do_file_read();
    void handle_file_read( const boost::system::error_code& ec
                         , std::size_t bytes_transferred );

    void do_file_cancel();

    void handle_file_error( const boost::system::error_code& ec );

    void close();
private:
    boost::asio::ip::tcp::socket        socket_;
    boost::asio::ip::tcp::socket        file_socket_;
    boost::asio::strand                 io_strand_;
    boost::asio::strand                 io_file_strand_;
    ChatRoom&                           room_;
    Message                             read_msg_;
    std::deque<Message>                 write_msg_queue_;

    Message                             file_msg_;
    std::set< ptr_ChatParticipant >     file_recievers_;
    std::size_t                         file_responses_remaining_;
    std::array<char,4096>               file_send_buf_;
    // std::deque< std::vector<char> >     file_send_queue_;
    // std::array<char,4096>               file_read_buf_;
    std::deque< std::vector<char> >     file_read_queue_;
    bool                                recieving_file_;

    static const std::unordered_map<MessageType, Handler> s_handler_map_;
};
/* ------------------------------------------------------------------------- */


/* Server */
/* ------------------------------------------------------------------------- */
class Server
{
public:
    Server( const boost::asio::ip::tcp::endpoint& endpoint 
          , const boost::asio::ip::tcp::endpoint& file_endpoint )
        : acceptor_( io_service_, endpoint )
        , file_acceptor_( io_service_, file_endpoint )
        , socket_( io_service_ )
        , file_socket_( io_service_ )
        , room_( io_service_, io_file_service_ )
        {
            // run();
            do_file_accept();
            do_accept();
        }

    void run()
        { 
            thread_group.create_thread( [this](){ io_service_.run(); } );
            thread_group.create_thread( [this](){ io_file_service_.run(); } );
        }

    ~Server()
        { thread_group.join_all(); }

private:
    void do_accept();
    void handle_accept( const boost::system::error_code& ec );

    void do_file_accept();
    void handle_file_accept( const boost::system::error_code& ec );

private:
    boost::asio::io_service             io_service_;
    boost::asio::io_service             io_file_service_;
    boost::asio::ip::tcp::acceptor      acceptor_;
    boost::asio::ip::tcp::acceptor      file_acceptor_;
    boost::asio::ip::tcp::socket        socket_;
    boost::asio::ip::tcp::socket        file_socket_;
    boost::thread_group                 thread_group;
    ChatRoom                            room_;
};
/* ------------------------------------------------------------------------- */

#endif /* SERVER_HPP_ */
