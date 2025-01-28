#include "queue.h"


/* vehicle queue */
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
  /* size goes from 1 to 5 */
  q->size = (q->size + 1) % (MAX_VEHICLE_QUEUE_SIZE + 1);
}

Vehicle Dequeue_Vehicle(Vehicle_Queue *q) {
  if (Is_Vehicle_Queue_Empty(q)) {
    fprintf(stderr, "The vehicle queue is empty.\n");

    /* indication of error */
    Vehicle error;
    error.direction = -1;

    return error;
  }

  q->front = (q->front + 1) % MAX_VEHICLE_QUEUE_SIZE;
  Vehicle vehicle = q->vehicles[q->front];

  /* queue is empty now */
  if (q->front == q->rear) {
    Init_Vehicle_Queue(q);
  }

  return vehicle;
}


/* lane queue */
void Init_Lane_Queue(Lane_Queue *lq) {
  lq->size = 0;
}

int Is_Lane_Queue_Empty(Lane_Queue *lq) {
  if (lq->size == 0) {
    return 1;
  }
  return 0;
}

int Is_Lane_Queue_Full(Lane_Queue *lq) {
  if (lq->size == MAX_LANE_QUEUE_SIZE) {
    return 1;
  }
  return 0;
}

void Enqueue_Lane(Lane_Queue *lq, Lane_Data ld) {
  if (Is_Lane_Queue_Full(lq)) {
    fprintf(stderr, "The lane queue is full!\n");
    return;
  }
  
  int i;
  for (i = lq->size - 1; i >= 0; i--) {
    if (lq->lanes[i].priority > ld.priority) {
      lq->lanes[i + 1] = lq->lanes[i];
    }
    else {
      break;
    }
  }
  lq->lanes[i + 1].lane = ld.lane;
  lq->lanes[i + 1].no_of_vehicle = ld.no_of_vehicle;
  lq->lanes[i + 1].priority = ld.priority;

  lq->size++;
}

Lane_Data Dequeue_Lane(Lane_Queue *lq) {
  if (Is_Lane_Queue_Empty(lq)) {
    fprintf(stderr, "The lane queue is empty!\n");

    Lane_Data error;
    /* -1 to indicate error. */
    error.lane = -1;

    return error;
  }

  Lane_Data dequeued_element = lq->lanes[0];
  for (int i = 1; i < lq->size; i++) {
    lq->lanes[i - 1] = lq->lanes[i];
  }
  lq->size--;

  return dequeued_element;
}
