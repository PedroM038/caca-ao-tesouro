#ifndef CLIENT_LIB_H
#define CLIENT_LIB_H

#include "network_lib.h"

typedef struct {
    int socket;
    unsigned char sequencia;
    FILE *arquivo_recebimento;
    char nome_arquivo_destino[64];
} ClienteEstado;

// Funções do cliente
ClienteEstado* criar_cliente(char* interface_rede);
void destruir_cliente(ClienteEstado* cliente);
int abrir_arquivo_recebimento(ClienteEstado* cliente, const char* nome_arquivo);
int receber_arquivo(ClienteEstado* cliente);
int enviar_movimento(ClienteEstado* cliente, unsigned char direcao);

#endif // CLIENT_LIB_H