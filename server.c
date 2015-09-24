#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>

void syserr(char* msg) { perror(msg); exit(-1); }

void printlist(char *list)
{
  DIR *dp ;
  struct dirent *ep ;
  dp = opendir ("./") ;
 
  if (dp != NULL)
  {
	  while (ep = readdir (dp))
	  {
	    if ( (&(ep->d_name)[0])[0] != '.' )
	    {
	      strcat(list, ep->d_name) ;
	      strcat(list, "\n") ;
	    }
	  }
	  (void) closedir (dp) ;
  }
  else printf ("Couldn't open the dir\n");
  
}

int main(int argc, char *argv[])
{
  int sockfd, newsockfd, portno, n, in, out, connected ;
  struct sockaddr_in serv_addr, clt_addr;
  struct stat f_stat ;
  socklen_t addrlen;
  pid_t child ;
  char buffer[255] ;
  char f_name [200] ;
  int len ;
  char *token ;
  char list[1000] ;
  char *message ;
  const char del[2] = " " ;
  
  if(argc != 2) 
  { 
	  fprintf(stderr,"Usage: %s <port>\n", argv[0]);
	  return 1;
  } 
  portno = atoi(argv[1]);

  sockfd = socket(AF_INET, SOCK_STREAM, 0); 
  if(sockfd < 0) syserr("can't open socket"); 
  printf("create socket...\n");

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) 
	syserr("can't bind");
  printf("bind socket to port %d...\n", portno);

  listen(sockfd, 5);

  for(;;) 
  {
	  printf("wait on port %d...\n", portno);
	  addrlen = sizeof(clt_addr); 
	  newsockfd = accept(sockfd, (struct sockaddr*)&clt_addr, &addrlen);
	  if(newsockfd < 0) syserr("can't accept"); 

	  child = fork() ;
	  if (child < 0)
	    printf("Fork error") ;
	  else if (child == 0)
	  {
	    connected = 1 ;
	    while (connected)
	    {
		    printf("Waiting for command\n") ;
		    n = recv(newsockfd, buffer, sizeof(buffer), 0) ;  
		    if(n < 0) syserr("can't receive from client") ; 
		    else buffer[n] = '\0' ;
		    token = strtok(buffer, del) ;	 

		    if (strcmp(token, "put") == 0)
		    {
		      token = strtok(NULL, del) ;
		      in = open(token, O_WRONLY | O_CREAT | O_TRUNC,
		          S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) ;

		      token = strtok(NULL, del) ;
		      len = atoi (token) ;
		      n = recv(newsockfd, buffer, len, 0) ;
		 
		      while (n <= len)
		      {
		        write (in, buffer, n) ; 
		        len = len - n ;
		      }

          close(in) ;
		    }
		    else if (strcmp(token, "get") == 0) 
	    	{
		      // get filename, open file, get size
		      token = strtok(NULL, del) ;
		      out = open(token, 0) ;
		      fstat(out, &f_stat) ;
		
		      // send file size
		      sprintf(buffer, "%jd", f_stat.st_size) ;
		      n = send(newsockfd, buffer, sizeof(buffer), 0) ;

		      // send file
		      n = sendfile(newsockfd, out, NULL, f_stat.st_size) ;
          close(out) ;

		    }
		    else if (strcmp(buffer, "ls-remote") == 0)
		    {
          buffer[0] = '\0' ;
          printlist(buffer) ;
          n = send(newsockfd, buffer, sizeof(buffer), 0) ;
		    }
      }
    }
  }
  close(sockfd); 
  return 0;
}
