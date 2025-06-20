#include "utils.h"
#include "front.h"

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
    unsigned char x = 7, y = 0; // player pos

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
                        if (recebe_mensagem(socket, 100000, buffer, sizeof(buffer), sequencia) < 0)
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

                    if (recebe_mensagem(socket, 100000, buffer, sizeof(buffer), sequencia) < 0)
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
