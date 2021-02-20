
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>

 unsigned short pcc_total[127];
 unsigned short pcc_total_aux[127]; // aux data structure makes sure our data structure is consistent- updated after finished handling cient
 int signaled=0;
 void handler(int signum) { signaled=1; }
int main(int argc, char *argv[])
{

   if (argc != 2) {

        printf(" not enough  or too many args\n");

        exit(1);

    }
  int loop=0;


  int listenfd  = -1;
  int connfd    = -1;
  int i=0;
 for(i=32;i<127;i++){
    pcc_total_aux[i]=0;
    pcc_total[i]=0;
}

  struct sockaddr_in serv_addr;
  struct sockaddr_in my_addr;
  struct sockaddr_in peer_addr;
  socklen_t addrsize = sizeof(struct sockaddr_in );
  struct sigaction myaction;

  memset(&myaction, 0, sizeof(myaction));

    // assign

  myaction.sa_handler = handler;

    if (sigaction(SIGINT, &myaction, NULL) != 0){

        printf("Signal handler reg failed %s\n", strerror(errno));

        exit(1);

    }

 
  



  int port = atoi(argv[1]);

  listenfd = socket( AF_INET, SOCK_STREAM, 0 );
  int opt=1;
  setsockopt(listenfd, SOL_SOCKET  , SO_REUSEADDR , &opt , sizeof(opt));
  memset( &serv_addr, 0, addrsize );

  serv_addr.sin_family = AF_INET;
  // INADDR_ANY = any local machine address
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(port);

  if( 0 != bind( listenfd,
                 (struct sockaddr*) &serv_addr,
                 addrsize ) )
  {
    printf("\n Error : Bind Failed. %s \n", strerror(errno));
    return 1;
  }

  if( 0 != listen( listenfd, 10 ) )
  {
    printf("\n Error : Listen Failed. %s \n", strerror(errno));
    return 1;
  }
 
  while( 1 )
  {
    loop=0;
    uint32_t length;
    unsigned long printable=0;
  
    // Accept a connection.
 

    connfd = accept( listenfd,
                     (struct sockaddr*) &peer_addr,
                     &addrsize);


    if( connfd < 0 )
    {
      
       if(signaled==1){ // if there was a signal which caused accept to fail
        for (i=32; i<= 126; i++) {

         printf("char ’%c’ : %u times\n",i,pcc_total[i]);




    }
     return 0;

    }
    else{  printf("\n Error : Accept Failed. %s \n", strerror(errno)); return 1;}
    }

    getsockname(connfd, (struct sockaddr*) &my_addr,   &addrsize);
    getpeername(connfd, (struct sockaddr*) &peer_addr, &addrsize);
  


   

  char *bytes1 = (char*)&length;
  int notwritten = sizeof(length);

  int bytes_read;
  while (notwritten > 0) // read N from client
  {
    bytes_read = read(connfd, bytes1, notwritten);
      
    if (bytes_read <= 0)
    {
      if (errno == ETIMEDOUT ||errno == ECONNRESET ||errno == EPIPE)
      {
        perror("failed sending to client ");
        loop=1;
        break;
      }
      else
      {
        perror("we need to exit ");

       return 1;
      }
    }
    else
    {
      bytes1 +=   bytes_read;
      notwritten -=   bytes_read;
    }
  }
  if(loop==1)continue;
  
  length = ntohl(length);
 

  
  char* message= malloc(length);

  notwritten=length;

  // read message from client into a buffer
   while( notwritten>0 )
  {
   
    bytes_read = read(  connfd,
                      message,
                      notwritten );
       
    if (bytes_read < 0) // read failed
    {
      if (errno == ETIMEDOUT ||errno == ECONNRESET ||errno == EPIPE)
      {
        perror("failed sending to client ");
        
        loop=1;
       break;
      }
      else
      {
        perror("we need to exit ");

        return 1;
      }
    }
    else{
    message +=  bytes_read;
    notwritten -=  bytes_read;
    }

  }
  if(loop==1)continue; // if there was one of thoose 3 errors, go back to accept
  message-=length;


printable=0;
 for (i=0; i< length; i++) {// go over msg and count printable chars , update aux data structure

     if(message[i] >= 32 && message[i] <= 126) {
           
        pcc_total_aux[(int)(message[i])]++;

        printable++;

     }
 }



    // write printable to client , keep looping until nothing left to write
  uint32_t temp = htonl(printable);
  char *bytes = (char*)&temp;
 notwritten = sizeof(uint32_t);
  int bytes_written;

  while (notwritten > 0)
  {
    bytes_written = write(  connfd, bytes, notwritten);
    if (bytes_written < 0)
    {
     if (errno == ETIMEDOUT ||errno == ECONNRESET ||errno == EPIPE)
      {
        perror("failed sending to client ");
        loop=1;
        break;
      }
      else
      {
        perror("we need to exit ");

        return 1;
      }
    }
    else
    {
      bytes += bytes_written;
      notwritten -= bytes_written;
    }

  }

    if(loop==1) continue;
    for (i=32; i<= 126; i++) { // uptade real data structure

     pcc_total[i]=pcc_total_aux[i];


    }
    if(signaled==1){
        for (i=32; i<= 126; i++) {

         printf("char ’%c’ : %u times\n",i,pcc_total[i]);


    }
     return 0;

    }
    free(message);
    // close socket
    close(connfd);
    
  }
}
