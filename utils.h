#include <time.h>
#include <arpa/inet.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>

#define ESCAPE_BYTE 0xFF
#define DATA 4
#define MIN_SIZE 22
#define MAX_DATA 127

#define DATA 4
#define MAX_DATA 127
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

#define TAMANHO_NOME_ARQUIVO 64

#define TAM 8
#define VALORES 8

 typedef struct Package {
    unsigned char start;
    unsigned char info_upper;
    unsigned char info_down;
    unsigned char checksum;
} Package;

int cria_raw_socket(char* nome_interface_rede);

long long timestamp();

unsigned char get_size (unsigned char *buffer);

unsigned char get_sequence (unsigned char *buffer);

unsigned char get_type (unsigned char *buffer);
 
int protocolo_e_valido(char* buffer, int tamanho_buffer, unsigned char sequencia);

// realiza soma byte a byte do buffer
unsigned char checksum (unsigned char *buffer);

void package_assembler (unsigned char *buffer, unsigned char size, unsigned char sequence, unsigned char type, unsigned char *data);

// Retorna o tamanho do arquivo em bytes ou -1 em caso de erro
off_t tamanho_arquivo(const char *caminho_arquivo);

void print_data (unsigned char* buffer);

// Retorna espaço livre em bytes, ou -1 em caso de erro
int64_t espaco_livre(unsigned char *caminho);

uint64_t le_uint64(unsigned char *src);

FILE *abre_arquivo(unsigned char *nome_arquivo);

// abre programa do tipo do arquivo
void play(unsigned char *arquivo);