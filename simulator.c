#include "simulator.h"

int main() {
  if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    SDL_Log("SDL_Init error: %s", SDL_GetError());
    return 1;
  }

  SDL_Window *window = SDL_CreateWindow("dsa-queue-simulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 800, 0);
  if (window == NULL) {
    Error_Handler("SDL_CreateWindow", window);
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (renderer == NULL) {
    Error_Handler("SDL_CreateRenderer", window);
  }
  /* set the background color of the whole screen to white */
  Error_Checker(SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255), "SDL_SetRenderDrawColor", window);

  /* render the previously set background color */
  Error_Checker(SDL_RenderClear(renderer), "SDL_RenderClear", window);

  SDL_Rect first_road;
  SDL_Rect second_road;

  Set_Road_Dimensions(&first_road, 0, 300, 800, 200);
  Set_Road_Dimensions(&second_road, 300, 0, 200, 800);

  /* set the color of the road */
  Error_Checker(SDL_SetRenderDrawColor(renderer, 65, 65, 65, 1), "SDL_SetRenderDrawColor", window);
  Error_Checker(SDL_RenderFillRect(renderer, &first_road), "SDL_RenderFillRect", window);
  Error_Checker(SDL_RenderFillRect(renderer, &second_road), "SDL_RenderFillRect", window);

  /* update the screen with the latest renderings */
  SDL_RenderPresent(renderer);

  bool running = true;
  while (running) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          running = false;
          break;
      }
    }
  }

  SDL_DestroyWindow(window);
  SDL_Quit();

  return 0;
}

void Error_Checker(int return_value, const char *calling_function_name, SDL_Window *window) {
  if (return_value != 0) {
    Error_Handler(calling_function_name, window);
  }
}

void Error_Handler(const char *calling_function_name, SDL_Window *window) {
  SDL_Log("%s error: %s", calling_function_name, SDL_GetError());
  if (window != NULL) {
    SDL_DestroyWindow(window);
  }
  SDL_Quit();
  exit(1);
}

void Set_Road_Dimensions(SDL_Rect *road, int x, int y, int w, int h) {
  road->x = x;
  road->y = y;
  road->w = w;
  road->h = h;
}
