#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/statvfs.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>

//#define SOL_SOCKET 1
//#define SO_RCVTIMEO 20
#define ESCAPE_BYTE 0xFF

#define DATA 4
#define MAX_DATA 127
#define MIN_SIZE 22

#define ACK 0
#define NACK 1
#define OK_ACK 2
#define LIVRE 3
#define TAMANHO 4
#define DADOS 5
#define TEXTO_ACK_NOME 6
#define VIDEO_ACK_NOME 7
#define IMAGEM_ACK_NOME 8
#define FIM_ARQUIVO 9
#define DIREITA 10
#define CIMA 11
#define BAIXO 12
#define ESQUERDA 13
#define LIVRE_2 14
#define ERRO 15

#define SEM_PERMISSAO_DE_ACESSO 0
#define ESPACO_INSUFICIENTE 1
//struct timeval {
 //   int tv_sec;
  //  int tv_usec;
//};
/*
const int timeoutMillis = 300; // 300 milisegundos de timeout por exemplo
struct timeval timeout = { .tv_sec = timeoutMillis / 1000, .tv_usec = (timeoutMillis % 1000) * 1000 };
setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, (char*) &timeout, sizeof(timeout));
 */


typedef struct Package {
    unsigned char start;
    unsigned char info_upper;
    unsigned char info_down;
    unsigned char checksum;
    //unsigned char *data; //0 - 127 
} Package;

int cria_raw_socket(char* nome_interface_rede) {
    // Cria arquivo para o socket sem qualquer protocolo
    int soquete = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (soquete == -1) {
        fprintf(stderr, "Erro ao criar socket: Verifique se você é root!\n");
        exit(-1);
    }
 
    int ifindex = if_nametoindex(nome_interface_rede);
 
    struct sockaddr_ll endereco = {0};
    endereco.sll_family = AF_PACKET;
    endereco.sll_protocol = htons(ETH_P_ALL);
    endereco.sll_ifindex = ifindex;
    // Inicializa socket
    if (bind(soquete, (struct sockaddr*) &endereco, sizeof(endereco)) == -1) {
        fprintf(stderr, "Erro ao fazer bind no socket\n");
        exit(-1);
    }
 
    struct packet_mreq mr = {0};
    mr.mr_ifindex = ifindex;
    mr.mr_type = PACKET_MR_PROMISC;
    // Não joga fora o que identifica como lixo: Modo promíscuo
    if (setsockopt(soquete, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1) {
        fprintf(stderr, "Erro ao fazer setsockopt: "
            "Verifique se a interface de rede foi especificada corretamente.\n");
        exit(-1);
    }
 
    return soquete;
}

long long timestamp() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_sec*1000 + tp.tv_usec/1000;
}

unsigned char get_size (unsigned char *buffer) {
    return (buffer[1] >> 1);
}
 
unsigned char get_sequence (unsigned char *buffer) {
    return (((0x01 & buffer[1]) << 4) | ((0xf0 & buffer[2]) >> 4)); // junta o primeiro bit de buffer[1] com os 4 mais significativos de buffer[2]
}

unsigned char get_type (unsigned char *buffer) {
    return (0xf & buffer[2]);
}
 
