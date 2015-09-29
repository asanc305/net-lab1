all: myftpclient myftpserver

myftpclient: client.c
	gcc -o myftpclient client.c

myftpserver: server.c
	gcc -o myftpserver server.c

clean:
	rm -f myftpclient myftpserver *.o *~ core
