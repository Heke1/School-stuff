CC = g++

CFLAGS = -Wall -std=c++14 -O3

all: separator server client

server: main_server.o lottery.o udp_messenger.o
		$(CC) $(CFLAGS) -o server main_server.o lottery.o udp_messenger.o

client: main_client.o udp_messenger.o
		$(CC) $(CFLAGS) -o client main_client.o udp_messenger.o

main_client.o: main_client.cpp udp_messenger.h
	 	$(CC) $(CFLAGS) -c main_client.cpp

main_server.o: main_server.cpp lottery.h udp_messenger.h
		$(CC) $(CFLAGS) -c main_server.cpp

udp_messenger.o: udp_messenger.cpp udp_messenger.h
		$(CC) $(CFLAGS) -c udp_messenger.cpp

lottery.o: lottery.cpp lottery.h
	 	$(CC) $(CFLAGS) -c lottery.cpp

clean:  separator
		$(RM) server client main_server.o lottery.o udp_messenger.o main_client.o

separator:
		@printf "\n\n\n\n\n---------------MAKE---------------\n\n\n\n\n"
