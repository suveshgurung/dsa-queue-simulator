#include "simulator.h"

int main() {
  if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    SDL_Log("SDL_Init error: %s", SDL_GetError());
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
  Error_Checker(SDL_RenderDrawLine(renderer, WINDOW_WIDTH/2 - 35, 0, WINDOW_WIDTH/2 -35, WINDOW_HEIGHT/2 - 105), "SDL_RenderDrawLine", window);
  Error_Checker(SDL_RenderDrawLine(renderer, WINDOW_WIDTH/2 + 35, 0, WINDOW_WIDTH/2 + 35, WINDOW_HEIGHT/2 - 105), "SDL_RenderDrawLine", window);
  Error_Checker(SDL_RenderDrawLine(renderer, WINDOW_WIDTH/2 - 35, WINDOW_HEIGHT/2 + 105, WINDOW_WIDTH/2 - 35, WINDOW_HEIGHT), "SDL_RenderDrawLine", window);
  Error_Checker(SDL_RenderDrawLine(renderer, WINDOW_WIDTH/2 + 35, WINDOW_HEIGHT/2 + 105, WINDOW_WIDTH/2 + 35, WINDOW_HEIGHT), "SDL_RenderDrawLine", window);

  /* update the screen with the latest renderings */
  SDL_RenderPresent(renderer);
}
