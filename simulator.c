/* TODO : see how to render traffic lights */
#include "simulator.h"
#include "queue.h"
#include "socket.h"
#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_log.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_image.h>
#include <sys/socket.h>

int main() {
  if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    SDL_Log("SDL_Init error: %s", SDL_GetError());
    return 1;
  }

  if (IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG) == 0) {
    SDL_Log("IMG_Init error: %s", IMG_GetError());
    return 1;
  }

  SDL_Window *window = SDL_CreateWindow("dsa-queue-simulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH, WINDOW_HEIGHT, 0);
  if (window == NULL) {
    Error_Handler("SDL_CreateWindow", window);
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (renderer == NULL) {
    Error_Handler("SDL_CreateRenderer", window);
  }

  Render_Roads_Traffic_Lights(renderer, window);

  /* open the socket for listening for generated traffic. */
  int socket_FD = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_FD == -1) {
    perror("socket");
    return -1;
  }

  struct sockaddr_in address = Create_IPv4_Socket_Address("127.0.0.1", 6000);
  if (bind(socket_FD, (struct sockaddr *)&address, sizeof(address)) == -1) {
    perror("bind");
    return -1;
  }
  if (listen(socket_FD, 1) == -1) {
    perror("listen");
    return -1;
  }

  bool running = true;
  SDL_Event event;

  /* vehicle queue for three lanes of each of the four roads. 
   * vehicle_queue[0][0] -> R_A L_A1
   * vehicle_queue[0][1] -> R_A L_A2
   * vehicle_queue[0][2] -> R_A L_A3
   *
   * vehicle_queue[1][0] -> R_B L_B1
   * vehicle_queue[1][1] -> R_B L_B2
   * vehicle_queue[1][2] -> R_B L_B3
   *
   * vehicle_queue[2][0] -> R_C L_C1
   * vehicle_queue[2][1] -> R_C L_C2
   * vehicle_queue[2][2] -> R_C L_C3
   *
   * vehicle_queue[3][0] -> R_D L_D1
   * vehicle_queue[3][1] -> R_D L_D2
   * vehicle_queue[3][2] -> R_D L_D3
   */
  Vehicle_Queue vehicle_queue[4][3];
  Lane_Queue lane_queue;

  /* initialize the queues. */
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 3; j++) {
      Init_Vehicle_Queue(&vehicle_queue[i][j]);
    }
  }
  Init_Lane_Queue(&lane_queue);

  while (running) {
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          running = false;
          break;
      }
    }
  }

  SDL_DestroyWindow(window);
  IMG_Quit();
  SDL_Quit();

  return 0;
}

/* 
 * @desc checks the return value of SDL functions. 0 if success and failure for any other return value.
 *
 * @params
 * const char *calling_function_name -> name of the function to be checked for error.
 * SDL_Window *window -> pointer to the window.
 *
 * @retval void
 */
void Error_Checker(int return_value, const char *calling_function_name, SDL_Window *window) {
  if (return_value != 0) {
    Error_Handler(calling_function_name, window);
  }
}

/* 
 * @desc logs the error and exits the program.
 *
 * @params
 * const char *calling_function_name -> name of the function which caused the error.
 * SDL_Window *window -> pointer to the window.
 *
 * @retval void
 */
void Error_Handler(const char *calling_function_name, SDL_Window *window) {
  SDL_Log("%s error: %s", calling_function_name, SDL_GetError());
  if (window != NULL) {
    SDL_DestroyWindow(window);
  }
  IMG_Quit();
  SDL_Quit();
  exit(1);
}

/* 
 * @desc sets the dimensions of the roads.
 *
 * @params
 * SDL_Rect *road -> pointer to the variable which contains the parameters of the road.
 * int x -> x-coordinate from where the rectangle starts to render.
 * int y -> y-coordinate from where the rectangle starts to render.
 * int w -> width of the road.
 * int h -> height of the road.
 *
 * @retval void
 */
void Set_Road_Dimensions(SDL_Rect *road, int x, int y, int w, int h) {
  road->x = x;
  road->y = y;
  road->w = w;
  road->h = h;
}

/* 
 * @desc renders the background along with the roads and traffic lights.
 *
 * @params
 * SDL_Renderer *renderer -> pointer to the renderer.
 * SDL_Window *window -> pointer to the window.
 *
 * @retval void
 */
