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
#include <sys/stat.h>
#include <sys/stat.h>

void syserr(char* msg) { perror(msg); exit(-1); }


int connection(char * args[])
{
  int sockfd, portno, n;
  struct hostent* server;
  struct sockaddr_in serv_addr;

  server = gethostbyname(args[1]);
  if(!server) fprintf(stderr, "ERROR: no such host: %s\n", args[1]) ;
  portno = atoi(args[2]) ;

  sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(sockfd < 0) syserr("can't open socket");
  printf("create socket...\n");

  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr = *((struct in_addr*)server->h_addr);
  serv_addr.sin_port = htons(portno);

  if(connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    syserr("can't connect to server");
  printf("connect...\n");

  return sockfd ;

}

void printlist()
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
	      printf("%s\n", ep->d_name) ;
	    }
	  }
    printf("\n") ;	        
    
    (void) closedir (dp) ;
  }
  else printf ("Couldn't open the dir\n") ;
}

int main(int argc, char* argv[])
{
  int sockfd, n, connected,len, out, total ;
  char buffer [255] ;
  char buffer2 [255] ;
  char *token ;
  char f_len [5] ;
  const char del[2] = " " ;
  FILE *in ;
  struct stat f_stat ;
  struct hostent* server ;

  server = gethostbyname(argv[1]) ;
  
  if(argc != 3) 
  {
    fprintf(stderr, "Usage: %s <hostname> <port>\n", argv[0]);
    return 1;
  }
  sockfd = connection(argv) ;
  connected = 1 ;

  while (connected)
  {
    // get message from console
    printf("PLEASE ENTER MESSAGE: ") ;
    fgets(buffer, 255, stdin) ;
    n = strlen(buffer) ;
    if(n>0 && buffer[n-1] == '\n') buffer[n-1] = '\0';
    strncpy(buffer2, buffer, n) ;
    token = strtok(buffer2, del) ;

    if (strcmp(token, "put") == 0 )
    {
      // get and print name of file
      token = strtok(NULL, del) ;
      printf("upload '%s' to remote server: ", token) ;

      // open the file descriptor and get size
      out = open(token, 0) ;
      if (out <= 0) printf("Error file not found\n") ;
      else
      {
        fstat(out, &f_stat) ;

        // send header with put command file name and size
        strcat(buffer, " ") ;
        sprintf(f_len, "%jd", f_stat.st_size) ;
        strcat(buffer, f_len) ;
        n = send(sockfd, buffer, sizeof(buffer), 0) ;

        // send the file
        n = sendfile(sockfd, out, NULL, f_stat.st_size) ;
        if (n == f_stat.st_size) printf("Successful!\n") ;
        else
        {
          printf("Error sending the file. Please tru again\n") ;
          remove(token) ;
        }
        close (out) ;
      }
    }
    else if (strcmp(token, "get") == 0 )
    {
      token = strtok(NULL, del) ;
      printf("Retrieve '%s' from remote server: ", token) ;
      
      in = fopen(token, "w") ;
      if ( in == NULL ) printf("Error creating file\n") ;
      else
      {
        // send command and file to get and open fd
        n = send(sockfd, buffer, sizeof(buffer), 0) ;
      
        // receive file size
        n = recv(sockfd, buffer, sizeof(buffer), 0) ;
        len = atoi(buffer) ;
        if (len == 0)
        {
          printf("Error file does not exist\n") ; 
          remove(token) ;
        }     
        else
        {        
          //receive and write
          total = 0 ;
          while (len > total)
          {
            n = recv(sockfd, buffer, sizeof(buffer), 0) ;
            fwrite(buffer, sizeof(char), n, in) ; 
            total += n ;
            //printf("Received %d bytes: Left %d bytes\n", n, len) ;
          }
          
          //check 
          if (total == len) printf("Successful !\n") ;
          else
          {
            printf("Error receiving file. Please try again.\n") ;
            remove(token) ;
          }
          fclose(in) ;
        }
      } 
    }   
    else if (strcmp(token, "ls-remote") == 0 )
    {
      printf("Files at server (%s) :\n", server->h_name ) ;
      n = send(sockfd, buffer, sizeof(buffer), 0);

      n = recv(sockfd, buffer, sizeof(buffer), 0) ;
      if(n < 0) printf("Error! Please Try Again\n") ; 
      else buffer[n] = '\0' ;
      printf("%s\n",buffer) ;
    }
    else if (strcmp(token, "ls-local") == 0 )
    {
      printf("Files at client :\n") ;
      printlist() ;
    }
    else if (strcmp(token, "exit") == 0 ) 
    {
      n = send(sockfd, buffer, sizeof(buffer), 0) ;
      printf("Connection to server %s terminated. Bye Now!\n", server->h_name) ;
      connected = 0 ;
    }
    else  printf("Command %s does not exist!\n", buffer) ;
    
  }
  
  close(sockfd);
  return 0;
}
