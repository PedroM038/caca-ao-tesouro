#ifndef NETWORK_LIB_H
#define NETWORK_LIB_H

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
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>

#define ESCAPE_BYTE 0xFF
#define DATA 4
#define MAX_DATA 127
#define MIN_SIZE 22

// Tipos de mensagem
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

// Códigos de erro
#define SEM_PERMISSAO_DE_ACESSO 0
#define ESPACO_INSUFICIENTE 1

typedef struct Package {
    unsigned char start;
    unsigned char info_upper;
    unsigned char info_down;
    unsigned char checksum;
} Package;

// Funções de rede
int cria_raw_socket(char* nome_interface_rede);
long long timestamp(void);

// Funções de protocolo
unsigned char get_size(unsigned char *buffer);
unsigned char get_sequence(unsigned char *buffer);
unsigned char get_type(unsigned char *buffer);
unsigned char checksum(unsigned char *buffer);
void package_assembler(unsigned char *buffer, unsigned char size, unsigned char sequence, unsigned char type, unsigned char *data);
int protocolo_e_valido(char* buffer, int tamanho_buffer, unsigned char sequencia);

// Funções de comunicação
int recebe_mensagem(int soquete, int timeoutMillis, char* buffer, int tamanho_buffer, unsigned char sequencia);
int envia_mensagem(int soquete, unsigned char *buffer, int tamanho_buffer);
int envia_ack(int soquete, unsigned char sequencia);
int envia_nack(int soquete, unsigned char sequencia);

// Funções de transferência de arquivo
int read_data_with_escape(FILE* fd_read, unsigned char* read_buffer, int max_size);
void write_data_without_escape(FILE* fd_write, unsigned char* buffer, int size);

#endif // NETWORK_LIB_H