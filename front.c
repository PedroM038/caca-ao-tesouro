#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdlib.h>

#define GRID_SIZE 8
#define WINDOW_WIDTH 640
#define WINDOW_HEIGHT 480
#define CELL_SIZE 50

typedef struct {
    int x, y;
} Position;

typedef struct {
    Position player;
    int treasures_found[GRID_SIZE][GRID_SIZE];
    int visited[GRID_SIZE][GRID_SIZE];
} GameState;

typedef struct {
    SDL_Window* window;
    SDL_Renderer* renderer;
    SDL_Texture* player_texture;
} Graphics;

int init_graphics(Graphics* gfx) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    gfx->window = SDL_CreateWindow("Caça ao Tesouro",
                                   SDL_WINDOWPOS_UNDEFINED,
                                   SDL_WINDOWPOS_UNDEFINED,
                                   WINDOW_WIDTH, WINDOW_HEIGHT,
                                   SDL_WINDOW_SHOWN);
    
    if (gfx->window == NULL) {
        printf("Window could not be created! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    gfx->renderer = SDL_CreateRenderer(gfx->window, -1, SDL_RENDERER_ACCELERATED);
    if (gfx->renderer == NULL) {
        printf("Renderer could not be created! SDL_Error: %s\n", SDL_GetError());
        return 0;
    }

    // Inicializa SDL_image
    if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG)) {
        printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError());
        return 0;
    }

    return 1;
}

