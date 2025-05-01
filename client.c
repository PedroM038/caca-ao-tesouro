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
    unsigned char info;
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
 
int protocolo_e_valido(char* buffer, int tamanho_buffer) {
    if (tamanho_buffer <= 0) { return 0; }
    // insira a sua validação de protocolo aqui
    return (buffer[0] == 0x7e);
}
 
// retorna -1 se deu timeout, ou quantidade de bytes lidos
int recebe_mensagem(int soquete, int timeoutMillis, char* buffer, int tamanho_buffer) {
    long long comeco = timestamp();
    struct timeval timeout;
    timeout.tv_sec = timeoutMillis/1000;
    timeout.tv_usec = (timeoutMillis%1000) * 1000;
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, (char*) &timeout, sizeof(timeout));
    int bytes_lidos;
    do {
        bytes_lidos = recv(soquete, buffer, tamanho_buffer, 0);
        if (protocolo_e_valido(buffer, bytes_lidos)) { return bytes_lidos; }
    } while (timestamp() - comeco <= timeoutMillis);
    return -1;
}

int main () {
    int socket = cria_raw_socket ("lo"); // "eth0" ou "enp5s0
    
    unsigned char sequencia = 0xff; // valor provisorio de teste
    unsigned char tipo = 0x00;  // valor provisorio de teste
    unsigned char checksum = 0x00; // valor provisorio de teste
    unsigned char buffer[sizeof (Package) + MAX_DATA];
    int counter = 1;
    while (1) {
        ssize_t tamanho = recebe_mensagem (socket, 1000, buffer, sizeof (buffer));
        if (tamanho < 0) {
            perror ("Erro ao receber dados");
            break;
        }
        printf("%dº Pacote recebido, tamanho do pacote: %ld bytes\n", counter, tamanho);
            
        unsigned int string_size = (buffer[1] >> 1) + 1; // adiciona espaco para /0
        unsigned char string[string_size];
        memcpy (string, &buffer[DATA], string_size -1); // copia sem considerar /0
        string[string_size] = '\0'; 
        printf ("%s", string);
        usleep (100000); // 100ms 
        counter++;
    }

	
	return 0;

}
