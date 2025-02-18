/* TODO : see how to render traffic lights */
#include "simulator.h"
#include "queue.h"
#include <stdlib.h>
#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_log.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_image.h>
#include <sys/socket.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>

int running = 1;
int generator_requesting_connection = 0;
int generator_socket_FD;
int received_from_generator = 0;
char buffer[MAX_SOCKET_BUFFER_SIZE];

int main() {
  signal(SIGINT, Signal_Handler);
  srand(time(NULL));

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

  pthread_t accept_connection_from_generator_thread_id;
  pthread_t render_vehicles_thread_id[12];              /* for 12 lanes */
  pthread_t listen_from_generator_thread_id;
  pthread_t parse_received_data_thread_id;

  /* start a new thread which accepts for connection from generator */
  if (pthread_create(&accept_connection_from_generator_thread_id, NULL, &Accept_Connection_From_Generator, (void *)&socket_FD) != 0) {
    fprintf(stderr, "Error in pthread_create for accept_connection_from_generator_thread\n");
    return -1;
  }
  if (pthread_detach(accept_connection_from_generator_thread_id) != 0) {
    fprintf(stderr, "pthread_detach: accept_connection_from_generator_thread\n");
    return -1;
  }

  SDL_Event event;

  /* vehicle queue for three lanes of each of the four roads. 
   * vehicle_queue[0] -> R_A L_A1
   * vehicle_queue[1] -> R_A L_A2
   * vehicle_queue[2] -> R_A L_A3
   *
   * vehicle_queue[3] -> R_B L_B1
   * vehicle_queue[4] -> R_B L_B2
   * vehicle_queue[5] -> R_B L_B3
   *
   * vehicle_queue[6] -> R_C L_C1
   * vehicle_queue[7] -> R_C L_C2
   * vehicle_queue[8] -> R_C L_C3
   *
   * vehicle_queue[9] -> R_D L_D1
   * vehicle_queue[10] -> R_D L_D2
   * vehicle_queue[11] -> R_D L_D3
   */
  Vehicle_Queue vehicle_queue[12];
  Lane_Queue lane_queue;

  /* initialize the queues. */
  for (int i = 0; i < 12; i++) {
    Init_Vehicle_Queue(&vehicle_queue[i]);
  }
  Init_Lane_Queue(&lane_queue);

  /* make threads for each lane rendering */
  {
    Render_Vehicles_Thread_Data render_vehicles_data[12];
    for (int i = 0; i < 12; i++) {
      render_vehicles_data[i].vehicle_queue = vehicle_queue;
      render_vehicles_data[i].lane = i;
      render_vehicles_data[i].window = window;
      render_vehicles_data[i].renderer = renderer;

      if (pthread_create(&render_vehicles_thread_id[i], NULL, &Render_Vehicles, (void *)&render_vehicles_data[i]) != 0) {
        fprintf(stderr, "Error in pthread_create for render_vehicles_thread\n");
        return -1;
      }
      if (pthread_detach(render_vehicles_thread_id[i]) != 0) {
        fprintf(stderr, "pthread_detach: render_vehicles_thread\n");
        return -1;
      }
    }
  }

  while (running) {
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          running = 0;
          break;
      }
    }

    /* check if generator side is trying to connect */
    if (generator_requesting_connection) {
      /* start a new thread which paralelly listens for data from traffic-generator program */
      if (pthread_create(&listen_from_generator_thread_id, NULL, &Receive_From_Generator, NULL) != 0) {
        fprintf(stderr, "pthread_create: listen_from_generator_thread\n");
        return -1;
      }
      if (pthread_detach(listen_from_generator_thread_id) != 0) {
        fprintf(stderr, "pthread_detach: listen_from_generator_thread\n");
        return -1;
      }
      generator_requesting_connection = 0;
    }

    /* check if data is received from the generator */
    if (received_from_generator) {
      Parser_Thread_Data parser_data = {
        .vehicle_queue = vehicle_queue,
        .lane_queue = &lane_queue
      };
      memcpy(parser_data.data_buffer, buffer, strlen(buffer) + 1);

      /* start a new thread which parses and renders the received data */
      if (pthread_create(&parse_received_data_thread_id, NULL, Parse_Received_Data, (void *)&parser_data) != 0) {
        fprintf(stderr, "pthread_create: parse_received_data_thread\n");
        return -1;
      }
      if (pthread_detach(parse_received_data_thread_id) != 0) {
        fprintf(stderr, "pthread_detach: parse_received_data_thread\n");
        return -1;
      }
      received_from_generator = 0;
    }
  }

  SDL_DestroyWindow(window);
  IMG_Quit();
  SDL_Quit();
  close(socket_FD);

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
void Set_Rectangle_Dimensions(SDL_Rect *road, int x, int y, int w, int h) {
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

  Set_Rectangle_Dimensions(&first_road, 0, WINDOW_HEIGHT/2 - 105, WINDOW_WIDTH, 210);
  Set_Rectangle_Dimensions(&second_road, WINDOW_WIDTH/2 - 105, 0, 210, WINDOW_HEIGHT);

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
  /* SDL_Texture *traffic_light_laneA = IMG_LoadTexture(renderer, "images/traffic-light-laneA-go.png");
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
  */

  /* update the screen with the latest renderings */
  SDL_RenderPresent(renderer);
}

