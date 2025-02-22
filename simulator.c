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
int fixed_x_coordinate[12] = { 370, 440, 510, -1, -1, -1, 510, 440, 370, -1, -1, -1 };
int fixed_y_coordinate[12] = { -1, -1, -1, 370, 440, 510, -1, -1, -1, 510, 440, 370 };
Uint32 frame_start;
int frame_time;
SDL_Texture *static_texture;
SDL_Rect *vehicles_to_render = NULL;
int number_of_vehicle_to_render = 0;

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

  static_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, WINDOW_WIDTH, WINDOW_HEIGHT);
  SDL_SetRenderTarget(renderer, static_texture);

  Render_Roads_Traffic_Lights(renderer, window);

  SDL_SetRenderTarget(renderer, NULL);
  SDL_RenderCopy(renderer, static_texture, NULL, NULL);
  SDL_RenderPresent(renderer);

  SDL_RenderCopy(renderer, static_texture, NULL, NULL);
  SDL_Rect vehicle;
  Set_Rectangle_Dimensions(&vehicle, 510, 0, X_FIXED_VEHICLE_WIDTH, X_FIXED_VEHICLE_HEIGHT);
  Error_Checker(SDL_SetRenderDrawColor(renderer, 255, 0, 0, 1), "SDL_SetRenderDrawColor", window);
  Error_Checker(SDL_RenderFillRect(renderer, &vehicle), "SDL_RenderFillRect", window);
  SDL_RenderPresent(renderer);


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
  pthread_t render_vehicles_thread_id;
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

  /* make thread for lane rendering */
  Render_Vehicles_Thread_Data render_vehicles_thread_data = {
    .vehicle_queue = vehicle_queue,
    .window = window,
    .renderer = renderer
  };

  if (pthread_create(&render_vehicles_thread_id, NULL, &Render_Vehicles, (void *)&render_vehicles_thread_data) != 0) {
    fprintf(stderr, "error in pthread_create for render_vehicles_thread\n");
    return -1;
  }
  if (pthread_detach(render_vehicles_thread_id) != 0) {
    fprintf(stderr, "pthread_detach: render_vehicles_thread\n");
    return -1;
  }
  // for (int i = 0; i < 12; i++) {
  //   render_vehicles_data[i].vehicle_queue = vehicle_queue;
  //   render_vehicles_data[i].lane = i;
  //   render_vehicles_data[i].window = window;
  //   render_vehicles_data[i].renderer = renderer;
  //
  //   if (pthread_create(&render_vehicles_thread_id[i], null, &render_vehicles, (void *)&render_vehicles_data[i]) != 0) {
  //     fprintf(stderr, "error in pthread_create for render_vehicles_thread\n");
  //     return -1;
  //   }
  //   if (pthread_detach(render_vehicles_thread_id[i]) != 0) {
  //     fprintf(stderr, "pthread_detach: render_vehicles_thread\n");
  //     return -1;
  //   }
  // }

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
  free(vehicles_to_render);

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
  */
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

  int first_lane_digit = parser_data->data_buffer[5] - '0'; int second_lane_digit = -1;
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
    Determine_Vehicle_Direction_And_Initial_Position(&vehicle, lane);
    Enqueue_Vehicle(&parser_data->vehicle_queue[lane], vehicle);
  }

  return NULL;
}

