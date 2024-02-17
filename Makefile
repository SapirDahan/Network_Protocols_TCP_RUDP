CC = gcc
CFLAGS = -Wall

.PHONY: clean
all: TCP_Receiver TCP_Sender

TCP_Receiver: TCP_Receiver.o
	$(CC) $(CFLAGS) -o TCP_Receiver TCP_Receiver.o

TCP_Receiver.o: TCP_Receiver.c
	$(CC) $(CFLAGS) -c TCP_Receiver.c -o TCP_Receiver.o

TCP_Sender: TCP_Sender.o
	$(CC) $(CFLAGS) -o TCP_Sender TCP_Sender.o

 TCP_Sender.o:  TCP_Sender.c
	$(CC) $(CFLAGS) -c TCP_Sender.c -o TCP_Sender.o

clean:
	rm -f *.o TCP_Sender TCP_Receiver