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
    using std::getline;


    string send ="Hi server!";
    string rec;

    if(argc < 3 or argc > 4) {
       cout<<"Invalid arguments"<<endl;
        return -1;
    }

    string ip= argv[1];
    string port = argv[2];
    // optional parameter which sets up the message 
    if(argc == 4)
        send = argv[3];


   udp_messenger udp(ip, port);

   // make sure that we get a response from server. Retry if not.
   while (1) {

        if(udp.send(send))
            rec = udp.recieve();

        if(!rec.empty()) {
            cout << rec<<endl;
            break;
        }

    }
    return 0;
}
