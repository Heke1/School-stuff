#include "lottery.h"
#include <iostream>
/*#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>*/
#include "udp_messenger.h"
#include <string.h>


int main (int argc, char* argv[])
{
    using std::cout;
    using std::endl;
    using std::string;

    string lotto, rec, ip, port;
    lottery ltr;

    if(argc != 3) {
        cout<<"Invalid arguments"<<endl;
        return -1;
    }

    ip = argv[1];
    port = argv[2];

    udp_messenger udp(ip, port);
    // server needs to bind to socket.
    udp.bind_to_socket();

    // recieve and respond in a infinite loop.
    // both send and recieve are non-blocking
        while(1) {
          //get new numbers.
            lotto = ltr();
            rec = udp.recieve();

            if(!rec.empty()) {
                udp.send(lotto);
                cout << rec<<endl;
            }
        }

    // I tried this server (with script.sh provided with code)
    // with multiple clients some on same computer and some on other computer in same LAN
    // served every client with correct message for a ~hour (I made it to echo while testing)

    return 0;
}
