#define _DEFAULT_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <assert.h>

int main(int argc, char *argv[]) // credit fo read and send funcs: https://stackoverflow.com/questions/9140409/transfer-integer-over-a-socket-in-c
{
    if (argc != 4) {

        printf(" not enough orr too many args\n");

        exit(1);

    }
  int  sockfd     = -1;
  uint32_t bytes_read;
  uint32_t printable;
  uint32_t printable_res;

int file;
int nsent=-1;
  

  struct sockaddr_in serv_addr; // where we Want to get to
  struct sockaddr_in my_addr;   // where we actually connected through 
  struct sockaddr_in peer_addr; // where we actually connected to
  socklen_t addrsize = sizeof(struct sockaddr_in );

  unsigned short port = (unsigned short) atoi(argv[2]);
 inet_pton(AF_INET, argv[1], &serv_addr.sin_addr);


  if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    printf("\n Error : Could not create socket \n");
    return 1;
  }

  getsockname(sockfd,
              (struct sockaddr*) &my_addr,
              &addrsize);


  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port); // Note: htons for endiannes
  

  
  // Note: what about the client port number?
  // connect socket to the target address
  if( connect(sockfd,
              (struct sockaddr*) &serv_addr,
              sizeof(serv_addr)) < 0)
  {
    printf("\n Error : Connect Failed. %s \n", strerror(errno));
    return 1;
  }
  file = open(argv[3], O_RDONLY);

  if(file == -1)

  {

    printf("failed to open the file - %s\n", strerror(errno));

    return 1;

  }

  
  getsockname(sockfd, (struct sockaddr*) &my_addr,   &addrsize);
  getpeername(sockfd, (struct sockaddr*) &peer_addr, &addrsize);
 


  
  uint32_t file_size;
  struct stat status;
  // calculating the size of the file 
 fstat(file,&status);
 file_size =status.st_size ; 
 
  char* message= (char*)calloc(file_size, sizeof(char));
  uint32_t temp = htonl(file_size);
  char *data = (char*)&temp;
   int notwritten = sizeof(temp);
 
  
    // keep looping until nothing left to write- write file size to server
    while( notwritten > 0 )
    {
      // notwritten = how much we have left to write
      // totalsent  = how much we've written so far
      // nsent = how much we've written in last write() call */
      nsent = write(sockfd,
                    data ,
                    notwritten);
      // check if error occured (client closed connection?)
      if (nsent < 0) {
            if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
                printf("EAGAIN or EWOULDBLOCK \n");
                exit(1);
            }
            else if (errno != EINTR) {
                printf("einter\n");
                exit(1);
            }
        }
      
      else{
      data  += nsent;
      notwritten -= nsent;
      }
    }

 
  uint32_t total_read=0;
   while( 1 ) // read message to a buffer
  {
    bytes_read = read(file,
                      message+total_read,
                      file_size -total_read );
     if( bytes_read < 0 ){
     perror("error happend\n");
     exit(1);
     }
      
    if( bytes_read ==0 )
      break;
    total_read += bytes_read;
   
  }


  // write file to server
bytes_read=0;
  notwritten=file_size;


  // keep looping until nothing left to write
    while( notwritten > 0 )
    {
      // notwritten = how much we have left to write
      // totalsent  = how much we've written so far
      // nsent = how much we've written in last write() call */
      nsent = write(sockfd,
                    message ,
                    notwritten);
     
      if(nsent<0){perror("error happend  \n");  exit(1);}
 
      message  += nsent;
      notwritten -= nsent;
    }

  
  // read data from server into prinatable

   
  char *bytes = (char*)&printable;
  notwritten = sizeof(uint32_t);
  
  while (notwritten > 0)
  {
    bytes_read = read(sockfd, bytes, notwritten);
    if (bytes_read < 0) 
    {
      perror("failed reading from server ");
      exit(1);
    }
    else 
    {
      bytes +=   bytes_read;
      notwritten -=   bytes_read;
    }
  }
  
   printable_res = ntohl(printable);
  printf("# of printable characters: %u\n",printable_res);
  close(file);
  close(sockfd); // is socket really done here?
  
  return 0;
}
