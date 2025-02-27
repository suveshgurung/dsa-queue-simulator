#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "queue.h"
#include "socket.h"
#include <SDL2/SDL_stdinc.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>

extern int running;
extern int generator_requesting_connection;
extern int generator_socket_FD;
extern int received_from_generator;
extern char buffer[MAX_SOCKET_BUFFER_SIZE];

extern int fixed_x_coordinate[12];
extern int fixed_y_coordinate[12];

extern Uint32 frame_start;
extern int frame_time;
extern SDL_Texture *static_texture;

extern SDL_Rect *vehicles_to_render;
extern int number_of_vehicle_to_render;

/* structures */
typedef struct Parser_Thread_Data {
  Vehicle_Queue *vehicle_queue;
  Lane_Queue *lane_queue;
  char data_buffer[MAX_SOCKET_BUFFER_SIZE];
} Parser_Thread_Data;

typedef struct Render_Vehicles_Thread_Data {
  Vehicle_Queue *vehicle_queue;
  /* for rendering purpose */
  SDL_Window *window;
  SDL_Renderer *renderer;
} Render_Vehicles_Thread_Data;

typedef struct Check_For_Connection_And_Received_Data_Thread_Params {
  Vehicle_Queue *vehicle_queue;
  Lane_Queue *lane_queue;
} Check_For_Connection_And_Received_Data_Thread_Params;

/* defines */
#define FPS 60
#define FRAME_DELAY (1000.0 / FPS)
#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 900
#define X_FIXED_VEHICLE_WIDTH 20
#define X_FIXED_VEHICLE_HEIGHT 30
#define Y_FIXED_VEHICLE_WIDTH 30 
#define Y_FIXED_VEHICLE_HEIGHT 20
#define DISTANCE_BETWEEN_VEHICLES 40
#define A_AND_D_FIXED_STOPPING_POINT 315
#define B_AND_C_FIXED_STOPPING_POINT 555

/* function prototypes */
void Error_Checker(int, const char *, SDL_Window *);
void Error_Handler(const char *, SDL_Window *);

void Set_Rectangle_Dimensions(SDL_Rect *, int, int, int, int);
void Render_Roads_Traffic_Lights(SDL_Renderer *, SDL_Window *);

void *Accept_Connection_From_Generator(void *);
void *Receive_From_Generator(void *);
void *Parse_Received_Data(void *);
void *Check_For_Connection_And_Received_Data(void *);

void *Render_Vehicles(void *);
void Determine_Vehicle_Direction_And_Initial_Position(Vehicle *, int);
void Render_Particular_Lane(Vehicle_Queue *, int, SDL_Window *, SDL_Renderer *);

void Signal_Handler(int);

#endif
