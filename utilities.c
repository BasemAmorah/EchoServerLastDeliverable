#include "utilities.h"

const char *getIPAddress(struct sockaddr_in *clientAddress) {
  char *buffer = malloc(INET_ADDRSTRLEN * sizeof(char));
  inet_ntop(AF_INET, &clientAddress->sin_addr, buffer, INET_ADDRSTRLEN);
  return buffer;
}

void error(const char *msg) {
  perror(msg);
  exit(0);
}
