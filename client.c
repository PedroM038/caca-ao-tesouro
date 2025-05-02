#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

//#define SOL_SOCKET 1
//#define SO_RCVTIMEO 20

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
    return (buffer[0] == 0x7e && sequencia == get_sequence(buffer));
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
    string[string_size] = '\0'; 
    printf ("%s", string);
}


int main () {
    int socket = cria_raw_socket ("lo"); // "eth0" ou "enp5s0
    
    unsigned char sequencia = 0;
    unsigned char tipo = 0x00;  // valor provisorio de teste
    unsigned char buffer[sizeof (Package) + MAX_DATA];
    int counter = 1;
    while (1) {
        ssize_t tamanho = recebe_mensagem (socket, 100000, buffer, sizeof (buffer), sequencia); // tempo de espera deve ser maior que timeout de envio
        if (tamanho < 0) {
            perror ("Erro ao receber dados");
            exit (0);
        }
        
        while (checksum (buffer) != buffer[3]) { // checksum incorreto
            package_assembler (buffer, 0, sequencia, NACK, NULL);
            if (send (socket, buffer, sizeof (buffer), 0) < 0) {
                perror ("Erro ao enviar\n");
                exit (0);
            }
            printf ("NACK do %dº pacote enviado\n", counter);

            ssize_t tamanho = recebe_mensagem (socket, 100000, buffer, sizeof (buffer), sequencia); // tempo de espera deve ser maior que timeout de envio
            if (tamanho < 0) {
                perror ("Erro ao receber dados");
                exit (0);
            } 

            usleep (100000); // 100ms 
        }

        print_data (buffer);
        //printf("\n%dº Pacote recebido, tamanho do pacote: %ld bytes\n", counter, tamanho);
        package_assembler (buffer, 0, sequencia, ACK, NULL);     
        if (send (socket, buffer, sizeof (buffer), 0) < 0) {
            perror ("Erro ao enviar\n");
            exit (0);
        }
        //printf ("ACK do %dº pacote enviado\n", counter);
        sequencia = (sequencia + 1) % 32;
        counter++;            

        usleep (100000); // 100ms 
    }

	
	return 0;
}
