
#ifndef ECHO_C_C
#define ECHO_C_C

#include "echo_c.h"
#include "utilities.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>

void processTCPConnection(int socketFD, struct sockaddr *serverAddress,
                          char *buffer, int bufferSize) {
  if (connect(socketFD, (struct sockaddr *)serverAddress, sizeof(*serverAddress)) < 0) {
    error("Client count not connect to server");
  }

  int numBytes = (int)write(socketFD, buffer, strlen(buffer));
  if (numBytes < 0) {
    error("Client could not write to TCP socket");
  }

  bzero(buffer, bufferSize);
  numBytes = (int)read(socketFD, buffer, bufferSize - 1);
  if (numBytes < 0) {
    error("Client could not read from TCP socket");
  }

  /* Print server's response */
  printf("%s\n", buffer);

  close(socketFD);
  free(buffer);  /* Free buffer to prevent a memory leak. */
}

void processUDPConnection(int socketFD, struct sockaddr *serverAddress,
                          char *buffer, int bufferSize) {
  socklen_t serverAddressLength = sizeof(*serverAddress);
  int numBytes = (int)sendto(socketFD, buffer, strlen(buffer), 0, serverAddress, serverAddressLength);
  if (numBytes < 0) {
    error("Client could not write to UDP socket");
  }

  numBytes = (int)recvfrom(socketFD, buffer, bufferSize, 0, serverAddress, &serverAddressLength);
  if (numBytes < 0) {
    error("Client could not read from UDP socket");
  }

  /* Print server's response */
  printf("%s\n", buffer);

  free(buffer);  /* Free buffer to prevent a memory leak. */
}

int main(int argc, char *argv[]) {
  int sockfd, portno;
  struct sockaddr_in serv_addr;
  struct hostent *server;

  if (argc < 3) {
    fprintf(stderr, "usage %s hostname port\n", argv[0]);
    exit(0);
  }

  portno = atoi(argv[2]);
  server = gethostbyname(argv[1]);
  if (server == NULL) {
    fprintf(stderr, "error, no such host\n");
    exit(0);
  }

  bzero((char *) &serv_addr, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  bcopy((char *)server->h_addr_list[0],
        (char *)&serv_addr.sin_addr.s_addr,
        server->h_length);
  serv_addr.sin_port = htons(portno);

  char input[100];
  char socketType;
  while (1) {
    printf("Please choose socket type ('U' for UDP, 'T' for TCP): ");
    scanf("%s", input);
    socketType = input[0];

    if (socketType == 'u' || socketType == 'U' || socketType == 't' || socketType == 'T') {
      break;
    }
  }

  /* This is to clear any redundant characters (including newlines) in the standard input stream */
  char c;
  while ((c = getchar()) != '\n' && c != EOF) {
  }

  const size_t BUFFER_SIZE = 512;
  /* calloc() sets the memory to zero, so no need to call bzero here. */
  char *buffer = calloc(BUFFER_SIZE, sizeof(char));

  /* Read message from stdin */
  printf("Please enter the message: ");
  fgets(buffer, BUFFER_SIZE - 1, stdin);

  if (socketType == 't' || socketType == 'T') {
    /* We will communicate with server using TCP */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
      error("error opening socket");
    }

    processTCPConnection(sockfd, (struct sockaddr *)&serv_addr, buffer, BUFFER_SIZE);
  } else {
    /* We will communicate with server using UDP */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
      error("error opening socket");
    }

    processUDPConnection(sockfd, (struct sockaddr *)&serv_addr, buffer, BUFFER_SIZE);
  }
  
  return 0;
}

#endif
