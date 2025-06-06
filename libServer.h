#ifndef SERVER_LIB_H
#define SERVER_LIB_H

#include "network_lib.h"

typedef struct {
    int socket;
    unsigned char sequencia;
    FILE *arquivo_atual;
    char nome_arquivo[64];
} ServidorEstado;

// Funções do servidor
ServidorEstado* criar_servidor(char* interface_rede);
void destruir_servidor(ServidorEstado* servidor);
int abrir_arquivo_envio(ServidorEstado* servidor, const char* nome_arquivo);
int enviar_arquivo(ServidorEstado* servidor);
int processar_mensagem_cliente(ServidorEstado* servidor, unsigned char* buffer, int tamanho);

#endif // SERVER_LIB_H