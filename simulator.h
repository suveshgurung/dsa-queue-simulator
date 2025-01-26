#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <stdlib.h>
#include <stdbool.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_error.h>
#include <SDL2/SDL_log.h>
#include <SDL2/SDL_video.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_rect.h>
#include <SDL2/SDL_image.h>

/* defines */
#define WINDOW_WIDTH 900
#define WINDOW_HEIGHT 900

/* function prototypes */
void Error_Checker(int, const char *, SDL_Window *);
void Error_Handler(const char *, SDL_Window *);

void Set_Road_Dimensions(SDL_Rect *, int, int, int, int);
void Render_Roads_Traffic_Lights(SDL_Renderer *, SDL_Window *);

#endif
