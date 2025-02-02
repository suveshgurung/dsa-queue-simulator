#include "socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>

struct sockaddr_in Create_IPv4_Socket_Address(const char *ip_address, uint16_t port) {
  struct sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  int return_value;

  if ((return_value = inet_pton(AF_INET, ip_address, &address.sin_addr)) != 1) {
    if (return_value == 0) {
      fprintf(stderr, "IP address does not represent a valid network.\n");
      exit(-1);
    }
    if (return_value == -1) {
      perror("inet_pton");
      exit(-1);
    }
  }

  return address;
}
