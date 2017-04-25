
#ifndef ECHO_S_C
#define ECHO_S_C

#include "echo_s.h"

#include "utilities.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>


/* Number of ports that will be listened by the server process. */
int numPorts;

/* TCP socket file descriptors for the server process. */
int *tcpSockFDs;

/* UDP socket file descriptor for the server process. */
int *udpSockFDs;

/* IP address of the log server. Defaults to 127.0.0.1 (localhost), but can be overridden by
 * a command-line parameter.
 */
char *logServerIPAddress = "127.0.0.1";

void interruptHandler(int signalNo) {
  for (int i = 0; i < numPorts; i++) {
    close(tcpSockFDs[i]);
    close(udpSockFDs[i]);
  }
  exit(0);
}

void processTCPRequest(int socketFD, struct sockaddr *address) {
  if (socketFD < 0) {
    return;
  }

  const size_t BUFFER_SIZE = 512;
  /* calloc() sets the memory to zero, so no need to call bzero here. */
  char *buffer = calloc(BUFFER_SIZE, sizeof(char));

  int n = (int)read(socketFD, buffer, BUFFER_SIZE - 1);
  if (n < 0) {
    error("Error reading from socket");
  }

  printf("Message received from TCP client: %s", buffer);
  const char *ipAddress = getIPAddress((struct sockaddr_in *)address);
  printf("Client's IP address: '%s'\n", ipAddress);
  printf("\n");

  /* Send the received message back to the connected client */
  n = (int)write(socketFD, buffer, strlen(buffer));
  if (n < 0) {
    error("Error writing to socket");
  }

  sendToLogServer(ipAddress, buffer);
  free(buffer);  /* Free buffer to prevent a memory leak. */
}

void processUDPRequest(int socketFD, struct sockaddr *address) {
  if (socketFD < 0) {
    return;
  }

  const size_t BUFFER_SIZE = 512;
  /* calloc() sets the memory to zero, so no need to call bzero here. */
  char *buffer = calloc(BUFFER_SIZE, sizeof(char));

  socklen_t addressLength = sizeof(*address);
  int numBytes = (int)recvfrom(socketFD, buffer, BUFFER_SIZE - 1, 0, address, &addressLength);

  if (numBytes < 0) {
    error("Error in recvfrom");
  } else if (numBytes == 0) {
    printf("No data received from UDP socket");
  } else {
    printf("Message received from UDP client: %s", buffer);
  }

  const char *ipAddress = getIPAddress((struct sockaddr_in *)address);
  printf("Client's IP address: '%s'\n", ipAddress);
  printf("\n");

  /* Send the received message back to the connected client */
  numBytes = (int)sendto(socketFD, buffer, strlen(buffer), 0, address, addressLength);
  if (numBytes < 0) {
    error("Error in sendto");
  }

  sendToLogServer(ipAddress, buffer);
  free(buffer);  /* Free buffer to prevent a memory leak. */
}

