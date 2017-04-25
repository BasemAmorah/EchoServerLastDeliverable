
#ifndef echo_c_h
#define echo_c_h

#include <sys/socket.h>
#include <netinet/in.h>

/* Tries to create a TCP connection to the server. If successful, it then sends a message,
 * and displays the server's response
 */
void processTCPConnection(int socketFD, struct sockaddr *serverAddress,
                          char *buffer, int bufferSize);

/* Tries to create a UDP connection to the server. If successful, it then sends a message,
 * and displays the server's response
 */
void processUDPConnection(int socketFD, struct sockaddr *serverAddress,
                          char *buffer, int bufferSize);

#endif /* echo_c_h */
