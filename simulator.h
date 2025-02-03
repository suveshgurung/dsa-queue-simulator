#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <stdbool.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_render.h>

extern bool running;

/* defines */
#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 900

/* function prototypes */
void Error_Checker(int, const char *, SDL_Window *);
void Error_Handler(const char *, SDL_Window *);

void Set_Road_Dimensions(SDL_Rect *, int, int, int, int);
void Render_Roads_Traffic_Lights(SDL_Renderer *, SDL_Window *);

void *Receive_From_Generator(void *);

void Signal_Handler(int);

#endif
