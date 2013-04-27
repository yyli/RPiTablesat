#ifndef UDTCOMM_H
#define UDTCOMM_H
#include <udt/udt.h>

double getSystemTime();

UDTSOCKET udtConnect(const char* host, const char* port);

UDTSOCKET udtOpenServer(const char* port);

int udtRecv(UDTSOCKET sock, char *buf, int size);

int udtSend(UDTSOCKET sock, char *buf, int size);

#endif