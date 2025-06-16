#include "utils.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

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
    game->player.x = 0;
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
                    50 + x * CELL_SIZE + 2,
                    50 + (7 - y) * CELL_SIZE + 2,
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
                    50 + x * CELL_SIZE + 10,
                    50 + (7 - y) * CELL_SIZE + 10,
                    CELL_SIZE - 20,
                    CELL_SIZE - 20};
                SDL_RenderFillRect(gfx->renderer, &rect);
            }
        }
    }

    // Desenha jogador
    SDL_SetRenderDrawColor(gfx->renderer, 0, 255, 0, 255);
    SDL_Rect player_rect = {
        50 + game->player.x * CELL_SIZE + 5,
        50 + (7 - game->player.y) * CELL_SIZE + 5,
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

// Escreve dados no arquivo ignorando os bytes de escape 0xFF que seguem 0x81, 0x88 ou 0xFF
void write_data(FILE *fd_write, unsigned char *buffer, int size)
{
    int i = 0;

    while (i < size)
    {
        unsigned char byte = buffer[i];
        fputc(byte, fd_write);

        // Caso de byte problematico pula o escape
        if ((byte == 0x81 || byte == 0x88 || byte == 0xFF) &&
            i < size - 1 && buffer[i + 1] == 0xFF)
        {
            i += 2; // salta o byte de escape
        }
        else
        {
            i++;
        }
    }
    fflush(fd_write);
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
        if (game->player.y < 7)
            game->player.y++;
        break;
    case BAIXO:
        if (game->player.y > 0)
            game->player.y--;
        break;
    case DIREITA:
        if (game->player.x < 7)
            game->player.x++;
        break;
    case ESQUERDA:
        if (game->player.x > 0)
            game->player.x--;
        break;
    default:
        break;
    }
}

// retorna -1 se deu timeout, ou quantidade de bytes lidos
int recebe_mensagem(int soquete, int timeoutMillis, char* buffer, int tamanho_buffer, unsigned char sequencia) {
    long long comeco = timestamp();
    struct timeval timeout;
    timeout.tv_sec = timeoutMillis/1000;
    timeout.tv_usec = (timeoutMillis%1000) * 1000;
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, (char*) &timeout, sizeof(timeout));
    int bytes_lidos;
    unsigned char received_sequence;
    do {
        bytes_lidos = recv(soquete, buffer, tamanho_buffer, 0);
        if (protocolo_e_valido(buffer, bytes_lidos, sequencia)) { return bytes_lidos; }
    } while ((timestamp() - comeco <= timeoutMillis));
    return -1;
}

