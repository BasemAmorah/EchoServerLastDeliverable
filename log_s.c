#include "log_s.h"

#include "utilities.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>
#include <signal.h>

/* UDP socket file descriptor for the server process. */
int udpSockFD;

void interruptHandler(int signalNo) {
  close(udpSockFD);
  exit(0);
}

void logEntry(char *message) {
  const size_t messageBufferSize = 1024;
  const size_t timestampBufferSize = 40;
  char *messageBuffer = calloc(messageBufferSize, sizeof(char));

  time_t now;
  time(&now);
  struct tm *timeStruct = localtime(&now);
  strftime(messageBuffer, timestampBufferSize, "%Y-%m-%d %H:%M:%S", timeStruct);

  char *ipAddress = strtok(message, "\t");
  char *messageContent = strtok(NULL, "\t");

  /* If the last character of the message content is newline, remove it to prevent it from looking ugly. */
  int length = (int)strlen(messageContent);
  if (messageContent[length - 1] == '\n') {
    messageContent[length - 1] = '\0';
  }

  strcat(messageBuffer, "\t\"");
  strcat(messageBuffer, messageContent);
  strcat(messageBuffer, "\" was received from ");
  strcat(messageBuffer, ipAddress);

  /* If file does not exist, create it. Otherwise, append to it. */
  const char *logFileName = "echo.log";
  FILE *file = fopen(logFileName, "a");

  if (!file) {
    error("Could not open file: echo.log");
  }

  fprintf(file, "%s\n", messageBuffer);
  fclose(file);
  free(messageBuffer);
}

int main(int argc, char *argv[]) {
  struct sockaddr_in serv_addr, cli_addr;

  udpSockFD = socket(AF_INET, SOCK_DGRAM, 0);
  if (udpSockFD < 0) {
    error("Error opening UDP socket");
  }

  printf("Logging server is running...\n");

  /* Enable reusing of addresses to help avoid "Address already in use" errors. */
  const int enable = 1;
  setsockopt(udpSockFD, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int));

  /* Install an interrupt handler to catch when user presses Ctrl+C (interrupt)
   to terminate the program. */
  if (signal(SIGINT, interruptHandler) == SIG_ERR) {
    fprintf(stderr, "Could not set interrupt handler in server process");
  }

  /* These two should fail, but we try anyway. */
  signal(SIGSTOP, interruptHandler);
  signal(SIGKILL, interruptHandler);

  /* Set the server address */
  bzero((char *) &serv_addr, sizeof(serv_addr));
  const int portNo = 9999;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portNo);

  if (bind(udpSockFD, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
    fprintf(stderr,
            "Error on binding UDP socket. This usually happens when the server process is killed "
            "(e.g. Ctrl+Z). To fix this, please clean-up all remaining processes named \"server\".\n");
    error(NULL);
  }

  while (1) {
    const size_t BUFFER_SIZE = 512;
    /* calloc() sets the memory to zero, so no need to call bzero here. */
    char *buffer = calloc(BUFFER_SIZE, sizeof(char));

    socklen_t addressLength = sizeof(cli_addr);
    int numBytes = (int)recvfrom(udpSockFD, buffer, BUFFER_SIZE - 1, 0,
                                 (struct sockaddr *)&cli_addr, &addressLength);

    if (numBytes < 0) {
      error("Error in recvfrom");
    }

    pid_t pid = fork();
    if (pid > 0) {
      /* This is the parent process. Do nothing here and keep looping
       * in order not to block the server. */
      continue;
    } else if (pid == 0) {
      /* This is the child process. Process the request and log the message to the file here. */
      logEntry(buffer);
      free(buffer);  /* Free buffer to prevent a memory leak. */
      return 0;  /* Make sure to terminate the child process here in order to prevent a fork bomb. */
    } else {
      /* Something went wrong and the fork operation failed. */
      error("Could not fork a child process");
    }
  }
  
  return 0;
}