void sendToLogServer(const char *ipAddress, const char *messageContent) {
  int sockfd;
  struct sockaddr_in serv_addr;

  int portno = 9999;
  struct hostent *server = gethostbyname(logServerIPAddress);
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

  const size_t BUFFER_SIZE = 512;
  /* calloc() sets the memory to zero, so no need to call bzero here. */
  char *buffer = calloc(BUFFER_SIZE, sizeof(char));

  /* Construct the message to be sent to the log server.
   * The message will consist of the IP address of the client and the message content, separated by
   * a tab ('\t') character.
   */
  strcat(buffer, ipAddress);
  strcat(buffer, "\t");
  strcat(buffer, messageContent);

  /* We will communicate with server using UDP */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0) {
    error("error opening socket");
  }

  socklen_t serverAddressLength = sizeof(serv_addr);
  int numBytes = (int)sendto(sockfd, buffer, strlen(buffer), 0,
                             (struct sockaddr *)&serv_addr, serverAddressLength);
  if (numBytes < 0) {
    error("Could not send UDP packet to log server.");
  }

  free(buffer);  /* Free buffer to prevent a memory leak. */
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Invalid arguments to echo_s\n");
    exit(1);
  }

  numPorts = 0;
  // 3 is the max number of ports echo_s will support.
  int *portNos = malloc(3 * sizeof(int));

  // Skip 0, because it will be the name of the executable.
  for (int i = 1; i < argc; i++) {
    char *arg = argv[i];

    if (strcmp(arg, "-logip") == 0) {
      logServerIPAddress = argv[i + 1];
      break;
    } else {
      int portNo = atoi(argv[i]);
      portNos[i - 1] = portNo;
      numPorts++;
    }
  }

  int *newtcpSockFDs = calloc(numPorts, sizeof(int));
  socklen_t *clilens = calloc(numPorts, sizeof(socklen_t));

  struct sockaddr_in *serv_addrs = calloc(numPorts, sizeof(struct sockaddr_in));
  struct sockaddr_in *cli_addrs = calloc(numPorts, sizeof(struct sockaddr_in));

  fd_set fdSet;  /* File descriptor set to store file descriptors for TCP and UDP sockets */

  tcpSockFDs = malloc(sizeof(numPorts * sizeof(int)));
  udpSockFDs = malloc(sizeof(numPorts * sizeof(int)));

  for (int i = 0; i < numPorts; i++) {
    tcpSockFDs[i] = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpSockFDs[i] < 0) {
      error("Error opening TCP socket");
    }
    udpSockFDs[i] = socket(AF_INET, SOCK_DGRAM, 0);
    if (udpSockFDs[i] < 0) {
      error("Error opening UDP socket");
    }
  }

  printf("Server is running...\n");

  for (int i = 0; i < numPorts; i++) {
    /* Enable reusing of addresses to help avoid "Address already in use" errors. */
    const int enable = 1;
    setsockopt(tcpSockFDs[i], SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
    setsockopt(udpSockFDs[i], SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));
  }

  /* Install an interrupt handler to catch when user presses Ctrl+C (interrupt)
   to terminate the program. */
  if (signal(SIGINT, interruptHandler) == SIG_ERR) {
    fprintf(stderr, "Could not set interrupt handler in server process");
  }

  /* These two should fail, but we try anyway. */
  signal(SIGSTOP, interruptHandler);
  signal(SIGKILL, interruptHandler);

  for (int i = 0; i < numPorts; i++) {
    /* Set the server address */
    bzero((char *) serv_addrs + i, sizeof(serv_addrs[i]));
    serv_addrs[i].sin_family = AF_INET;
    serv_addrs[i].sin_addr.s_addr = INADDR_ANY;
    serv_addrs[i].sin_port = htons(portNos[i]);

    if (bind(tcpSockFDs[i], (struct sockaddr *) serv_addrs + i, sizeof(serv_addrs[i])) < 0) {
      fprintf(stderr,
              "Error on binding TCP socket. This usually happens when the server process is killed "
              "(e.g. Ctrl+Z). To fix this, please clean-up all remaining processes named \"echo_s\".\n");
      error(NULL);
    }
    if (bind(udpSockFDs[i], (struct sockaddr *) serv_addrs + i, sizeof(serv_addrs[i])) < 0) {
      fprintf(stderr,
              "Error on binding UDP socket. This usually happens when the server process is killed "
              "(e.g. Ctrl+Z). To fix this, please clean-up all remaining processes named \"echo_s\".\n");
      error(NULL);
    }
  }

  while (1) {
    FD_ZERO(&fdSet);
    for (int i = 0; i < numPorts; i++) {
      listen(tcpSockFDs[i], 5);
      listen(udpSockFDs[i], 5);

      FD_SET(tcpSockFDs[i], &fdSet);
      FD_SET(udpSockFDs[i], &fdSet);
    }

    int maxFD = -1;
    for (int i = 0; i < numPorts; i++) {
      if (tcpSockFDs[i] > maxFD) {
        maxFD = tcpSockFDs[i];
      }
      if (udpSockFDs[i] > maxFD) {
        maxFD = udpSockFDs[i];
      }
    }

    int numReady = select(maxFD + 1, &fdSet, NULL, NULL, NULL);
    if (numReady < 0) {
      error("Error in select function");
    }

    for (int i = 0; i < numPorts; i++) {
      if (FD_ISSET(tcpSockFDs[i], &fdSet)) {
        /* Handle TCP connection */

        clilens[i] = sizeof(cli_addrs[i]);
        newtcpSockFDs[i] = accept(tcpSockFDs[i], (struct sockaddr *)cli_addrs + i, clilens + i);
        if (newtcpSockFDs[i] < 0) {
          error("Error on accept");
        }

        pid_t pid = fork();
        if (pid == 0) {
          /* This is the child process. Read socket and print message from client here. */
          /* We don't need this (parent's) socket here, so it is safe to close it. */
          close(tcpSockFDs[i]);

          processTCPRequest(newtcpSockFDs[i], (struct sockaddr *)cli_addrs + i);
          /* Child should exit immediately after sending the client confirmation. */
          return 0;
        } else if (pid < 0) {
          /* Something went wrong and the fork operation failed. */
          error("Could not fork a child process");
        } else {
          /* This is the parent process. This process should keep listening for new connections. */
          /* We don't need this (child's) socket here, so it is safe to close it. */
          close(newtcpSockFDs[i]);
        }
      }
    }

    for (int i = 0; i < numPorts; i++) {
      if (FD_ISSET(udpSockFDs[i], &fdSet)) {
        /* Handle UDP connection */

        pid_t pid = fork();
        if (pid == 0) {
          /* This is the child process. Process request and print message from client here. */

          processUDPRequest(udpSockFDs[i], (struct sockaddr *)cli_addrs + i);
          /* Child should exit immediately after sending the client confirmation. */
          return 0;
        } else if (pid < 0) {
          /* Something went wrong and the fork operation failed. */
          error("Could not fork a child process");
        } else {
          /* This is the parent process. This process should keep listening for new connections. */
          /* Nothing else needs to be done here. */
        }
      }
    }
  }
  
  return 0;
}

#endif
