#ifndef TRAFFIC_GENERATOR_H
#define TRAFFIC_GENERATOR_H

extern int socket_FD;

/* function prototypes */
void Generate_Vehicles();
void Serialize_And_Send_Data(int, int);

#endif
