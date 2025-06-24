#include "utils.h"

// Le arquivo byte a byte verificando se ha bytes 0X81 ou 0x88 e insere 0xff quando isso ocorre
// Retorna o tamanho do buffer lido
int read_data (FILE* fd_read, unsigned char* read_buffer, int max_size) {
    unsigned char byte;
    int i = 0;
    int b;

    while(i < max_size) { // enquanto ouver espaco para inserir escape bytes
        b = fgetc(fd_read);
        if (b == EOF)
            break;

        byte = (unsigned char)b;
        read_buffer[i++] = byte;

        if ((byte == 0x88 || byte == 0x81 || byte == 0xFF)) {
            if (i >= max_size) {
                // nao houve espaco para inserir o byte problematico e o escape 
                // desfaz leitura do byte para próxima chamada
                ungetc(byte, fd_read);
                i--; // desfaz escrita do byte
                break;
            }
            read_buffer[i++] = ESCAPE_BYTE;
        }
    }
    return i;
}

// geração do mapa
void preencher_matriz_aleatoria(unsigned char matriz[TAM][TAM]) {
    // Inicializa toda a matriz com zeros
    for (int i = 0; i < TAM; i++) {
        for (int j = 0; j < TAM; j++) {
            matriz[i][j] = 0;
        }
    }

    // Gera posições aleatórias únicas para os números
    int posicoes[TAM * TAM];
    for (int i = 0; i < TAM * TAM; i++) {
        posicoes[i] = i;
    }

    // Embaralha as posições
    for (int i = TAM * TAM - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = posicoes[i];
        posicoes[i] = posicoes[j];
        posicoes[j] = tmp;
    }

    // Coloca os valores de 1 a 8 nas primeiras posições embaralhadas
    for (int k = 0; k < VALORES; k++) {
        int idx = posicoes[k];
        int linha = idx / TAM;
        int coluna = idx % TAM;
        matriz[linha][coluna] = k + 1;
    }

    for (int i = 0; i < TAM; ++i) {
        for (int j = 0; j < TAM; ++j) {
            printf ("%d ", matriz[i][j]);
        }
        printf ("\n");
    }
}

// retorna ponteiro do aruqivo
// argumento tera nome do arquivo e caminho ate o arquivo
FILE *abrir_arquivo_desconhecido(int numero, char *nome_saida, char *caminho_saida) {
    const char *exts[] = {"txt", "mp4", "jpg" };
    char nome[64];

    for (size_t i = 0; i < sizeof(exts) / sizeof(exts[0]); i++) {
        snprintf(nome, sizeof(nome), "%d.%s", numero, exts[i]);
        snprintf(caminho_saida, 128, "objetos/%s", nome);
        FILE *f = fopen(caminho_saida, "rb");
        if (f != NULL) {
            // Copia o nome encontrado para o argumento de saída
            strncpy(nome_saida, nome, TAMANHO_NOME_ARQUIVO - 1);
            nome_saida[TAMANHO_NOME_ARQUIVO - 1] = '\0'; // garante fim
            nome_saida[2*TAMANHO_NOME_ARQUIVO - 1] = '\0'; // garante fim
            return f;
        }
    }

    // Se nenhum arquivo foi encontrado, zera nome_saida
    if (nome_saida && TAMANHO_NOME_ARQUIVO > 0) {
        nome_saida[0] = '\0';
        caminho_saida[0] = '\0';
    }

    return NULL;
}