void *Accept_Connection_From_Generator(void *arg) {
  int socket_FD = *((int *)arg);

  struct sockaddr_in generator_socket_address;
  socklen_t generator_socket_address_length = sizeof(generator_socket_address);

  while (running) {
    generator_socket_FD = accept(socket_FD, (struct sockaddr *)&generator_socket_FD, &generator_socket_address_length);
    if (generator_socket_FD == -1) {
      perror("accpet");
      exit(-1);
    }
    else {
      generator_requesting_connection = 1;
    }
  }

  return NULL;
}

void *Receive_From_Generator(void *arg) {
  int is_generator_connected = 1;
  ssize_t bytes_received;
  while (is_generator_connected) {
    bytes_received = recv(generator_socket_FD, buffer, MAX_SOCKET_BUFFER_SIZE, 0);
    if (bytes_received == -1) {
      perror("recv");
      exit(-1);
    }
    if (bytes_received == 0) {
      /* generator side program is terminated */
      is_generator_connected = 0;
    }

    if (is_generator_connected) {
      received_from_generator = 1;
      // printf("%s\n", buffer);
    }
  }

  return NULL;
}

void *Parse_Received_Data(void *arg) {
  Parser_Thread_Data *parser_data = (Parser_Thread_Data *)arg;

  int first_lane_digit = parser_data->data_buffer[5] - '0';
  int second_lane_digit = -1;
  int lane;
  int number_of_vehicles;
  /* lane is either 10 or 11 if parser_data->data_buffer[6] != ',' */
  if (parser_data->data_buffer[6] != ',') {
    second_lane_digit = parser_data->data_buffer[6] - '0';
    number_of_vehicles = parser_data->data_buffer[17] - '0';
  }
  else {
    number_of_vehicles = parser_data->data_buffer[16] - '0';
  }

  if (second_lane_digit != -1) {
    lane = (first_lane_digit * 10) + second_lane_digit;
  }
  else {
    lane = first_lane_digit;
  }

  /* enqueue number_of_vehicles of vehicles */
  for (int i = 0; i < number_of_vehicles; i++) {
    Vehicle vehicle;
    Determine_Vehicle_Direction(&vehicle, lane);
    Enqueue_Vehicle(&parser_data->vehicle_queue[lane], vehicle);
  }

  return NULL;
}

