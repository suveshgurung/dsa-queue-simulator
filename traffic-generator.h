#ifndef TRAFFIC_GENERATOR_H
#define TRAFFIC_GENERATOR_H

extern int socket_FD;
extern int is_running;
extern int valid_random_lanes[];

/* function prototypes */
void Generate_Vehicles();
void Serialize_And_Send_Data(int, int);
void Signal_Handler(int);

#endif
