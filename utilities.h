
#ifndef utilities_h
#define utilities_h

#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* Message that is used to signal that echo_s is about to stop */
extern const char *ServerTerminationMessage;

/* Returns the IP address as a string from given client address. */
const char *getIPAddress(struct sockaddr_in *clientAddress);

/* Prints an error message to stderr and exits. */
void error(const char *msg);

#endif /* utilities_h */