void *Render_Vehicles(void *arg) {
  Render_Vehicles_Thread_Data *render_vehicles_data = (Render_Vehicles_Thread_Data *)arg;
  Vehicle_Queue *vehicle_queue = render_vehicles_data->vehicle_queue;
  SDL_Window *window = render_vehicles_data->window;
  SDL_Renderer *renderer = render_vehicles_data->renderer;

  while (running) {
    // struct timespec ts;
    // ts.tv_sec = 0;
    // ts.tv_nsec = 100000000; // 100 milliseconds
    // nanosleep(&ts, NULL);

    frame_start = SDL_GetTicks();
    SDL_Rect *vehicles = NULL;
    int vehicles_size = 0;
    int vehicles_index = 0;
    for (int i = 0; i < 12; i++) {
      vehicles_size += vehicle_queue[i].size;
      vehicles = (SDL_Rect *)realloc(vehicles, vehicles_size);
      if (vehicles == NULL) {
        perror("realloc");
        return NULL;
      }
      for (int j = 0; j < vehicle_queue[i].size; j++) {
        Set_Rectangle_Dimensions(&vehicles[vehicles_index], vehicle_queue[i].vehicles[j].x, vehicle_queue[i].vehicles[j].y, vehicles[j].w, vehicles[j].h);
        vehicles_index++;
      }
    }

    SDL_RenderCopy(renderer, static_texture, NULL, NULL);
    for (int i = 0; i < vehicles_index; i++) {
      Error_Checker(SDL_SetRenderDrawColor(renderer, 255, 0, 0, 1), "SDL_SetRenderDrawColor", window);
      Error_Checker(SDL_RenderFillRect(renderer, &vehicles[i]), "SDL_RenderFillRect", window);
    }
    free(vehicles);

    SDL_RenderPresent(renderer);

    frame_time = SDL_GetTicks() - frame_start;
    if (FRAME_DELAY > frame_time) {
      SDL_Delay(FRAME_DELAY - frame_time); // Delay to maintain frame rate
    }

    // switch (render_vehicles_data->lane) {
    //   case L_A2:
    //     // Render_Particular_Lane(&(render_vehicles_data->vehicle_queue[L_A2]), L_A2, render_vehicles_data->window, render_vehicles_data->renderer);
    //     break;
    //   case L_A3:
    //     // Render_Particular_Lane(&(render_vehicles_data->vehicle_queue[L_A3]), L_A3, render_vehicles_data->window, render_vehicles_data->renderer);
    //     break;
    //   case L_B2:
    //     // Render_Particular_Lane(&(render_vehicles_data->vehicle_queue[L_B2]), L_B2, render_vehicles_data->window, render_vehicles_data->renderer);
    //     break;
    //   case L_B3:
    //     // Render_Particular_Lane(&(render_vehicles_data->vehicle_queue[L_B3]), L_B3, render_vehicles_data->window, render_vehicles_data->renderer);
    //     break;
    //   case L_C2:
    //     // Render_Particular_Lane(&(render_vehicles_data->vehicle_queue[L_C2]), L_C2, render_vehicles_data->window, render_vehicles_data->renderer);
    //     break;
    //   case L_C3:
    //     // Render_Particular_Lane(&(render_vehicles_data->vehicle_queue[L_C3]), L_C3, render_vehicles_data->window, render_vehicles_data->renderer);
    //     break;
    //   case L_D2:
    //     // Render_Particular_Lane(&(render_vehicles_data->vehicle_queue[L_D2]), L_D2, render_vehicles_data->window, render_vehicles_data->renderer);
    //     break;
    //   case L_D3:
    //     // Render_Particular_Lane(&(render_vehicles_data->vehicle_queue[L_D3]), L_D3, render_vehicles_data->window, render_vehicles_data->renderer);
    //     break;
    // }
  }

  return NULL;
}

