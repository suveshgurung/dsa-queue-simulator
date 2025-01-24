#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_log.h>

int main() {
  if (SDL_Init(SDL_INIT_TIMER | SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    SDL_Log("SDL_Init error: %s", SDL_GetError());
  }

  SDL_Quit();

  return 0;
}
