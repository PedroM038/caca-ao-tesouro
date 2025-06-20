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

    // Gera posições aleatórias únicas para os 9 números
    int posicoes[TAM * TAM];
    for (int i = 0; i < TAM * TAM; i++) {
        posicoes[i] = i;
    }

    // Embaralha as posições (Fisher–Yates shuffle)
    for (int i = TAM * TAM - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int tmp = posicoes[i];
        posicoes[i] = posicoes[j];
        posicoes[j] = tmp;
    }

    // Coloca os valores de 1 a 9 nas primeiras 9 posições embaralhadas
    for (int k = 0; k < VALORES; k++) {
        int idx = posicoes[k];
        int linha = idx / TAM;
        int coluna = idx % TAM;
        matriz[linha][coluna] = k + 1;
    }
}

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
    unsigned char tipo = 0x0f;  // valor provisorio de teste
  
    
    unsigned char buffer[sizeof (Package) + MAX_DATA];// sera necessario tornar dinamico
    unsigned char backup_receiv_buffer[sizeof (Package) + MAX_DATA];
    unsigned char backup_sent_buffer[sizeof (Package) + MAX_DATA];

    
    int counter = 1;
    unsigned char type, sequence;
    unsigned char map[TAM][TAM];

    preencher_matriz_aleatoria (map);

    unsigned char x = 7, y= 0; //player pos
    int recebido = 1;
    while (1) {        
        printf ("linha = %d coluna =%d\n", x, y);

        do
        {
        } while (recebe_mensagem(socket, 1000, buffer, sizeof(buffer), sequencia) < 0); // espera e reenvia ate obter resposta
        printf ("passou\n");
        sequence = get_sequence(buffer);
        while (checksum(buffer) != buffer[3]) { // checksum incorreto
            package_assembler(buffer, 0, get_sequence(buffer), NACK, NULL);
            if (send(socket, buffer, sizeof(buffer), 0) < 0) {
                perror("Erro ao enviar\n");
                printf("\nerro aqui\n");
                exit(0);
            }
            printf("NACK do %dº pacote enviado\n", counter);

            ssize_t tamanho = recebe_mensagem(socket, 100000, buffer, sizeof(buffer), sequencia); // tempo de espera deve ser maior que timeout de envio
            if (tamanho < 0) {
                perror("Erro ao receber dados");
                exit(0);
            }
            sequence = get_sequence(buffer);
            // usleep (100000); // 100ms
        }
        printf ("sequencia recebida: %d sequencia atual: %d\n", get_sequence(buffer), sequencia);

        if (sequencia == get_sequence (buffer)) {
            switch (get_type (buffer)) {
            case CIMA:
                if (x > 0) {
                    x--;
                    printf ("player moveu para cima\n");
                }
                break;
            case BAIXO:
                if (x < 7) {
                    x++;
                    printf ("player moveu para baixo\n");
                }
                break;
            case DIREITA:
                if (y < 7) {
                    y++;
                    printf ("player moveu para direita\n");
                }
                break;
            case ESQUERDA: 
                if (y > 0) {
                    y--;
                    printf ("player moveu para esquerda\n");
                }
                break;
            default:
                break;
            }
        }

        // envia arquivo
        if (map[x][y]) {
            int erro = 0;
            printf ("PLAYER ENCONTROU OBJETO!!!\n");

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
                    if (checksum(buffer) == buffer[3] && get_sequence (buffer) == sequencia) 
                        package_assembler (buffer, 64, get_sequence (buffer), TEXTO_ACK_NOME, arquivo);
                    else {
                        package_assembler (buffer, 0, get_sequence (buffer), NACK, NULL);
                    }
                    
                    if (send(socket, buffer, sizeof(buffer), 0) < 0) {
                        perror("Erro ao enviar\n");
                        exit(0);
                    }
                    printf("%dº Pacote enviado\n", counter);
                    // for (int i = 4; i < 4 + get_size(buffer); ++i) putchar (buffer[i]);
                    // usleep (100000); // 100ms
                } while (recebe_mensagem(socket, 1000, buffer, sizeof(buffer), sequencia) < 0); // espera e reenvia ate obter resposta
                type = get_type(buffer);
                // usleep (100000); // 100ms
            } while (checksum(buffer) != buffer[3] && get_type (buffer) != ACK); // checksum incorreto ou erro no reenvio
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
                    printf("%dº Pacote enviado\n", counter);
                    // for (int i = 4; i < 4 + get_size(buffer); ++i) putchar (buffer[i]);
                    // usleep (100000); // 100ms
                } while (recebe_mensagem(socket, 1000, buffer, sizeof(buffer), sequencia) < 0); // espera e reenvia ate obter resposta
                type = get_type(buffer);
                // usleep (100000); // 100ms
            } while (checksum(buffer) != buffer[3] && get_type(buffer) == NACK); // checksum incorreto ou erro no reenvio

            sequencia = (sequencia + 1) % 32;

            // verifica se ha erro antes de trasmitir arquivo
            if (get_type (buffer) == ERRO) {
                printf ("Erro para envio de arquivo!\n");
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
                        printf ("%dº Pacote enviado\n", counter);
                        //usleep (100000); // 100ms 
                    } while (recebe_mensagem (socket, 100000, buffer, sizeof (buffer), sequencia) < 0);// espera e reenvia ate obter resposta

                    type = get_type (buffer);
                    //usleep (100000); // 100ms 
                } while (checksum (buffer) != buffer[3] || type != ACK); // checksum incorreto ou erro no reenvio

                if (sequencia == get_sequence (buffer)) {
                    bytes_read = read_data (fd_read, read_buffer, MAX_DATA);

                    printf ("enviando dados\n");
                    sequencia = (sequencia + 1) % 32;
                }

                counter++;
            }

            fclose (fd_read);

            do {
                do {
                    package_assembler(buffer, 0, sequencia, FIM_ARQUIVO, NULL);
                    memcpy(&backup_sent_buffer, &buffer, sizeof(Package));

                    if (send(socket, buffer, sizeof(buffer), 0) < 0) {
                        perror("Erro ao enviar\n");
                        exit(0);
                    }
                    printf("%dº Pacote enviado\n", counter);
                    // for (int i = 4; i < 4 + get_size(buffer); ++i) putchar (buffer[i]);
                    // usleep (100000); // 100ms
                } while (recebe_mensagem(socket, 1000, buffer, sizeof(buffer), sequencia) < 0); // espera e reenvia ate obter resposta
                memcpy(&backup_receiv_buffer, &buffer, sizeof(Package) + get_size(buffer));

                type = get_type(buffer);
                // usleep (100000); // 100ms
            } while (checksum(buffer) != buffer[3] && type == NACK);

            recebido = 1;
            sequencia = (sequencia + 1) % 32;
        }
        else {
            if (sequencia == get_sequence (buffer)) {
                sequencia = (sequencia + 1) % 32;
            }

            if (checksum(buffer) == buffer[3])
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