SDL_Texture* load_texture(Graphics* gfx, const char* path) {
    SDL_Surface* loaded_surface = IMG_Load(path);
    if (loaded_surface == NULL) {
        printf("Unable to load image %s! SDL_image Error: %s\n", path, IMG_GetError());
        return NULL;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(gfx->renderer, loaded_surface);
    SDL_FreeSurface(loaded_surface);
    
    if (texture == NULL) {
        printf("Unable to create texture from %s! SDL Error: %s\n", path, SDL_GetError());
    }

    return texture;
}

void init_game(GameState* game) {
    // Inicializa arrays
    for(int i = 0; i < GRID_SIZE; i++) {
        for(int j = 0; j < GRID_SIZE; j++) {
            game->treasures_found[i][j] = 0;
            game->visited[i][j] = 0;
        }
    }
    
    // Posição inicial do jogador (canto inferior esquerdo = 0,0)
    game->player.x = 0;
    game->player.y = 0;
}

void draw_grid(Graphics* gfx) {
    // Desenha linhas do grid
    SDL_SetRenderDrawColor(gfx->renderer, 100, 100, 100, 255); // Cinza
    
    // Linhas verticais
    for(int x = 0; x <= GRID_SIZE; x++) {
        int screen_x = 50 + x * CELL_SIZE;
        SDL_RenderDrawLine(gfx->renderer, screen_x, 50, screen_x, 50 + GRID_SIZE * CELL_SIZE);
    }
    
    // Linhas horizontais
    for(int y = 0; y <= GRID_SIZE; y++) {
        int screen_y = 50 + y * CELL_SIZE;
        SDL_RenderDrawLine(gfx->renderer, 50, screen_y, 50 + GRID_SIZE * CELL_SIZE, screen_y);
    }
}

void draw_game(Graphics* gfx, GameState* game) {
    // Limpa tela com cor de fundo
    SDL_SetRenderDrawColor(gfx->renderer, 30, 30, 30, 255); // Cinza escuro
    SDL_RenderClear(gfx->renderer);
    
    // Desenha o grid
    draw_grid(gfx);
    
    // Desenha posições visitadas
    SDL_SetRenderDrawColor(gfx->renderer, 80, 80, 80, 255); // Cinza médio
    for(int x = 0; x < GRID_SIZE; x++) {
        for(int y = 0; y < GRID_SIZE; y++) {
            if(game->visited[x][y]) {
                SDL_Rect rect = {
                    50 + x * CELL_SIZE + 2,
                    50 + (7-y) * CELL_SIZE + 2,
                    CELL_SIZE - 4,
                    CELL_SIZE - 4
                };
                SDL_RenderFillRect(gfx->renderer, &rect);
            }
        }
    }
    
    // Desenha tesouros encontrados
    SDL_SetRenderDrawColor(gfx->renderer, 255, 215, 0, 255); // Dourado
    for(int x = 0; x < GRID_SIZE; x++) {
        for(int y = 0; y < GRID_SIZE; y++) {
            if(game->treasures_found[x][y]) {
                SDL_Rect rect = {
                    50 + x * CELL_SIZE + 10,
                    50 + (7-y) * CELL_SIZE + 10,
                    CELL_SIZE - 20,
                    CELL_SIZE - 20
                };
                SDL_RenderFillRect(gfx->renderer, &rect);
            }
        }
    }
    
    // Desenha jogador
    SDL_SetRenderDrawColor(gfx->renderer, 0, 255, 0, 255); // Verde
    SDL_Rect player_rect = {
        50 + game->player.x * CELL_SIZE + 5,
        50 + (7 - game->player.y) * CELL_SIZE + 5,
        CELL_SIZE - 10,
        CELL_SIZE - 10
    };
    
    if(gfx->player_texture) {
        SDL_RenderCopy(gfx->renderer, gfx->player_texture, NULL, &player_rect);
    } else {
        SDL_RenderFillRect(gfx->renderer, &player_rect);
    }
    
    SDL_RenderPresent(gfx->renderer);
}

int move_player(GameState* game, int dx, int dy) {
    int new_x = game->player.x + dx;
    int new_y = game->player.y + dy;
    
    // Verifica limites
    if(new_x >= 0 && new_x < GRID_SIZE && new_y >= 0 && new_y < GRID_SIZE) {
        // Marca posição atual como visitada
        game->visited[game->player.x][game->player.y] = 1;
        
        // Move jogador
        game->player.x = new_x;
        game->player.y = new_y;
        
        return 1; // Movimento válido
    }
    
    return 0; // Movimento inválido
}

void add_treasure(GameState* game, int x, int y) {
    if(x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE) {
        game->treasures_found[x][y] = 1;
    }
}

void cleanup(Graphics* gfx) {
    if(gfx->player_texture) {
        SDL_DestroyTexture(gfx->player_texture);
    }
    SDL_DestroyRenderer(gfx->renderer);
    SDL_DestroyWindow(gfx->window);
    IMG_Quit();
    SDL_Quit();
}

int main() {
    Graphics gfx = {0};
    GameState game;
    SDL_Event e;
    int quit = 0;
    
    if(!init_graphics(&gfx)) {
        return 1;
    }
    
    // Tenta carregar imagem do jogador (opcional)
    gfx.player_texture = load_texture(&gfx, "player.png");
    if(!gfx.player_texture) {
        printf("Usando retângulo para jogador (player.png não encontrado)\n");
    }
    
    init_game(&game);
    
    // Adiciona alguns tesouros para teste
    add_treasure(&game, 3, 4);
    add_treasure(&game, 6, 2);
    add_treasure(&game, 1, 7);
    
    while(!quit) {
        while(SDL_PollEvent(&e) != 0) {
            if(e.type == SDL_QUIT) {
                quit = 1;
            }
            else if(e.type == SDL_KEYDOWN) {
                switch(e.key.keysym.sym) {
                    case SDLK_w:
                        move_player(&game, 0, 1); // Move para cima
                        break;
                    case SDLK_s:
                        move_player(&game, 0, -1); // Move para baixo
                        break;
                    case SDLK_a:
                        move_player(&game, -1, 0); // Move para esquerda
                        break;
                    case SDLK_d:
                        move_player(&game, 1, 0); // Move para direita
                        break;
                    case SDLK_q:
                        quit = 1;
                        break;
                }
            }
        }
        
        draw_game(&gfx, &game);
        SDL_Delay(16); // ~60 FPS
    }
    
    cleanup(&gfx);
    return 0;
}