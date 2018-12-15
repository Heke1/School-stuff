# objective:

Using sockets, make a udp-server that responses to any message sent by a (udp)client with "lottery numbers".

# structure:

Both the client and the server use the same udp_messenger socket wrapper-object, that can recognize if it's in server or client state,
based on how it is used by the creator.

Server uses a lottery object for raffling the numbers to be sent.

# usage:

Build with 'make'.

Both, the client and the server take their parameters in following form:
'./client|./server <IP> <PORT>'

Additionally, client accepts the message to be sent as a third parameter, if not provided, default one will be used. Though, despite the 
value of the message sent, the response of the server will be the same (lottery numbers) in this example.

