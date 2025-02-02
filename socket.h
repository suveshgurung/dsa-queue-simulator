#ifndef SOCKET_H
#define SOCKET_H

#include <netinet/in.h>
#include <stdint.h>

struct sockaddr_in Create_IPv4_Socket_Address(const char *, uint16_t);

#endif
