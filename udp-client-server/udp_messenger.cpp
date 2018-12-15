#include "udp_messenger.h"

udp_messenger::udp_messenger(const string ip_addr,const string port):
_ip_addr(ip_addr),
_port(port)


{

    addrinfo hints;
    //null data
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_UNSPEC; //uspedified, both ipv4 and 6 results may appear
    hints.ai_socktype = SOCK_DGRAM; //datagram, -->udp
    hints.ai_flags = AI_PASSIVE|AI_ADDRCONFIG; //ipv6 only if server has a ipv6 address.


    //this will populate the res_lst with available addresses i.e addrinfo about them.
    int ret = getaddrinfo(_ip_addr.c_str(), _port.c_str(), &hints, &res_lst);

    if ( ret != 0 )
        throw socket_exception("getaddrinfo()");

    //open socket and save it's file descriptor.
    _fd = socket(res_lst->ai_family, res_lst->ai_socktype, res_lst->ai_protocol);
    if ( _fd < 0 )
        throw socket_exception("socket()");


}


udp_messenger::~udp_messenger() {
    //need to free allocated addrinfo struct.
    freeaddrinfo(res_lst);

}

bool udp_messenger::send( const string message)
{
    //zero and (re) set our file descriptor set for writng.
    FD_ZERO(&write_fds);
    FD_SET(_fd, &write_fds);
    //Select allows us to do reading and writing  in non-blocking way.
    // So let's select a set of fd's to listen. First paramater needs to be: ( our largest fd + 1 )
    // read_fds and excep_fds are not passed, last parameter is for timeout (0 means no time out).
    // ...I tried also with timeout..
    // Not a good idea, hogs the entire core. Maybe I did something wrong or timeout isn't for udp.
    int tmp = select(_fd +1, 0, &write_fds, 0, 0);
    if (tmp < 0)
        throw socket_exception("select()");
    if (tmp == 0)
        return false;

    //if there is something to write do so.
	if(FD_ISSET(_fd, &write_fds)) {

        ssize_t count = sendto(_fd,message.c_str(), message.length(),0,res_lst->ai_addr, res_lst->ai_addrlen);

        if( count < 0 or count >1024)
            throw socket_exception("sendto()");
        //clear the fd set.
        FD_CLR(_fd, &write_fds);
        return true;
    }

    return false;
}

string udp_messenger::recieve ()
{
    // same as in write this time for read.
    FD_ZERO(&read_fds);
	FD_SET(_fd, &read_fds);

    int tmp = select(_fd +1, &read_fds,0, 0, 0);
	if(tmp < 0)
        throw socket_exception("select()");
    if(tmp == 0)
        return "";

    if (FD_ISSET(_fd, &read_fds) ) {
        //nul the buffer.
        memset(buffer, 0, 1024);
        ssize_t count = recvfrom (_fd, buffer, 1024, 0, (sockaddr*)&src_addr, &src_addr_len);

        if( count < 0 or count >1024)
            throw socket_exception("recfrom()");

        if(!_client_mode) {
            //On server, res_lst (holding own address) is only needed for binding the socket,
            //after the bind, we can reuse the struct to hold info from last sender!
            res_lst->ai_addr = (sockaddr*)&src_addr;
            res_lst->ai_addrlen = src_addr_len;
        }

        FD_CLR(_fd, &read_fds);

        return  buffer;
    }

    return "";

}

void udp_messenger::bind_to_socket()
{       //bind our socket to listen address defined in res_lst
    if ( bind(_fd, res_lst->ai_addr, res_lst->ai_addrlen) == -1  )
            throw socket_exception("bind()");
    //the object waiting for message before sending one is the only one calling bind, i.e the object is in server mode
    _client_mode = false;

}