void *Render_Vehicles(void *arg) {
  Render_Vehicles_Thread_Data *render_vehicles_data = (Render_Vehicles_Thread_Data *)arg;

  while (running) {
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 100000000; // 100 milliseconds
    nanosleep(&ts, NULL);

    switch (render_vehicles_data->lane) {
      case L_A2:
        Render_Particular_Lane(&(render_vehicles_data->vehicle_queue[L_A2]), L_A2, render_vehicles_data->window, render_vehicles_data->renderer);
        break;
      case L_A3:
        Render_Particular_Lane(&(render_vehicles_data->vehicle_queue[L_A3]), L_A3, render_vehicles_data->window, render_vehicles_data->renderer);
        break;
      case L_B2:
        Render_Particular_Lane(&(render_vehicles_data->vehicle_queue[L_B2]), L_B2, render_vehicles_data->window, render_vehicles_data->renderer);
        break;
      case L_B3:
        Render_Particular_Lane(&(render_vehicles_data->vehicle_queue[L_B3]), L_B3, render_vehicles_data->window, render_vehicles_data->renderer);
        break;
      case L_C2:
        Render_Particular_Lane(&(render_vehicles_data->vehicle_queue[L_C2]), L_C2, render_vehicles_data->window, render_vehicles_data->renderer);
        break;
      case L_C3:
        Render_Particular_Lane(&(render_vehicles_data->vehicle_queue[L_C3]), L_C3, render_vehicles_data->window, render_vehicles_data->renderer);
        break;
      case L_D2:
        Render_Particular_Lane(&(render_vehicles_data->vehicle_queue[L_D2]), L_D2, render_vehicles_data->window, render_vehicles_data->renderer);
        break;
      case L_D3:
        Render_Particular_Lane(&(render_vehicles_data->vehicle_queue[L_D3]), L_D3, render_vehicles_data->window, render_vehicles_data->renderer);
        break;
    }
  }

  return NULL;
}

void Determine_Vehicle_Direction(Vehicle *vehicle, int lane) {
  /* 1 and 2 are the determiner 
   * 1 -> defines the vertical direction
   * 2 -> defines the horizontal direction
   */
  int random_direction_determiner = rand() % (2 - 1 + 1) + 1;
  switch (lane) {
    case L_A2:         /* AL2 */
      if (random_direction_determiner == 1) {
        vehicle->direction = D_DOWN;
      }
      else {
        vehicle->direction = D_LEFT;
      }
      break;
    case L_A3:         /* AL3 */
      vehicle->direction = D_RIGHT;
      break;
    case L_B2:         /* BL2 */
      if (random_direction_determiner == 1) {
        vehicle->direction = D_UP;
      }
      else {
        vehicle->direction = D_LEFT;
      }
      break;
    case L_B3:         /* BL3 */
      vehicle->direction = D_DOWN;
      break;
    case L_C2:         /* CL2 */
      if (random_direction_determiner == 1) {
        vehicle->direction = D_UP;
      }
      else {
        vehicle->direction = D_RIGHT;
      }
      break;
    case L_C3:         /* CL3 */
      vehicle->direction = D_LEFT;
      break;
    case L_D2:         /* DL2 */
      if (random_direction_determiner == 1) {
        vehicle->direction = D_DOWN;
      }
      else {
        vehicle->direction = D_RIGHT;
      }
      break;
    case L_D3:         /* DL3 */
      vehicle->direction = D_UP;
      break;
  }
}

void Render_Particular_Lane(Vehicle_Queue *queue_to_render, int lane, SDL_Window *window, SDL_Renderer *renderer) {
  // SDL_Rect first_road;
  // SDL_Rect second_road;
  //
  // Set_Rectangle_Dimensions(&first_road, 0, WINDOW_HEIGHT/2 - 105, WINDOW_WIDTH, 210);
  // Set_Rectangle_Dimensions(&second_road, WINDOW_WIDTH/2 - 105, 0, 210, WINDOW_HEIGHT);
  //
  // /* set the color of the road */
  // Error_Checker(SDL_SetRenderDrawColor(renderer, 65, 65, 65, 1), "SDL_SetRenderDrawColor", window);
  // Error_Checker(SDL_RenderFillRect(renderer, &first_road), "SDL_RenderFillRect", window);
  // Error_Checker(SDL_RenderFillRect(renderer, &second_road), "SDL_RenderFillRect", window);


  /* TODO, SEE:we will have some sort of mapping that gives us the fixed coordinate. For eg: for lane B and D, the X position is always the same and for lane A and C, the Y position is always the same while rendereing. Only the Y and X position is changed for each vehicle respectively. */
  // for (int i = 0; i < queue_to_render->size; i++) {
  //   if ((queue_to_render->vehicles[i].x == -1) &&(queue_to_render->vehicles[i].y == -1)) {
  //   }
  // }
  printf("Lane: %d, Size: %d\n", lane, queue_to_render->size);
}

void Signal_Handler(int signal) {
  running = 0;
}
