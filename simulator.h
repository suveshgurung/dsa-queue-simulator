#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "queue.h"
#include "socket.h"
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>

extern int running;
extern int generator_requesting_connection;
extern int generator_socket_FD;
extern int received_from_generator;
extern char buffer[MAX_SOCKET_BUFFER_SIZE];

/* structures */
typedef struct Parser_Data {
  Vehicle_Queue *vehicle_queue;
  Lane_Queue *lane_queue;
  char data_buffer[MAX_SOCKET_BUFFER_SIZE];
} Parser_Data;

/* defines */
#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 900

/* function prototypes */
void Error_Checker(int, const char *, SDL_Window *);
void Error_Handler(const char *, SDL_Window *);

void Set_Road_Dimensions(SDL_Rect *, int, int, int, int);
void Render_Roads_Traffic_Lights(SDL_Renderer *, SDL_Window *);

void *Accept_Connection_From_Generator(void *);
void *Receive_From_Generator(void *);
void *Parse_Received_Data(void *);

void Determine_Vehicle_Direction(Vehicle *, int);

void Signal_Handler(int);

#endif