void Determine_Vehicle_Direction_And_Initial_Position(Vehicle *vehicle, int lane) {
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
      vehicle->x = fixed_x_coordinate[L_A2];
      vehicle->y = 0;                 // vehicles on lane A2 start from top of the screen.
      vehicle->w = X_FIXED_VEHICLE_WIDTH;
      vehicle->h = X_FIXED_VEHICLE_HEIGHT;
      break;
    case L_A3:         /* AL3 */
      vehicle->direction = D_RIGHT;
      vehicle->x = fixed_x_coordinate[L_A3];
      vehicle->y = 0;                 // vehicles on lane A3 start from top of the screen.
      vehicle->w = X_FIXED_VEHICLE_WIDTH;
      vehicle->h = X_FIXED_VEHICLE_HEIGHT;
      break;
    case L_B2:         /* BL2 */
      if (random_direction_determiner == 1) {
        vehicle->direction = D_UP;
      }
      else {
        vehicle->direction = D_LEFT;
      }
      vehicle->x = WINDOW_WIDTH;      // vehicles on lane B2 start from the far right of the screen.
      vehicle->y = fixed_y_coordinate[L_B2];
      vehicle->w = Y_FIXED_VEHICLE_WIDTH;
      vehicle->h = Y_FIXED_VEHICLE_HEIGHT;
      break;
    case L_B3:         /* BL3 */
      vehicle->direction = D_DOWN;
      vehicle->x = WINDOW_WIDTH;      // vehicles on lane B3 start from the far right of the screen.
      vehicle->y = fixed_y_coordinate[L_B3];
      vehicle->w = Y_FIXED_VEHICLE_WIDTH;
      vehicle->h = Y_FIXED_VEHICLE_HEIGHT;
      break;
    case L_C2:         /* CL2 */
      if (random_direction_determiner == 1) {
        vehicle->direction = D_UP;
      }
      else {
        vehicle->direction = D_RIGHT;
      }
      vehicle->x = fixed_x_coordinate[L_C2];
      vehicle->y = WINDOW_HEIGHT;     // vehicles on lane C2 start from the bottom of the screen.
      vehicle->w = X_FIXED_VEHICLE_WIDTH;
      vehicle->h = X_FIXED_VEHICLE_HEIGHT;
      break;
    case L_C3:         /* CL3 */
      vehicle->direction = D_LEFT;
      vehicle->x = fixed_x_coordinate[L_C3];
      vehicle->y = WINDOW_HEIGHT;     // vehicles on lane C3 start from the bottom of the screen.
      vehicle->w = X_FIXED_VEHICLE_WIDTH;
      vehicle->h = X_FIXED_VEHICLE_HEIGHT;
      break;
    case L_D2:         /* DL2 */
      if (random_direction_determiner == 1) {
        vehicle->direction = D_DOWN;
      }
      else {
        vehicle->direction = D_RIGHT;
      }
      vehicle->x = 0;      // vehicles on lane D2 start from the left of the screen.
      vehicle->y = fixed_y_coordinate[L_D2];
      vehicle->w = Y_FIXED_VEHICLE_WIDTH;
      vehicle->h = Y_FIXED_VEHICLE_HEIGHT;
      break;
    case L_D3:         /* DL3 */
      vehicle->direction = D_UP;
      vehicle->x = 0;      // vehicles on lane D3 start from the left of the screen.
      vehicle->y = fixed_y_coordinate[L_D3];
      vehicle->w = Y_FIXED_VEHICLE_WIDTH;
      vehicle->h = Y_FIXED_VEHICLE_HEIGHT;
      break;
  }
}

