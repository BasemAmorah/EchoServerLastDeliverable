
#ifndef echo_s_h
#define echo_s_h

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

/* Function that will close date sockets when a terminating signal is received.
 * This is to make sure the port does not remain in use after process is terminated.
 *
 * Please note that SIGSTOP and SIGKILL cannot be caught or handled.
 */
void interruptHandler(int signalNo);

/* Processes a TCP connection and sends a confirmation message to the socket */
void processTCPRequest(int socketFD, struct sockaddr *address);

/* Processes a UDP connection and sends a confirmation message */
void processUDPRequest(int socketFD, struct sockaddr *address);

/* Sends the IP address and message content to the logging server over UDP */
void sendToLogServer(const char *ipAddress, const char *messageContent);

#endif /* echo_s_h */
