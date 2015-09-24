all:	client server

tcpclient: client.c
	gcc -Wall $< -o $@

tcpserver: server.c
	gcc -Wall $< -o $@

clean:
	rm -f client server *.o *~ core
