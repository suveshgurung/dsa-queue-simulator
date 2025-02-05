#include "traffic-generator.h"
#include "socket.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

int socket_FD;
int is_running = 1;
/* lane AL1->0, BL1->3, CL1->6, DL1->9 are incoming lanes only */
int valid_random_lanes[] = {1, 2, 4, 5, 7, 8, 10, 11};

int main() {
  /* seed rand() with current time in seconds */
  signal(SIGINT, Signal_Handler);
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

  while (is_running) {
    Generate_Vehicles();
    sleep(1);
  }

  close(socket_FD);

  return 0;
}

void Generate_Vehicles() {
  /* for now generate from 1 to max 3 vehicles at a time. */
  int vehicle_min = 1;
  int vehicle_max = 3;

  /* index of lanes are from 0 to 7 and the corresponding lanes are in valid_random_lanes[] */
  int lane_min = 0;
  int lane_max = 7;

  double r = (double)rand() / RAND_MAX;  // Generate a random float between 0 and 1
  int random_vehicle_number = vehicle_min + (int)(pow(r, 2) * (vehicle_max - vehicle_min));
  int random_lane_index = rand() % (lane_max - lane_min + 1) + lane_min;

  Serialize_And_Send_Data(random_vehicle_number, valid_random_lanes[random_lane_index]);
}

void Serialize_And_Send_Data(int vehicle_number, int lane_number) {
  char buffer[MAX_SOCKET_BUFFER_SIZE];

  snprintf(buffer, MAX_SOCKET_BUFFER_SIZE, "LANE:%d, VEHICLE:%d", lane_number, vehicle_number);

  printf("%s\n", buffer);
  /* strlen(buffer) + 1 to send the null byte as well */
  if (send(socket_FD, buffer, strlen(buffer) + 1, 0) == -1) {
    perror("send");
    exit(-1);
  }
}

void Signal_Handler(int signal) {
  is_running = 0;
}
