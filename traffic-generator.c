#include "traffic-generator.h"
#include "socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

int socket_FD;

int main() {
  /* seed rand() with current time in seconds */
  srand(time(NULL));

  socket_FD = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_FD == -1) {
    perror("socket");
    return -1;
  }

  struct sockaddr_in address = Create_IPv4_Socket_Address("127.0.0.1", 6000);
  if (connect(socket_FD, (struct sockaddr *)&address, sizeof(address)) == -1) {
    perror("connect");
    return -1;
  }

  while (1) {
    Generate_Vehicles();
    sleep(5);
  }

  return 0;
}

void Generate_Vehicles() {
  /* for now generate from 1 to max 3 vehicles at a time. */
  int vehicle_min = 1;
  int vehicle_max = 3;

  /* lanes are from 0 to 11
   * 0 -> AL1
   * 1 -> AL2
   * 2 -> AL3
   * 3 -> BL1
   * 4 -> BL2
   * 5 -> BL3
   * 6 -> CL1
   * 7 -> CL2
   * 8 -> CL3
   * 9 -> DL1
   * 10 -> DL2
   * 11 -> DL3
   */
  int lane_min = 0;
  int lane_max = 11;

  double r = (double)rand() / RAND_MAX;  // Generate a random float between 0 and 1
  int random_vehicle_number = vehicle_min + (int)(pow(r, 2) * (vehicle_max - vehicle_min));
  int random_lane_number = rand() % (lane_max - lane_min + 1) + lane_min;

  Serialize_And_Send_Data(random_vehicle_number, random_lane_number);
}

void Serialize_And_Send_Data(int vehicle_number, int lane_number) {
  char buffer[19];

  snprintf(buffer, 19, "LANE:%d, VEHICLE:%d", lane_number, vehicle_number);

  printf("%s\n", buffer);
  if (send(socket_FD, buffer, strlen(buffer), 0) == -1) {
    perror("send");
    exit(-1);
  }
}