int main ( int argc, char** argv ) {
    if (argc < 2) { 
		printf ("executar ./server <nome da porta> <nome_do_arquivo_fonte>\n"); 
		return 1;
	}
    int socket = cria_raw_socket (argv[1]);
    
    unsigned char sequencia = 0;
    
    unsigned char buffer[sizeof (Package) + MAX_DATA];// sera necessario tornar dinamico
    unsigned char backup_receiv_buffer[sizeof (Package) + MAX_DATA];
    unsigned char backup_sent_buffer[sizeof (Package) + MAX_DATA];

    unsigned char type, sequence;
    unsigned char map[TAM][TAM];

    srand ((unsigned) time(NULL));
    preencher_matriz_aleatoria (map);

    unsigned char x = 7, y = 0; //player pos
    printf ("linha = %d coluna = %d\n", x, y);
    int recebido = 1;
    while (1) {        
        while (recebe_mensagem(socket, TEMPO_REENVIO, buffer, sizeof(buffer), sequencia) < 0) {} // espera e reenvia ate obter resposta
        sequence = get_sequence(buffer);
        while (checksum(buffer) != buffer[CHECKSUM]) { // checksum incorreto
            package_assembler(buffer, 0, get_sequence(buffer), NACK, NULL);
            if (send(socket, buffer, sizeof(buffer), 0) < 0) {
                perror("Erro ao enviar\n");
                exit(0);
            }

            if (recebe_mensagem(socket, TIMEOUT, buffer, sizeof(buffer), sequencia) < 0) { // tempo de espera deve ser maior que timeout de envio
                perror("Timeout\n");
                exit(0);
            }
            sequence = get_sequence(buffer);
        }
        printf ("sequencia recebida: %d sequencia atual: %d\n", get_sequence(buffer), sequencia);

        if (sequencia == get_sequence (buffer)) {
            switch (get_type (buffer)) {
            case CIMA:
                if (x > 0) {
                    x--;
                    printf ("player moveu para cima\n");
                    printf ("linha = %d coluna = %d\n", x, y);
                }
                break;
            case BAIXO:
                if (x < 7) {
                    x++;
                    printf ("player moveu para baixo\n");
                    printf ("linha = %d coluna = %d\n", x, y);
                }
                break;
            case DIREITA:
                if (y < 7) {
                    y++;
                    printf ("player moveu para direita\n");
                    printf ("linha = %d coluna = %d\n", x, y);
                }
                break;
            case ESQUERDA: 
                if (y > 0) {
                    y--;
                    printf ("player moveu para esquerda\n");
                    printf ("linha = %d coluna = %d\n", x, y);
                }
                break;
            default:
                break;
            }
        }

        // envia arquivo
        if (map[x][y]) {
            int erro = 0;
            printf ("PLAYER ENCONTROU TESOURO!!!\n");

            unsigned char arquivo[TAMANHO_NOME_ARQUIVO]; // nome do arquivo
            unsigned char caminho[2*TAMANHO_NOME_ARQUIVO]; // caminho ate o arquivo + nome do arquivo
            FILE *fd_read = abrir_arquivo_desconhecido (map[x][y], arquivo, caminho); 
	        map[x][y] = 0;
            if (fd_read == NULL) { 
                printf ("falha ao abrir arquivo\n"); 
                erro = 1;
            }

            // envia o nome do arquivo
            do {
                do {
                    if (checksum(buffer) == buffer[CHECKSUM] && get_sequence (buffer) == sequencia) 
                        package_assembler (buffer, 64, get_sequence (buffer), TEXTO_ACK_NOME, arquivo);
                    else {
                        package_assembler (buffer, 0, get_sequence (buffer), NACK, NULL);
                    }
                    
                    if (send(socket, buffer, sizeof(buffer), 0) < 0) {
                        perror("Erro ao enviar\n");
                        exit(0);
                    }
                } while (recebe_mensagem(socket, TEMPO_REENVIO, buffer, sizeof(buffer), sequencia) < 0); // espera e reenvia ate obter resposta
                type = get_type(buffer);
            } while (checksum(buffer) != buffer[CHECKSUM] && get_type (buffer) != ACK); // checksum incorreto ou erro no reenvio
            sequencia = (sequencia + 1) % 32;

            uint64_t tamanho = tamanho_arquivo(caminho); 
            // envia tamanho do arquivo
            if (tamanho == 0) {
                erro = 1;
            }

            unsigned char size_buffer[sizeof(uint64_t)];
            memcpy(size_buffer, &tamanho, sizeof(uint64_t));
            do
            {
                do
                {
                    package_assembler(buffer, sizeof(uint64_t), sequencia, TAMANHO, size_buffer);
                    if (send(socket, buffer, sizeof(buffer), 0) < 0)
                    {
                        perror("Erro ao enviar\n");
                        exit(0);
                    }
                } while (recebe_mensagem(socket, TEMPO_REENVIO, buffer, sizeof(buffer), sequencia) < 0); // espera e reenvia ate obter resposta
                type = get_type(buffer);
            } while (checksum(buffer) != buffer[CHECKSUM] && get_type(buffer) == NACK); // checksum incorreto ou erro no reenvio

            sequencia = (sequencia + 1) % 32;

            // verifica se ha erro antes de trasmitir arquivo
            if (get_type (buffer) == ERRO) {
                uint64_t error_code = le_uint64 (&buffer[DATA]);
                printf ("Erro para envio de arquivo!\n");
                if (error_code == ESPACO_INSUFICIENTE) {
                    printf ("Espaço insuficiente!\n");
                }
                else if (error_code == SEM_PERMISSAO_DE_ACESSO) {
                    printf ("Cliente sem permissao de acesso!\n");
                }
                
                erro = 1;
            }
            else if (get_type (buffer) == ACK && !erro) {
                printf ("Arquivo pronto para envio!\n");
                erro = 0;
            }
            
            // envio de arquivo caso nao haja erro
            unsigned char read_buffer [MAX_DATA];
            int bytes_read;

            if (!erro)
                bytes_read = read_data (fd_read, read_buffer, MAX_DATA);
            
            unsigned char type;
            while (!erro && bytes_read > 0) {                
                do {
                    do {
                        package_assembler (buffer, bytes_read, sequencia, DADOS, read_buffer);
                        if (send (socket, buffer, sizeof (buffer), 0) < 0) {
                            perror ("Erro ao enviar\n");
                            exit (0);
                        }
                    } while (recebe_mensagem (socket, TEMPO_REENVIO, buffer, sizeof (buffer), sequencia) < 0);// espera e reenvia ate obter resposta
                    type = get_type (buffer);
                    
                } while (checksum (buffer) != buffer[CHECKSUM] || type != ACK); // checksum incorreto ou erro no reenvio

                if (sequencia == get_sequence (buffer)) {
                    bytes_read = read_data (fd_read, read_buffer, MAX_DATA);
                    sequencia = (sequencia + 1) % 32;
                }
            }

            fclose (fd_read);
            printf ("fim de arquivo\n");
            do {
                do {
                    package_assembler(buffer, 0, sequencia, FIM_ARQUIVO, NULL);
                    memcpy(&backup_sent_buffer, &buffer, sizeof(Package));

                    if (send(socket, buffer, sizeof(buffer), 0) < 0) {
                        perror("Erro ao enviar\n");
                        exit(0);
                    }                   
                } while (recebe_mensagem(socket, TEMPO_REENVIO, buffer, sizeof(buffer), sequencia) < 0); // espera e reenvia ate obter resposta
                memcpy(&backup_receiv_buffer, &buffer, sizeof(Package) + get_size(buffer));

                type = get_type(buffer);
                
            } while (checksum(buffer) != buffer[CHECKSUM] && type == NACK);

            recebido = 1;
            sequencia = (sequencia + 1) % 32;
        }
        else {
            if (sequencia == get_sequence (buffer)) {
                sequencia = (sequencia + 1) % 32;
            }

            if (checksum(buffer) == buffer[CHECKSUM])
                package_assembler(buffer, 0, get_sequence(buffer), OK_ACK, NULL);
            else {
                package_assembler(buffer, 0, get_sequence(buffer), NACK, NULL);
            }
            if (send(socket, buffer, sizeof(buffer), 0) < 0) {
                perror("Erro ao enviar\n");
                exit(0);
            }
        }
    }
    return 0;

}
