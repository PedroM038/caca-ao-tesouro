#ifndef FRONT_H
#define FRONT_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>
#include "utils.h"

#define GRID_SIZE 8
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define CELL_SIZE 50

typedef struct
{
    int x, y;
} Position;

typedef struct
{
    Position player;
    int treasures_found[GRID_SIZE][GRID_SIZE];
    int visited[GRID_SIZE][GRID_SIZE];
} GameState;

typedef struct
{
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *player_texture;
} Graphics;

// Declarações das funções
int init_graphics(Graphics *gfx);
SDL_Texture *load_texture(Graphics *gfx, const char *path);
void init_game_graphics(GameState *game);
void draw_grid(Graphics *gfx);
void draw_game(Graphics *gfx, GameState *game);
void cleanup_graphics(Graphics *gfx);
int controll(SDL_Event *e);
void atualiza_mapa_local_graphics(GameState *game, unsigned char x, unsigned char y, int tesouro);
void atualiza_pos_local_graphics(GameState *game, unsigned char direcao);

#endif // FRONT_H