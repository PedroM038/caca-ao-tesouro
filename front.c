#include <stdio.h>
#include "front.h"
#include "utils.h"

int init_graphics(Graphics *gfx)
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    gfx->window = SDL_CreateWindow("Caça ao Tesouro",
                                   SDL_WINDOWPOS_UNDEFINED,
                                   SDL_WINDOWPOS_UNDEFINED,
                                   WINDOW_WIDTH, WINDOW_HEIGHT,
                                   SDL_WINDOW_SHOWN);

    if (gfx->window == NULL)
    {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    gfx->renderer = SDL_CreateRenderer(gfx->window, -1, SDL_RENDERER_ACCELERATED);
    if (gfx->renderer == NULL)
    {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))
    {
        printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
        return 0;
    }

    return 1;
}

SDL_Texture *load_texture(Graphics *gfx, const char *path)
{
    SDL_Surface *loaded_surface = IMG_Load(path);
    if (loaded_surface == NULL)
    {
        printf("Unable to load image %s! SDL_image Error: %s\n", path, IMG_GetError());
        return NULL;
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(gfx->renderer, loaded_surface);
    SDL_FreeSurface(loaded_surface);

    if (texture == NULL)
    {
        printf("Unable to create texture from %s! SDL Error: %s\n", path, SDL_GetError());
    }

    return texture;
}

void init_game_graphics(GameState *game)
{
    // Inicializa arrays
    for (int i = 0; i < GRID_SIZE; i++)
    {
        for (int j = 0; j < GRID_SIZE; j++)
        {
            game->treasures_found[i][j] = 0;
            game->visited[i][j] = 0;
        }
    }

    // Posição inicial do jogador (canto inferior esquerdo = 0,0)
    game->player.x = 7;
    game->player.y = 0;
}

void draw_grid(Graphics *gfx)
{
    SDL_SetRenderDrawColor(gfx->renderer, 100, 100, 100, 255);

    for (int x = 0; x <= GRID_SIZE; x++)
    {
        int screen_x = 50 + x * CELL_SIZE;
        SDL_RenderDrawLine(gfx->renderer, screen_x, 50, screen_x, 50 + GRID_SIZE * CELL_SIZE);
    }

    for (int y = 0; y <= GRID_SIZE; y++)
    {
        int screen_y = 50 + y * CELL_SIZE;
        SDL_RenderDrawLine(gfx->renderer, 50, screen_y, 50 + GRID_SIZE * CELL_SIZE, screen_y);
    }
}

void draw_game(Graphics *gfx, GameState *game)
{
    SDL_SetRenderDrawColor(gfx->renderer, 30, 30, 30, 255);
    SDL_RenderClear(gfx->renderer);

    draw_grid(gfx);

    // Desenha posições visitadas
    SDL_SetRenderDrawColor(gfx->renderer, 80, 80, 80, 255);
    for (int x = 0; x < GRID_SIZE; x++)
    {
        for (int y = 0; y < GRID_SIZE; y++)
        {
            if (game->visited[x][y])
            {
                SDL_Rect rect = {
                    50 + y * CELL_SIZE + 2,  // y é coluna (horizontal)
                    50 + x * CELL_SIZE + 2,  // x é linha (vertical)
                    CELL_SIZE - 4,
                    CELL_SIZE - 4};
                SDL_RenderFillRect(gfx->renderer, &rect);
            }
        }
    }

    // Desenha tesouros encontrados
    SDL_SetRenderDrawColor(gfx->renderer, 255, 215, 0, 255);
    for (int x = 0; x < GRID_SIZE; x++)
    {
        for (int y = 0; y < GRID_SIZE; y++)
        {
            if (game->treasures_found[x][y])
            {
                SDL_Rect rect = {
                    50 + y * CELL_SIZE + 10,  // y é coluna (horizontal)
                    50 + x * CELL_SIZE + 10,  // x é linha (vertical)
                    CELL_SIZE - 20,
                    CELL_SIZE - 20};
                SDL_RenderFillRect(gfx->renderer, &rect);
            }
        }
    }

    // Desenha jogador
    SDL_SetRenderDrawColor(gfx->renderer, 0, 255, 0, 255);
     SDL_Rect player_rect = {
        50 + game->player.y * CELL_SIZE + 5,      // coluna -> x da tela
        50 + game->player.x * CELL_SIZE + 5,      // linha -> y da tela
        CELL_SIZE - 10,
        CELL_SIZE - 10};

    if (gfx->player_texture)
    {
        SDL_RenderCopy(gfx->renderer, gfx->player_texture, NULL, &player_rect);
    }
    else
    {
        SDL_RenderFillRect(gfx->renderer, &player_rect);
    }

    SDL_RenderPresent(gfx->renderer);
}

void cleanup_graphics(Graphics *gfx)
{
    if (gfx->player_texture)
    {
        SDL_DestroyTexture(gfx->player_texture);
    }
    SDL_DestroyRenderer(gfx->renderer);
    SDL_DestroyWindow(gfx->window);
    IMG_Quit();
    SDL_Quit();
}

// Modificar a função controll para usar SDL2
int controll(SDL_Event *e)
{
    if (SDL_PollEvent(e))
    {
        if (e->type == SDL_QUIT)
        {
            return -1; // Sair do jogo
        }
        else if (e->type == SDL_KEYDOWN)
        {
            switch (e->key.keysym.sym)
            {
            case SDLK_w:
                return CIMA;
            case SDLK_s:
                return BAIXO;
            case SDLK_d:
                return DIREITA;
            case SDLK_a:
                return ESQUERDA;
            case SDLK_q:
                return -1; // Sair do jogo
            }
        }
    }
    return 0; // Nenhuma tecla pressionada
}

// Modificar a função atualiza_mapa_local para usar GameState
void atualiza_mapa_local_graphics(GameState *game, unsigned char x, unsigned char y, int tesouro)
{
    if (tesouro)
        game->treasures_found[x][y] = 1;

    game->visited[x][y] = 1;
}

// Modificar a função atualiza_pos_local para usar GameState
void atualiza_pos_local_graphics(GameState *game, unsigned char direcao)
{
    switch (direcao)
    {
    case CIMA:
        if (game->player.x > 0)  // x é linha, diminui para subir
            game->player.x--;
        break;
    case BAIXO:
        if (game->player.x < 7)  // x é linha, aumenta para descer
            game->player.x++;
        break;
    case DIREITA:
        if (game->player.y < 7)  // y é coluna, aumenta para direita
            game->player.y++;
        break;
    case ESQUERDA:
        if (game->player.y > 0)  // y é coluna, diminui para esquerda
            game->player.y--;
        break;
    default:
        break;
    }
}