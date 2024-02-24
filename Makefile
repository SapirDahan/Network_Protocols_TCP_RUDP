CC = gcc
CFLAGS = -Wall

.PHONY: clean
all: TCP_Receiver TCP_Sender RUDP_Sender RUDP_Receiver

TCP_Receiver: TCP_Receiver.o
	$(CC) $(CFLAGS) -o TCP_Receiver TCP_Receiver.o

TCP_Receiver.o: TCP_Receiver.c
	$(CC) $(CFLAGS) -c TCP_Receiver.c -o TCP_Receiver.o

TCP_Sender: TCP_Sender.o
	$(CC) $(CFLAGS) -o TCP_Sender TCP_Sender.o

 TCP_Sender.o:  TCP_Sender.c
	$(CC) $(CFLAGS) -c TCP_Sender.c -o TCP_Sender.o


RUDP_Receiver: RUDP_Receiver.o RUDP_API.o
	$(CC) $(CFLAGS) -o RUDP_Receiver RUDP_Receiver.o RUDP_API.o

RUDP_Receiver.o: RUDP_Receiver.c
	$(CC) $(CFLAGS) -c RUDP_Receiver.c -o RUDP_Receiver.o

RUDP_Sender: RUDP_Sender.o RUDP_API.o
	$(CC) $(CFLAGS) -o RUDP_Sender RUDP_Sender.o RUDP_API.o

RUDP_Sender.o: RUDP_Sender.c
	$(CC) $(CFLAGS) -c RUDP_Sender.c -o RUDP_Sender.o

RUDP_API.o: RUDP_API.c
	$(CC) $(CFLAGS) -c RUDP_API.c -o RUDP_API.o

clean:
	rm -f *.o TCP_Sender TCP_Receiver RUDP_Sender RUDP_Receiver