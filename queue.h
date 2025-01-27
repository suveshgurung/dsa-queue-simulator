#ifndef QUEUE_H
#define QUEUE_H

/* defines */

/* for now queue size is 5 */
#define MAX_VEHICLE_QUEUE_SIZE 5

/* enums */
enum Vehicle_Direction {
  D_UP = 0,
  D_DOWN,
  D_LEFT,
  D_RIGHT
};


/* structures */

/* defines which direction the vehicle is going. */
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


/* function prototypes */
void Init_Vehicle_Queue(Vehicle_Queue *);
int Is_Vehicle_Queue_Empty(Vehicle_Queue *);
int Is_Vehicle_Queue_Full(Vehicle_Queue *);
void Enqueue_Vehicle(Vehicle_Queue *, Vehicle);
Vehicle Dequeue_Vehicle(Vehicle_Queue *);

#endif