void Render_Roads_Traffic_Lights(SDL_Renderer *renderer, SDL_Window *window) {
  /* set the background color of the whole screen to white */
  Error_Checker(SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255), "SDL_SetRenderDrawColor", window);

  /* render the previously set background color */
  Error_Checker(SDL_RenderClear(renderer), "SDL_RenderClear", window);

  SDL_Rect first_road;
  SDL_Rect second_road;

  Set_Road_Dimensions(&first_road, 0, WINDOW_HEIGHT/2 - 105, WINDOW_WIDTH, 210);
  Set_Road_Dimensions(&second_road, WINDOW_WIDTH/2 - 105, 0, 210, WINDOW_HEIGHT);

  /* set the color of the road */
  Error_Checker(SDL_SetRenderDrawColor(renderer, 65, 65, 65, 1), "SDL_SetRenderDrawColor", window);
  Error_Checker(SDL_RenderFillRect(renderer, &first_road), "SDL_RenderFillRect", window);
  Error_Checker(SDL_RenderFillRect(renderer, &second_road), "SDL_RenderFillRect", window);

  /* draw the lanes */
  Error_Checker(SDL_SetRenderDrawColor(renderer, 255, 255, 0, 1), "SDL_SetRenderDrawColor", window);
  /* x axis lanes */
  Error_Checker(SDL_RenderDrawLine(renderer, 0, WINDOW_HEIGHT/2 - 35, WINDOW_WIDTH/2 - 105, WINDOW_HEIGHT/2 - 35), "SDL_RenderDrawLine", window);
  Error_Checker(SDL_RenderDrawLine(renderer, 0, WINDOW_HEIGHT/2 + 35, WINDOW_WIDTH/2 - 105, WINDOW_HEIGHT/2 + 35), "SDL_RenderDrawLine", window);
  Error_Checker(SDL_RenderDrawLine(renderer, WINDOW_WIDTH/2 + 105, WINDOW_HEIGHT/2 - 35, WINDOW_WIDTH, WINDOW_HEIGHT/2 - 35), "SDL_RenderDrawLine", window);
  Error_Checker(SDL_RenderDrawLine(renderer, WINDOW_WIDTH/2 + 105, WINDOW_HEIGHT/2 + 35, WINDOW_WIDTH, WINDOW_HEIGHT/2 + 35), "SDL_RenderDrawLine", window);
  /* y axis lanes */
  Error_Checker(SDL_RenderDrawLine(renderer, WINDOW_WIDTH/2 - 35, 0, WINDOW_WIDTH/2 - 35, WINDOW_HEIGHT/2 - 105), "SDL_RenderDrawLine", window);
  Error_Checker(SDL_RenderDrawLine(renderer, WINDOW_WIDTH/2 + 35, 0, WINDOW_WIDTH/2 + 35, WINDOW_HEIGHT/2 - 105), "SDL_RenderDrawLine", window);
  Error_Checker(SDL_RenderDrawLine(renderer, WINDOW_WIDTH/2 - 35, WINDOW_HEIGHT/2 + 105, WINDOW_WIDTH/2 - 35, WINDOW_HEIGHT), "SDL_RenderDrawLine", window);
  Error_Checker(SDL_RenderDrawLine(renderer, WINDOW_WIDTH/2 + 35, WINDOW_HEIGHT/2 + 105, WINDOW_WIDTH/2 + 35, WINDOW_HEIGHT), "SDL_RenderDrawLine", window);

  /* render traffic lights */
  SDL_Texture *traffic_light_laneA = IMG_LoadTexture(renderer, "images/traffic-light-laneA-go.png");
  if (!traffic_light_laneA) {
    printf("Failed to load texture\n");
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    exit(1);
  }
  SDL_Rect traffic_light_laneA_destination;
  traffic_light_laneA_destination.x = WINDOW_WIDTH/2 + 12;
  traffic_light_laneA_destination.y = WINDOW_HEIGHT/2 -105;
  traffic_light_laneA_destination.w = 46;
  traffic_light_laneA_destination.h = 72;
  SDL_RenderCopy(renderer, traffic_light_laneA, NULL, &traffic_light_laneA_destination);

  SDL_Texture *traffic_light_laneB = IMG_LoadTexture(renderer, "images/traffic-light-laneB-go.png");
  if (!traffic_light_laneB) {
    printf("Failed to load texture\n");
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    exit(1);
  }
  SDL_Rect traffic_light_laneB_destination;
  traffic_light_laneB_destination.x = 100;
  traffic_light_laneB_destination.y = 20;
  traffic_light_laneB_destination.w = 46;
  traffic_light_laneB_destination.h = 72;
  SDL_RenderCopy(renderer, traffic_light_laneB, NULL, &traffic_light_laneB_destination);

  SDL_Texture *traffic_light_laneC = IMG_LoadTexture(renderer, "images/traffic-light-laneC-go.png");
  if (!traffic_light_laneC) {
    printf("Failed to load texture\n");
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    exit(1);
  }
  SDL_Rect traffic_light_laneC_destination;
  traffic_light_laneC_destination.x = 200;
  traffic_light_laneC_destination.y = 20;
  traffic_light_laneC_destination.w = 46;
  traffic_light_laneC_destination.h = 72;
  SDL_RenderCopy(renderer, traffic_light_laneC, NULL, &traffic_light_laneC_destination);

  SDL_Texture *traffic_light_laneD = IMG_LoadTexture(renderer, "images/traffic-light-laneD-go.png");
  if (!traffic_light_laneD) {
    printf("Failed to load texture\n");
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    exit(1);
  }
  SDL_Rect traffic_light_laneD_destination;
  traffic_light_laneD_destination.x = 300;
  traffic_light_laneD_destination.y = 20;
  traffic_light_laneD_destination.w = 46;
  traffic_light_laneD_destination.h = 72;
  SDL_RenderCopy(renderer, traffic_light_laneD, NULL, &traffic_light_laneD_destination);

  /* update the screen with the latest renderings */
  SDL_RenderPresent(renderer);
}