// void Render_Particular_Lane(Vehicle_Queue *queue_to_render, int lane, SDL_Window *window, SDL_Renderer *renderer) {
//   /* render the first vehicle of the queue so that other can take reference of it */
//   if (queue_to_render->size != 0) {
//     switch (lane) {
//       case L_A1:
//         queue_to_render->vehicles[0].x = fixed_x_coordinate[L_A1];
//         queue_to_render->vehicles[0].y = 0;
//         while (queue_to_render->vehicles[0].y < A_AND_D_FIXED_STOPPING_POINT) {
//           frame_start = SDL_GetTicks();
//           queue_to_render->vehicles[0].y += 10;
//
//           Render_Roads_Traffic_Lights(renderer, window);
//
//           SDL_Rect vehicle;
//           Set_Rectangle_Dimensions(&vehicle, queue_to_render->vehicles[0].x, queue_to_render->vehicles[0].y, X_FIXED_VEHICLE_WIDTH, X_FIXED_VEHICLE_HEIGHT);
//           Error_Checker(SDL_SetRenderDrawColor(renderer, 255, 0, 0, 1), "SDL_SetRenderDrawColor", window);
//           Error_Checker(SDL_RenderFillRect(renderer, &vehicle), "SDL_RenderFillRect", window);
//           SDL_RenderPresent(renderer);
//
//           frame_time = SDL_GetTicks() - frame_start;
//           if (FRAME_DELAY > frame_time) {
//             SDL_Delay(FRAME_DELAY - frame_time); // Delay to maintain frame rate
//           }
//         }
//         break;
//       case L_A2:
//         printf("Vehicle come\n");
//         queue_to_render->vehicles[0].x = fixed_x_coordinate[L_A2];
//         queue_to_render->vehicles[0].y = 0;
//         while (queue_to_render->vehicles[0].y < A_AND_D_FIXED_STOPPING_POINT) {
//           // printf("Vehicle come\n");
//           frame_start = SDL_GetTicks();
//           queue_to_render->vehicles[0].y += 10;
//
//           Render_Roads_Traffic_Lights(renderer, window);
//
//           SDL_Rect vehicle;
//           Set_Rectangle_Dimensions(&vehicle, queue_to_render->vehicles[0].x, queue_to_render->vehicles[0].y, X_FIXED_VEHICLE_WIDTH, X_FIXED_VEHICLE_HEIGHT);
//           Error_Checker(SDL_SetRenderDrawColor(renderer, 255, 0, 0, 1), "SDL_SetRenderDrawColor", window);
//           Error_Checker(SDL_RenderFillRect(renderer, &vehicle), "SDL_RenderFillRect", window);
//           SDL_RenderPresent(renderer);
//
//           frame_time = SDL_GetTicks() - frame_start;
//           if (FRAME_DELAY > frame_time) {
//             SDL_Delay(FRAME_DELAY - frame_time); // Delay to maintain frame rate
//           }
//         }
//         break;
//       case L_A3:
//         queue_to_render->vehicles[0].x = fixed_x_coordinate[L_A3];
//         queue_to_render->vehicles[0].y = 0;
//         while (queue_to_render->vehicles[0].y < A_AND_D_FIXED_STOPPING_POINT) {
//           frame_start = SDL_GetTicks();
//           queue_to_render->vehicles[0].y += 10;
//
//           Render_Roads_Traffic_Lights(renderer, window);
//
//           SDL_Rect vehicle;
//           Set_Rectangle_Dimensions(&vehicle, queue_to_render->vehicles[0].x, queue_to_render->vehicles[0].y, X_FIXED_VEHICLE_WIDTH, X_FIXED_VEHICLE_HEIGHT);
//           Error_Checker(SDL_SetRenderDrawColor(renderer, 255, 0, 0, 1), "SDL_SetRenderDrawColor", window);
//           Error_Checker(SDL_RenderFillRect(renderer, &vehicle), "SDL_RenderFillRect", window);
//           SDL_RenderPresent(renderer);
//
//           frame_time = SDL_GetTicks() - frame_start;
//           if (FRAME_DELAY > frame_time) {
//             SDL_Delay(FRAME_DELAY - frame_time); // Delay to maintain frame rate
//           }
//         }
//         break;
//       case L_B1:
//         queue_to_render->vehicles[0].x = WINDOW_WIDTH - 30;
//         queue_to_render->vehicles[0].y += fixed_y_coordinate[L_B1];
//         while (queue_to_render->vehicles[0].x > B_AND_C_FIXED_STOPPING_POINT) {
//           frame_start = SDL_GetTicks();
//           queue_to_render->vehicles[0].x -= 10;
//
//           Render_Roads_Traffic_Lights(renderer, window);
//
//           SDL_Rect vehicle;
//           Set_Rectangle_Dimensions(&vehicle, queue_to_render->vehicles[0].x, queue_to_render->vehicles[0].y, Y_FIXED_VEHICLE_WIDTH, Y_FIXED_VEHICLE_HEIGHT);
//           Error_Checker(SDL_SetRenderDrawColor(renderer, 255, 0, 0, 1), "SDL_SetRenderDrawColor", window);
//           Error_Checker(SDL_RenderFillRect(renderer, &vehicle), "SDL_RenderFillRect", window);
//           SDL_RenderPresent(renderer);
//
//           frame_time = SDL_GetTicks() - frame_start;
//           if (FRAME_DELAY > frame_time) {
//             SDL_Delay(FRAME_DELAY - frame_time); // Delay to maintain frame rate
//           }
//         }
//         break;
//       case L_B2:
//         queue_to_render->vehicles[0].x = WINDOW_WIDTH - 30;
//         queue_to_render->vehicles[0].y += fixed_y_coordinate[L_B2];
//         while (queue_to_render->vehicles[0].x > B_AND_C_FIXED_STOPPING_POINT) {
//           frame_start = SDL_GetTicks();
//           queue_to_render->vehicles[0].x -= 10;
//
//           Render_Roads_Traffic_Lights(renderer, window);
//
//           SDL_Rect vehicle;
//           Set_Rectangle_Dimensions(&vehicle, queue_to_render->vehicles[0].x, queue_to_render->vehicles[0].y, Y_FIXED_VEHICLE_WIDTH, Y_FIXED_VEHICLE_HEIGHT);
//           Error_Checker(SDL_SetRenderDrawColor(renderer, 255, 0, 0, 1), "SDL_SetRenderDrawColor", window);
//           Error_Checker(SDL_RenderFillRect(renderer, &vehicle), "SDL_RenderFillRect", window);
//           SDL_RenderPresent(renderer);
//
//           frame_time = SDL_GetTicks() - frame_start;
//           if (FRAME_DELAY > frame_time) {
//             SDL_Delay(FRAME_DELAY - frame_time); // Delay to maintain frame rate
//           }
//         }
//         break;
//       case L_B3:
//         printf("LANE: %d, size: %d\n", lane, queue_to_render->size);
//         queue_to_render->vehicles[0].x = WINDOW_WIDTH - 30;
//         queue_to_render->vehicles[0].y += fixed_y_coordinate[L_B3];
//         while (queue_to_render->vehicles[0].x > B_AND_C_FIXED_STOPPING_POINT) {
//           frame_start = SDL_GetTicks();
//           queue_to_render->vehicles[0].x -= 10;
//
//           Render_Roads_Traffic_Lights(renderer, window);
//
//           SDL_Rect vehicle;
//           Set_Rectangle_Dimensions(&vehicle, queue_to_render->vehicles[0].x, queue_to_render->vehicles[0].y, Y_FIXED_VEHICLE_WIDTH, Y_FIXED_VEHICLE_HEIGHT);
//           Error_Checker(SDL_SetRenderDrawColor(renderer, 255, 0, 0, 1), "SDL_SetRenderDrawColor", window);
//           Error_Checker(SDL_RenderFillRect(renderer, &vehicle), "SDL_RenderFillRect", window);
//           SDL_RenderPresent(renderer);
//
//           frame_time = SDL_GetTicks() - frame_start;
//           if (FRAME_DELAY > frame_time) {
//             SDL_Delay(FRAME_DELAY - frame_time); // Delay to maintain frame rate
//           }
//         }
//         break;
//       case L_C1:
//         queue_to_render->vehicles[0].x = fixed_x_coordinate[L_C1];
//         queue_to_render->vehicles[0].y = WINDOW_HEIGHT - 30;
//         while (queue_to_render->vehicles[0].y > B_AND_C_FIXED_STOPPING_POINT) {
//           frame_start = SDL_GetTicks();
//           queue_to_render->vehicles[0].y -= 10;
//
//           Render_Roads_Traffic_Lights(renderer, window);
//
//           SDL_Rect vehicle;
//           Set_Rectangle_Dimensions(&vehicle, queue_to_render->vehicles[0].x, queue_to_render->vehicles[0].y, X_FIXED_VEHICLE_WIDTH, X_FIXED_VEHICLE_HEIGHT);
//           Error_Checker(SDL_SetRenderDrawColor(renderer, 255, 0, 0, 1), "SDL_SetRenderDrawColor", window);
//           Error_Checker(SDL_RenderFillRect(renderer, &vehicle), "SDL_RenderFillRect", window);
//           SDL_RenderPresent(renderer);
//
//           frame_time = SDL_GetTicks() - frame_start;
//           if (FRAME_DELAY > frame_time) {
//             SDL_Delay(FRAME_DELAY - frame_time); // Delay to maintain frame rate
//           }
//         }
//         break;
//       case L_C2:
//         queue_to_render->vehicles[0].x = fixed_x_coordinate[L_C2];
//         queue_to_render->vehicles[0].y = WINDOW_HEIGHT - 30;
//         while (queue_to_render->vehicles[0].y > B_AND_C_FIXED_STOPPING_POINT) {
//           frame_start = SDL_GetTicks();
//           queue_to_render->vehicles[0].y -= 10;
//
//           Render_Roads_Traffic_Lights(renderer, window);
//
//           SDL_Rect vehicle;
//           Set_Rectangle_Dimensions(&vehicle, queue_to_render->vehicles[0].x, queue_to_render->vehicles[0].y, X_FIXED_VEHICLE_WIDTH, X_FIXED_VEHICLE_HEIGHT);
//           Error_Checker(SDL_SetRenderDrawColor(renderer, 255, 0, 0, 1), "SDL_SetRenderDrawColor", window);
//           Error_Checker(SDL_RenderFillRect(renderer, &vehicle), "SDL_RenderFillRect", window);
//           SDL_RenderPresent(renderer);
//
//           frame_time = SDL_GetTicks() - frame_start;
//           if (FRAME_DELAY > frame_time) {
//             SDL_Delay(FRAME_DELAY - frame_time); // Delay to maintain frame rate
//           }
//         }
//         break;
//       case L_C3:
//         queue_to_render->vehicles[0].x = fixed_x_coordinate[L_C3];
//         queue_to_render->vehicles[0].y = WINDOW_HEIGHT - 30;
//         while (queue_to_render->vehicles[0].y > B_AND_C_FIXED_STOPPING_POINT) {
//           frame_start = SDL_GetTicks();
//           queue_to_render->vehicles[0].y -= 10;
//
//           Render_Roads_Traffic_Lights(renderer, window);
//
//           SDL_Rect vehicle;
//           Set_Rectangle_Dimensions(&vehicle, queue_to_render->vehicles[0].x, queue_to_render->vehicles[0].y, X_FIXED_VEHICLE_WIDTH, X_FIXED_VEHICLE_HEIGHT);
//           Error_Checker(SDL_SetRenderDrawColor(renderer, 255, 0, 0, 1), "SDL_SetRenderDrawColor", window);
//           Error_Checker(SDL_RenderFillRect(renderer, &vehicle), "SDL_RenderFillRect", window);
//           SDL_RenderPresent(renderer);
//
//           frame_time = SDL_GetTicks() - frame_start;
//           if (FRAME_DELAY > frame_time) {
//             SDL_Delay(FRAME_DELAY - frame_time); // Delay to maintain frame rate
//           }
//         }
//         break;
//       case L_D1:
//         queue_to_render->vehicles[0].x = 0;
//         queue_to_render->vehicles[0].y = fixed_y_coordinate[L_D1];
//         while (queue_to_render->vehicles[0].x < A_AND_D_FIXED_STOPPING_POINT) {
//           frame_start = SDL_GetTicks();
//           queue_to_render->vehicles[0].x += 10;
//
//           Render_Roads_Traffic_Lights(renderer, window);
//
//           SDL_Rect vehicle;
//           Set_Rectangle_Dimensions(&vehicle, queue_to_render->vehicles[0].x, queue_to_render->vehicles[0].y, Y_FIXED_VEHICLE_WIDTH, Y_FIXED_VEHICLE_HEIGHT);
//           Error_Checker(SDL_SetRenderDrawColor(renderer, 255, 0, 0, 1), "SDL_SetRenderDrawColor", window);
//           Error_Checker(SDL_RenderFillRect(renderer, &vehicle), "SDL_RenderFillRect", window);
//           SDL_RenderPresent(renderer);
//
//           frame_time = SDL_GetTicks() - frame_start;
//           if (FRAME_DELAY > frame_time) {
//             SDL_Delay(FRAME_DELAY - frame_time); // Delay to maintain frame rate
//           }
//         }
//         break;
//       case L_D2:
//         queue_to_render->vehicles[0].x = 0;
//         queue_to_render->vehicles[0].y = fixed_y_coordinate[L_D2];
//         while (queue_to_render->vehicles[0].x < A_AND_D_FIXED_STOPPING_POINT) {
//           frame_start = SDL_GetTicks();
//           queue_to_render->vehicles[0].x += 10;
//
//           Render_Roads_Traffic_Lights(renderer, window);
//
//           SDL_Rect vehicle;
//           Set_Rectangle_Dimensions(&vehicle, queue_to_render->vehicles[0].x, queue_to_render->vehicles[0].y, Y_FIXED_VEHICLE_WIDTH, Y_FIXED_VEHICLE_HEIGHT);
//           Error_Checker(SDL_SetRenderDrawColor(renderer, 255, 0, 0, 1), "SDL_SetRenderDrawColor", window);
//           Error_Checker(SDL_RenderFillRect(renderer, &vehicle), "SDL_RenderFillRect", window);
//           SDL_RenderPresent(renderer);
//
//           frame_time = SDL_GetTicks() - frame_start;
//           if (FRAME_DELAY > frame_time) {
//             SDL_Delay(FRAME_DELAY - frame_time); // Delay to maintain frame rate
//           }
//         }
//         break;
//       case L_D3:
//         queue_to_render->vehicles[0].x = 0;
//         queue_to_render->vehicles[0].y = fixed_y_coordinate[L_D3];
//         while (queue_to_render->vehicles[0].x < A_AND_D_FIXED_STOPPING_POINT) {
//           frame_start = SDL_GetTicks();
//           queue_to_render->vehicles[0].x += 10;
//
//           Render_Roads_Traffic_Lights(renderer, window);
//
//           SDL_Rect vehicle;
//           Set_Rectangle_Dimensions(&vehicle, queue_to_render->vehicles[0].x, queue_to_render->vehicles[0].y, Y_FIXED_VEHICLE_WIDTH, Y_FIXED_VEHICLE_HEIGHT);
//           Error_Checker(SDL_SetRenderDrawColor(renderer, 255, 0, 0, 1), "SDL_SetRenderDrawColor", window);
//           Error_Checker(SDL_RenderFillRect(renderer, &vehicle), "SDL_RenderFillRect", window);
//           SDL_RenderPresent(renderer);
//
//           frame_time = SDL_GetTicks() - frame_start;
//           if (FRAME_DELAY > frame_time) {
//             SDL_Delay(FRAME_DELAY - frame_time); // Delay to maintain frame rate
//           }
//         }
//         break;
//     }
//   }
//
//   // for (int i = 1; i < queue_to_render->size; i++) {
//   //   int previous_vehicle_x_pos = queue_to_render->vehicles[i - 1].x;
//   //   int previous_vehicle_y_pos = queue_to_render->vehicles[i - 1].y;
//   //   int *current_vehicle_x_pos = &(queue_to_render->vehicles[i].x);
//   //   int *current_vehicle_y_pos = &(queue_to_render->vehicles[i].y);
//   //
//   //   /* the vehicle has not been rendered yet. */
//   //   if ((*current_vehicle_x_pos == -1) &&(*current_vehicle_y_pos == -1)) {
//   //     switch (lane) {
//   //       case L_A1:
//   //         *current_vehicle_x_pos = fixed_x_coordinate[L_A1];
//   //         *current_vehicle_y_pos = previous_vehicle_y_pos + DISTANCE_BETWEEN_VEHICLES;
//   //         break;
//   //       case L_A2:
//   //         *current_vehicle_x_pos = fixed_x_coordinate[L_A2];
//   //         *current_vehicle_y_pos = previous_vehicle_y_pos + DISTANCE_BETWEEN_VEHICLES;
//   //         break;
//   //       case L_A3:
//   //         *current_vehicle_x_pos = fixed_x_coordinate[L_A3];
//   //         *current_vehicle_y_pos = previous_vehicle_y_pos + DISTANCE_BETWEEN_VEHICLES;
//   //         break;
//   //       case L_B1:
//   //         break;
//   //       case L_B2:
//   //         break;
//   //       case L_B3:
//   //         break;
//   //       case L_C1:
//   //         break;
//   //       case L_C2:
//   //         break;
//   //       case L_C3:
//   //         break;
//   //       case L_D1:
//   //         break;
//   //       case L_D2:
//   //         break;
//   //       case L_D3:
//   //         break;
//   //     }
//   //   }
//   // }
//
//   // if (queue_to_render->size == 0) {
//   //   printf("LANE: %d, Vehicle: %d\n", lane, queue_to_render->size);
//   // }
// }

void Signal_Handler(int signal) {
  running = 0;
}
