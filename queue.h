#ifndef QUEUE_H
#define QUEUE_H

#include <stdio.h>

/* defines */

/* for now queue size is 5 */
#define MAX_VEHICLE_QUEUE_SIZE 5
/* there are 4 lanes for traffic conditions */
#define MAX_LANE_QUEUE_SIZE 4

/* enums */
enum Vehicle_Direction {
  D_UP = 0,
  D_DOWN,
  D_LEFT,
  D_RIGHT
};

enum Lanes {
  L_A2 = 0,
  L_B2,
  L_C2,
  L_D2
};


/* structures */

/* defines which direction the vehicle is going.
 * the values can be one of the following:
 * D_UP
 * D_DOWN
 * D_LEFT
 * D_RIGHT
 */
typedef struct Vehicle {
  int direction;
} Vehicle;

/* NOTE: we take the vehicle queue as a circular queue */
typedef struct Queue {
  int front;
  int rear;
  Vehicle vehicles[MAX_VEHICLE_QUEUE_SIZE];
  int size;
} Vehicle_Queue;

typedef struct Lane_Data {
  int lane;
  int no_of_vehicle;
  int priority;
} Lane_Data;

typedef struct Lane_Queue {
  Lane_Data lanes[MAX_LANE_QUEUE_SIZE];
  int size;
} Lane_Queue;


/* function prototypes */
/* vehicle queue */
void Init_Vehicle_Queue(Vehicle_Queue *);
int Is_Vehicle_Queue_Empty(Vehicle_Queue *);
int Is_Vehicle_Queue_Full(Vehicle_Queue *);
void Enqueue_Vehicle(Vehicle_Queue *, Vehicle);
Vehicle Dequeue_Vehicle(Vehicle_Queue *);

/* lane queue */
void Init_Lane_Queue(Lane_Queue *);
int Is_Lane_Queue_Empty(Lane_Queue *);
int Is_Lane_Queue_Full(Lane_Queue *);
void Enqueue_Lane(Lane_Queue *, Lane_Data);

#endif