int protocolo_e_valido(char* buffer, int tamanho_buffer, unsigned char sequencia) {
    if (tamanho_buffer <= 0) { return 0; }
    // insira a sua validação de protocolo aqui
    return (buffer[0] == 0x7e);
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
// realiza soma byte a byte do buffer
unsigned char checksum (unsigned char *buffer) {
    unsigned int sum = buffer[1] + buffer[2]; //tamanho + sequencia + tipo
    int size = buffer[1] >> 1; // tamanho do buffer
    for (int i = 4; i <  4 + size; ++i) { // comeca depois do byte do checksum
        sum+= buffer[i];
    }
    return (unsigned char) (0xff & sum);
}

void package_assembler (unsigned char *buffer, unsigned char size, unsigned char sequence, unsigned char type, unsigned char *data) {
    buffer[0] = 0x7e; // bits de inicio
    buffer[1] = (0x7f & size) << 1; // move o tamanho 1 bit para esquerda e garante que somente os 7 bits menos signficarivos sejam escritos
    buffer[1] = buffer[1] | ((0x10 & sequence) >> 4); // adinciona em buffer[1] apenas o quinto bit de sequencia
    buffer[2] = (0x0f & sequence) << 4; // os 4 bits mais significativos de buffer[2] recebem os primeiros bits de sequencia
    buffer[2] = buffer[2] | (0x0f & type); // os 4 primieros bits de tipos sao guardados em buffer[2]
    if (data == NULL || size == 0)
        memset(&buffer[DATA], 0, MIN_SIZE);
    else    
        memcpy (&buffer[DATA], data, size);
    buffer[3] = checksum (buffer);
}


void print_data (unsigned char* buffer) {
    unsigned int string_size = get_size(buffer) + 1; // adiciona espaco para /0
    unsigned char string[string_size];
    memcpy (string, &buffer[DATA], string_size -1); // copia sem considerar /0
    string[string_size-1] = '\0'; 
    fprintf(stdout, "%s", string);
    //printf ("%s", string);
}

// Escreve dados no arquivo ignorando os bytes de escape 0xFF que seguem 0x81, 0x88 ou 0xFF
void write_data(FILE* fd_write, unsigned char* buffer, int size) {
    int i = 0;

    while (i < size) {
        unsigned char byte = buffer[i];
        fputc(byte, fd_write);

        // Caso de byte problematico pula o escape
        if ((byte == 0x81 || byte == 0x88 || byte == 0xFF) &&
            i < size - 1 && buffer[i + 1] == 0xFF) {
            i += 2; // salta o byte de escape
        } else {
            i++;
        }
    }
    fflush(fd_write);
}

// Retorna espaço livre em bytes, ou -1 em caso de erro
int64_t espaco_livre(unsigned char *caminho) {
    struct statvfs st;

    if (statvfs(caminho, &st) == 0) {
        return (int64_t)st.f_bsize * (int64_t)st.f_bavail;
    } else {
        // Erro ao acessar informações do sistema de arquivos
        return -1;
    }
}

uint64_t le_uint64(unsigned char *src) {
    uint64_t resultado = 0;
    for (int i = sizeof(uint64_t) - 1; i >= 0; --i) {
        resultado <<= 8;
        resultado |= src[i];
    }
    return resultado;
}

int controll () {
    char key;
    while (1)
    {
        scanf ("%c", &key);
        switch (key)
        {
        case 'w':
            return CIMA;
            break;
        case 's':
            return BAIXO;
            break;
        case 'd':
            return DIREITA;
            break;
        case 'a':
            return ESQUERDA;
            break;
        
        default:
            break;
        }
    }
}


void atualiza_mapa_local(unsigned char map[8][8], unsigned char x, unsigned char y, int tesouro) {
    if (tesouro)
        map [x][y] = 2;
    else 
        map [x][y] = 1;
}

void atualiza_pos_local (unsigned char direcao, unsigned char *x, unsigned char *y) {
    switch (direcao)
    {
    case CIMA:
        if (*x > 0)
            (*x)--;
        break;
    case BAIXO:
        if ((*x) < 7)
            (*x)++;
        break;
    case DIREITA:
        if ((*y) < 7)
            (*y)++;
        break;
    case ESQUERDA:
        if ((*y) > 0)
            (*y)--;
        break;
    default:
        break;
    }
}

FILE *abre_arquivo(unsigned char *nome_arquivo) {
    FILE *fd_write = fopen((char *)nome_arquivo, "wb");
    if (fd_write == NULL) {
        perror("Erro ao criar o arquivo");
        return NULL;
    }
    
    // Pega o usuário que chamou o sudo (ex: "ubuntu")
    const char *user = getenv("SUDO_USER");
    struct passwd *pw = getpwnam(user);
    if (pw == NULL) {
        printf("Usuário não encontrado\n");
        fclose(fd_write);
        return NULL;
    }

    uid_t uid = pw->pw_uid;
    uid_t gid = pw->pw_gid;

    if (chown((char *)nome_arquivo, uid, gid) != 0) {
        perror("Erro ao mudar propriedade do arquivo");
        fclose(fd_write);
        return NULL;
    }

    return fd_write;
}

// abre programa do tipo do arquivo
void play(unsigned char *arquivo) {
    if (arquivo == NULL) {
        fprintf(stderr, "Arquivo nulo\n");
        return;
    }

    // Pega o usuário que chamou o sudo (ex: "ubuntu")
    const char *usuario = getenv("SUDO_USER");
    if (usuario == NULL) {
        fprintf(stderr, "Nao foi possivel detectar o usuario que chamou sudo\n");
        return;
    }

    // Monta o comando para abrir com o usuário correto e variáveis do ambiente
    char comando[512];
    snprintf(comando, sizeof(comando),
        "sudo -u %s DISPLAY=$DISPLAY DBUS_SESSION_BUS_ADDRESS=$DBUS_SESSION_BUS_ADDRESS xdg-open \"%s\"",
        usuario, arquivo);

    printf("Executando: %s\n", comando);

    int status = system(comando);
    if (status == -1) {
        perror("Erro ao executar o comando");
    }
}

int main ( int argc, char** argv ) {
	if (argc < 2) { 
		printf ("deve ser executado ./client <nome da porta>  <nome_arquivo_de_destino>\n"); 
		return 1;
	}

    // mapa local do cliente
    unsigned char map[8][8];
    for (int i = 0; i < 8; i++)
        for (int j = 0; j < 8; j++)
            map[i][j] = 0;
    unsigned char x = 0, y= 0; //player pos

    int socket = cria_raw_socket (argv[1]);
    
    unsigned char type;
    unsigned char sequencia = 0;
    unsigned char tipo = 0x00;  // valor provisorio de teste
    unsigned char buffer[sizeof (Package) + MAX_DATA];
    unsigned char backup_receiv_buffer[sizeof (Package) + MAX_DATA];
    unsigned char backup_sent_buffer[sizeof (Package) + MAX_DATA];
    int counter = 1;
    int recebido = 1;

    tipo = controll ();
    while (1) {
        if (recebido)
            tipo = controll ();
        atualiza_pos_local (tipo, &x, &y);
        recebido = 0;
        
        // Envia movimento e espeara ACK de movimento
        do {
            do {
                package_assembler (buffer, 0, sequencia, tipo, NULL);
                memcpy (&backup_sent_buffer, &buffer, sizeof (Package));

                if (send (socket, buffer, sizeof (buffer), 0) < 0) {
                    perror ("Erro ao enviar\n");
                    exit (0);
                }
                printf ("%dº Pacote enviado\n", counter);
                //for (int i = 4; i < 4 + get_size(buffer); ++i) putchar (buffer[i]);
                //usleep (100000); // 100ms 
            } while (recebe_mensagem (socket, 1000, buffer, sizeof (buffer), sequencia) < 0);// espera e reenvia ate obter resposta
            memcpy (&backup_receiv_buffer, &buffer, sizeof (Package)+ get_size(buffer));

            type = get_type (buffer);
            //usleep (100000); // 100ms
        } while (checksum (buffer) != buffer[3] && type == NACK /*|| (type != OK_ACK && type != TEXTO_ACK_NOME && type != VIDEO_ACK_NOME && type != IMAGEM_ACK_NOME)*/); // checksum incorreto ou erro no reenvio
        
        // recebe arquivo
        if ((type == TEXTO_ACK_NOME || type == IMAGEM_ACK_NOME || type == VIDEO_ACK_NOME)) {
            FILE *fd_write;
            unsigned char erro = 0;
            atualiza_mapa_local(map, x, y, 1);

            unsigned char nome_arquivo[get_size(buffer)];
            memcpy(nome_arquivo, &buffer[DATA], get_size(buffer));

            package_assembler(buffer, 0, get_sequence(buffer), ACK, NULL);
            if (send(socket, buffer, sizeof(buffer), 0) < 0) {
                perror("Erro ao enviar\n");
                exit(0);
            }
            sequencia = (sequencia + 1) % 32;

            // Envia movimento e espeara ACK de movimento
            uint64_t tamanho_livre = espaco_livre(".");
            uint64_t tamanho_arquivo;
            if (recebe_mensagem (socket, 1000, buffer, sizeof (buffer), sequencia) < 0) {
                perror("Timeout\n");
                exit(0);
            }

            do {
                if (checksum(buffer) == buffer[3]) {
                    if (get_type(buffer) == TAMANHO && sequencia == get_sequence(buffer)) {
                        // trata tamanho
                        tamanho_arquivo = le_uint64 (&buffer[DATA]);
                        fd_write = abre_arquivo (nome_arquivo);
                        if (fd_write != NULL && tamanho_livre > tamanho_arquivo) {
                            package_assembler (buffer, 0, get_sequence(buffer), ACK, NULL);
                            if (send (socket, buffer, sizeof (buffer), 0) < 0) {
                                perror ("Erro ao enviar\n");
                                exit (0);
                            }
                            printf ("%dº Pacote enviado\n", counter);        
                        }
                        else { // erro para receber arquivo, envia codigo de erro junto da mensagem
                            erro = 1;
                            unsigned char error_log[64];
                            uint64_t error_code;
                            if (!fd_write) {
                                error_code = SEM_PERMISSAO_DE_ACESSO;
                            }
                            else {
                                error_code = ESPACO_INSUFICIENTE;
                            }
                            memcpy(error_log, &error_code, sizeof(uint64_t));
                            package_assembler (buffer, sizeof(uint64_t), get_sequence(buffer), ERRO, error_log);

                            if (send (socket, buffer, sizeof (buffer), 0) < 0) {
                                perror ("Erro ao enviar\n");
                                exit (0);
                            }
                            printf ("%dº Pacote enviado\n", counter);     
                        }
                        break;
                    }

                    package_assembler (buffer, 0, get_sequence(buffer), ACK, NULL);
                    if (send (socket, buffer, sizeof (buffer), 0) < 0) {
                        perror ("Erro ao enviar\n");
                        exit (0);
                    }
                    printf ("%dº Pacote enviado\n", counter);
                }
                else {
                    package_assembler (buffer, 0, get_sequence(buffer), NACK, NULL);
                    if (send (socket, buffer, sizeof (buffer), 0) < 0) {
                        perror ("Erro ao enviar\n");
                        exit (0);
                    }
                    printf ("%dº Pacote enviado\n", counter);
                }
                
                if (recebe_mensagem (socket, 1000, buffer, sizeof (buffer), sequencia) < 0) {
                    perror("Timeout\n");
                    exit(0);
                }
                //usleep (100000); // 100ms 
            } while (checksum(buffer) != buffer[3]);
            sequencia = (sequencia + 1) % 32;

            if (recebe_mensagem(socket, 1000, buffer, sizeof(buffer), sequencia) < 0){
                perror ("Erro ao receber\n");
                exit (0);
            }

            while (get_type (buffer) != FIM_ARQUIVO)
            {
                while (checksum(buffer) != buffer[3]) { // checksum incorreto ou erro no reenvio
                        
                    package_assembler (buffer, 0, get_sequence (buffer), NACK, NULL);
                    if (send(socket, buffer, sizeof(buffer), 0) < 0) {
                        perror("Erro ao enviar\n");
                        exit(0);
                    }
                    printf("%dº Pacote enviado\n", counter);
                    // for (int i = 4; i < 4 + get_size(buffer); ++i) putchar (buffer[i]);
                    // usleep (100000); // 100ms
                    
                    if (recebe_mensagem(socket, 1000, buffer, sizeof(buffer), sequencia) < 0){
                        perror ("Erro ao enviar\n");
                        exit (0);
                    }
                    type = get_type(buffer);
                    // usleep (100000); // 100ms
                }

                if (sequencia == get_sequence (buffer) && get_type(buffer) == DADOS) {
                    printf ("recebendo dados\n");
                    write_data (fd_write, &buffer[DATA], get_size(buffer));
                    sequencia = (sequencia + 1) % 32;
                }

                package_assembler(buffer, 0, get_sequence(buffer), ACK, NULL);
                if (send(socket, buffer, sizeof(buffer), 0) < 0) {
                    perror("Erro ao enviar\n");
                    exit(0);
                }

                if (recebe_mensagem(socket, 1000, buffer, sizeof(buffer), sequencia) < 0){
                    perror ("Erro ao enviar\n");
                    exit (0);
                }
            }
            fclose (fd_write);
            printf ("fim do arquivo\n");

            package_assembler(buffer, 0, get_sequence(buffer), ACK, NULL);
            if (send(socket, buffer, sizeof(buffer), 0) < 0) {
                perror("Erro ao enviar\n");
                exit(0);
            }

            // abre arquivo se ele foi recebido corretamente
            if (!erro) {
                play (nome_arquivo);
            }

            recebido = 1;
            sequencia = (sequencia + 1) % 32;
        }
        else if (type == OK_ACK ){
            atualiza_mapa_local(map, x, y, 0);
            recebido = 1;
            sequencia = (sequencia + 1) % 32;
        }
        else { // fim do arquivo mal resolvido
            package_assembler(buffer, 0, get_sequence(buffer), ACK, NULL);
            if (send(socket, buffer, sizeof(buffer), 0) < 0) {
                perror("Erro ao enviar\n");
                exit(0);
            }
        }
    }
	return 0;
}
