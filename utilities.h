
#ifndef utilities_h
#define utilities_h

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Returns the IP address as a string from given client address. */
const char *getIPAddress(struct sockaddr_in *clientAddress);

/* Prints an error message to stderr and exits. */
void error(const char *msg);

#endif /* utilities_h */
