#ifndef UDTCOMM_H
#define UDTCOMM_H
#include <udt/udt.h>

// gets the system time from CLOCK_MONOTONIC in seconds
double getSystemTime();

// connect to a udt server for clients
// RETURNS: the UDTSOCKET descriptor
UDTSOCKET udtConnect(const char* host, const char* port);

// open a udt server for servers
// NOTE: does not accept clients only initializes server structures
// NOTE: look at camserver's main function to see an example of how to listen 
// BOTE: and accept connections
// RETURNS: UDTSOCKET descriptor
UDTSOCKET udtOpenServer(const char* port);

// recv a buf of a specific size
// RETURN: 0 if success, <0 on failture
int udtRecv(UDTSOCKET sock, char *buf, int size);

// send a buf of specific size
// RETURN: 0 if success, <0 on failture
int udtSend(UDTSOCKET sock, char *buf, int size);

#endif