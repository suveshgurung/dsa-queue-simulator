#include "traffic-generator.h"

int main() {
  /* seed rand() with current time in seconds */
  srand(time(NULL));

  while (1) {
    Generate_Vehicles();
    sleep(10);
  }

  return 0;
}

void Generate_Vehicles() {
  /* for now generate from 1 to max 3 vehicles at a time. */
  int vehicle_min = 1;
  int vehicle_max = 3;

  /* lanes are from 1 to 4 
   * 1 -> AL2
   * 2 -> BL2
   * 3 -> CL2
   * 4 -> DL2
   */
  int lane_min = 1;
  int lane_max = 4;

  double r = (double)rand() / RAND_MAX;  // Generate a random float between 0 and 1
  int random_vehicle_number = vehicle_min + (int)(pow(r, 2) * (vehicle_max - vehicle_min));
  int random_lane_number = rand() % (lane_max - lane_min + 1) + lane_min;

  Serialize_Data(random_vehicle_number, random_lane_number);
}

void Serialize_Data(int vehicle_number, int lane_number) {
  char buffer[18];

  snprintf(buffer, 18, "LANE:%d, VEHICLE:%d", lane_number, vehicle_number);

  printf("%s\n", buffer);
}