int main(int argc, char **argv)
{

    if (argc < 2)
    {
        printf("deve ser executado ./client <nome da porta>");
        return 1;
    }

    // Inicializa SDL2
    Graphics gfx = {0};
    GameState game;
    SDL_Event e;

    if (!init_graphics(&gfx))
    {
        return 1;
    }

    // Tenta carregar imagem do jogador (opcional)
    gfx.player_texture = load_texture(&gfx, "player.png");
    if (!gfx.player_texture)
    {
        printf("Usando retângulo para jogador (player.png não encontrado)\n");
    }

    init_game_graphics(&game);

    // mapa local do cliente (manter para compatibilidade com código existente)
    unsigned char map[8][8];
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
            map[i][j] = 0;
    unsigned char x = 0, y = 0; // player pos

    int socket = cria_raw_socket(argv[1]);

    unsigned char type;
    unsigned char sequencia = 0;
    unsigned char tipo = 0x00; // valor provisorio de teste
    unsigned char buffer[sizeof(Package) + MAX_DATA];
    unsigned char backup_receiv_buffer[sizeof(Package) + MAX_DATA];
    unsigned char backup_sent_buffer[sizeof(Package) + MAX_DATA];
    int counter = 1;
    int recebido = 1;
    int movimento_pendente = 0;

    while (1)
    {
        printf("linha = %d coluna =%d\n", x, y); // Desenha o jogo a cada frame
        draw_game(&gfx, &game);
        SDL_Delay(16); // ~60 FPS

        // Verifica input apenas se não há movimento pendente
        if (recebido)
        {
            int input = controll(&e);
            if (input == -1)
            {
                break; // Sair do jogo
            }
            if (input != 0)
            {
                tipo = input;
                movimento_pendente = 1;
                recebido = 0;
            }
        }

        // Processa movimento se há um pendente
        if (movimento_pendente)
        {
            atualiza_pos_local_graphics(&game, tipo);
            x = game.player.x;
            y = game.player.y;
            movimento_pendente = 0;
            // Envia movimento e espeara ACK de movimento

            do
            {
                do
                {
                    package_assembler(buffer, 0, sequencia, tipo, NULL);
                    memcpy(&backup_sent_buffer, &buffer, sizeof(Package));

                    if (send(socket, buffer, sizeof(buffer), 0) < 0)
                    {
                        perror("Erro ao enviar\n");
                        exit(0);
                    }
                    printf("%dº Pacote enviado\n", counter);
                } while (recebe_mensagem(socket, 1000, buffer, sizeof(buffer), sequencia) < 0); // espera e reenvia ate obter resposta
                memcpy(&backup_receiv_buffer, &buffer, sizeof(Package) + get_size(buffer));

                type = get_type(buffer);
            } while (checksum(buffer) != buffer[3] && type == NACK /*|| (type != OK_ACK && type != TEXTO_ACK_NOME && type != VIDEO_ACK_NOME && type != IMAGEM_ACK_NOME)*/); // checksum incorreto ou erro no reenvio

            // recebe arquivo
            if ((type == TEXTO_ACK_NOME || type == IMAGEM_ACK_NOME || type == VIDEO_ACK_NOME))
            {

                FILE *fd_write;
                unsigned char erro = 0;
                atualiza_mapa_local_graphics(&game, x, y, 1); // Marca tesouro encontrado

                unsigned char nome_arquivo[get_size(buffer)];
                memcpy(nome_arquivo, &buffer[DATA], get_size(buffer));

                package_assembler(buffer, 0, get_sequence(buffer), ACK, NULL);
                if (send(socket, buffer, sizeof(buffer), 0) < 0)
                {
                    perror("Erro ao enviar\n");
                    exit(0);
                }
                sequencia = (sequencia + 1) % 32;

                // Envia movimento e espeara ACK de movimento
                uint64_t tamanho_livre = espaco_livre(".");
                uint64_t tamanho_arquivo;
                if (recebe_mensagem(socket, 1000, buffer, sizeof(buffer), sequencia) < 0)
                {
                    perror("Timeout\n");
                    exit(0);
                }

                do
                {
                    if (checksum(buffer) == buffer[3])
                    {
                        if (get_type(buffer) == TAMANHO && sequencia == get_sequence(buffer))
                        {
                            // trata tamanho
                            tamanho_arquivo = le_uint64(&buffer[DATA]);
                            fd_write = abre_arquivo(nome_arquivo);
                            if (fd_write != NULL && tamanho_livre > tamanho_arquivo)
                            {
                                package_assembler(buffer, 0, get_sequence(buffer), ACK, NULL);
                                if (send(socket, buffer, sizeof(buffer), 0) < 0)
                                {
                                    perror("Erro ao enviar\n");
                                    exit(0);
                                }
                                printf("%dº Pacote enviado\n", counter);
                            }
                            else
                            { // erro para receber arquivo, envia codigo de erro junto da mensagem
                                erro = 1;
                                unsigned char error_log[64];
                                uint64_t error_code;
                                if (!fd_write)
                                {
                                    error_code = SEM_PERMISSAO_DE_ACESSO;
                                }
                                else
                                {
                                    error_code = ESPACO_INSUFICIENTE;
                                }
                                memcpy(error_log, &error_code, sizeof(uint64_t));
                                package_assembler(buffer, sizeof(uint64_t), get_sequence(buffer), ERRO, error_log);

                                if (send(socket, buffer, sizeof(buffer), 0) < 0)
                                {
                                    perror("Erro ao enviar\n");
                                    exit(0);
                                }
                                printf("%dº Pacote enviado\n", counter);
                            }
                            break;
                        }

                        package_assembler(buffer, 0, get_sequence(buffer), ACK, NULL);
                        if (send(socket, buffer, sizeof(buffer), 0) < 0)
                        {
                            perror("Erro ao enviar\n");
                            exit(0);
                        }
                        printf("%dº Pacote enviado\n", counter);
                    }
                    else
                    {
                        package_assembler(buffer, 0, get_sequence(buffer), NACK, NULL);
                        if (send(socket, buffer, sizeof(buffer), 0) < 0)
                        {
                            perror("Erro ao enviar\n");
                            exit(0);
                        }
                        printf("%dº Pacote enviado\n", counter);
                    }

                    if (recebe_mensagem(socket, 1000, buffer, sizeof(buffer), sequencia) < 0)
                    {
                        perror("Timeout\n");
                        exit(0);
                    }
                } while (checksum(buffer) != buffer[3]);
                sequencia = (sequencia + 1) % 32;

                if (recebe_mensagem(socket, 1000, buffer, sizeof(buffer), sequencia) < 0)
                {
                    perror("Erro ao receber\n");
                    exit(0);
                }

                while (get_type(buffer) != FIM_ARQUIVO)
                {
                    while (checksum(buffer) != buffer[3])
                    { // checksum incorreto ou erro no reenvio

                        package_assembler(buffer, 0, get_sequence(buffer), NACK, NULL);
                        if (send(socket, buffer, sizeof(buffer), 0) < 0)
                        {
                            perror("Erro ao enviar\n");
                            exit(0);
                        }
                        printf("%dº Pacote enviado\n", counter);
                        if (recebe_mensagem(socket, 1000, buffer, sizeof(buffer), sequencia) < 0)
                        {
                            perror("Erro ao enviar\n");
                            exit(0);
                        }
                        type = get_type(buffer);
                    }

                    if (sequencia == get_sequence(buffer) && get_type(buffer) == DADOS)
                    {
                        printf("recebendo dados\n");
                        write_data(fd_write, &buffer[DATA], get_size(buffer));
                        sequencia = (sequencia + 1) % 32;
                    }

                    package_assembler(buffer, 0, get_sequence(buffer), ACK, NULL);
                    if (send(socket, buffer, sizeof(buffer), 0) < 0)
                    {
                        perror("Erro ao enviar\n");
                        exit(0);
                    }

                    if (recebe_mensagem(socket, 1000, buffer, sizeof(buffer), sequencia) < 0)
                    {
                        perror("Erro ao enviar\n");
                        exit(0);
                    }
                }
                fclose(fd_write);
                printf("fim do arquivo\n");

                package_assembler(buffer, 0, get_sequence(buffer), ACK, NULL);
                if (send(socket, buffer, sizeof(buffer), 0) < 0)
                {
                    perror("Erro ao enviar\n");
                    exit(0);
                }

                // abre arquivo se ele foi recebido corretamente
                if (!erro)
                {
                    play(nome_arquivo);
                }

                recebido = 1;
                sequencia = (sequencia + 1) % 32;
            }
            else if (type == OK_ACK)
            {
                atualiza_mapa_local_graphics(&game, x, y, 0);
                recebido = 1;
                sequencia = (sequencia + 1) % 32;
            }
            else
            { // fim do arquivo mal resolvido
                package_assembler(buffer, 0, get_sequence(buffer), ACK, NULL);
                if (send(socket, buffer, sizeof(buffer), 0) < 0)
                {
                    perror("Erro ao enviar\n");
                    exit(0);
                }
            }
        }
    }
    cleanup_graphics(&gfx);
    return 0;
}
