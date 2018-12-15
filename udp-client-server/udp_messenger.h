#pragma once

#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string>
#include <string.h>
#include <exception>
#include <sstream>
#include <algorithm>
#include <unistd.h>



//exception for sockets

struct socket_exception: public std::exception {

public:

    explicit socket_exception( const char* what_arg):_what_arg(what_arg){}
    virtual const char* what() const throw() {
        return _what_arg;
    }
private:
    const char* _what_arg;
};



using std::string;
using std::endl;


//wrapper class for socket communication using udp.
class udp_messenger {

    public:

        udp_messenger(const string ip_addr,const string port);

        ~udp_messenger();

        //send a message false, if failed.
        bool send( const string message);
        //recive message, empty if failed. This also means that you can't recieve empty messages
        string recieve ();
        //binds to socket and sets the object in "server_mode"
        void bind_to_socket();


    private:
        //temporary storage for recieved data when in "server mode"
        sockaddr_storage src_addr;
        socklen_t src_addr_len = sizeof(src_addr);;
        //adress where to send and expect to recieve from.
        //when in server_mode this changes after every message recieved. (if sender is different than previous one)
        addrinfo *res_lst;
        //file descriptor for socket.
        int _fd;

        // buffer recieving
        char buffer[1024];
        //fd sets for select, one for each direction.
        fd_set read_fds, write_fds;
        // if this is false, object is in server_mode, (i.e bound to listen a socket.)
        bool _client_mode = true;

        const string _ip_addr;
        const string _port;


};
