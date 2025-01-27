#include "queue.h"
#include <cstdio>

void Init_Vehicle_Queue(Vehicle_Queue *q) {
  q->front = -1;
  q->rear = -1;
  q->size = 0;

  /* initially the direction where the vehicle is going is not defined. So we set it to -1 */
  for (int i = 0; i < MAX_VEHICLE_QUEUE_SIZE; i++) {
    q->vehicles[i].direction = -1;
  }
}

int Is_Vehicle_Queue_Empty(Vehicle_Queue *q) {
  if (q->front == q->rear) {
    return 1;
  }
  return 0;
}

int Is_Vehicle_Queue_Full(Vehicle_Queue *q) {
  if (((q->rear + 1) % MAX_VEHICLE_QUEUE_SIZE) == q->front) {
    return 1;
  }
  return 0;
}

void Enqueue_Vehicle(Vehicle_Queue *q, Vehicle v) {
  if (Is_Vehicle_Queue_Full(q)) {
    fprintf(stderr, "The vehicle queue is full. No more vehicles can be accomodated\n");
    return;
  }

  q->rear = (q->rear + 1) % MAX_VEHICLE_QUEUE_SIZE;
  q->vehicles[q->rear] = v;
}

Vehicle Dequeue_Vehicle(Vehicle_Queue *q) {
  if (Is_Vehicle_Queue_Empty(q)) {
    fprintf(stderr, "The vehicle queue is empty.\n");
  }

  q->front = (q->front + 1) % MAX_VEHICLE_QUEUE_SIZE;
  Vehicle vehicle = q->vehicles[q->front];

  /* queue is empty now */
  if (q->front == q->rear) {
    q->front = -1;
    q->rear = -1;
  }

  return vehicle;
}
