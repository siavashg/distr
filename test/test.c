/* Client code in C */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv)
{
  struct sockaddr_in stSockAddr;
  int Res;
  int port;
  int n;
  int i;
  int socketHolder[1024];

  if (argc < 3) {
    printf ("Usage: %s port connections", argv[0]);
    return -1;
  }

  port = atoi(argv[1]);
  n = atoi(argv[2]);  

  memset(&stSockAddr, 0, sizeof(struct sockaddr_in));

  stSockAddr.sin_family = AF_INET;
  stSockAddr.sin_port = htons(port);

  for (i = 0; i < n; i++) {
	  char buf[1024];

	  snprintf(buf, 1024, "INIT user%d %d\n", i, i);

	  int SocketFD = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	  socketHolder[i] = SocketFD;
	  Res = inet_pton(AF_INET, "127.0.0.1", &stSockAddr.sin_addr);

	  if (0 > Res)
	  {
	    perror("error: first parameter is not a valid address family");
	    close(SocketFD);
	    exit(EXIT_FAILURE);
	  }
	  else if (0 == Res)
	  {
	    perror("char string (second parameter does not contain valid ipaddress");
	    close(SocketFD);
	    exit(EXIT_FAILURE);
	  }


	  if (-1 == SocketFD)
	  {
	    perror("cannot create socket");
	    exit(EXIT_FAILURE);
	  }


	  if (-1 == connect(SocketFD, (const struct sockaddr *)&stSockAddr, sizeof(struct sockaddr_in)))
	  {
	    perror("connect failed");
	    close(SocketFD);
	    exit(EXIT_FAILURE);
	  }

	  write(SocketFD, buf, strlen(buf));

  }


  for (i = 0; i < n; i++) {
	  write(socketHolder[i], "MSG Hej\n", 8);
	  shutdown(socketHolder[i], SHUT_RDWR);
	  close(socketHolder[i]);
  }

 /*sleep(10); 

  for (i = 0; i < n; i++) {
	  write(socketHolder[i], "exit\n", 5);
	  shutdown(socketHolder[i], SHUT_RDWR);
	  close(socketHolder[i]);
  }
*/
  /* perform read write operations ... */

  return 0;
}
