#include "libServer.h"

ServidorEstado* criar_servidor(char* interface_rede) {
    ServidorEstado* servidor = malloc(sizeof(ServidorEstado));
    if (!servidor) return NULL;
    
    servidor->socket = cria_raw_socket(interface_rede);
    servidor->sequencia = 0;
    servidor->arquivo_atual = NULL;
    memset(servidor->nome_arquivo, 0, sizeof(servidor->nome_arquivo));
    
    return servidor;
}

void destruir_servidor(ServidorEstado* servidor) {
    if (!servidor) return;
    
    if (servidor->arquivo_atual) {
        fclose(servidor->arquivo_atual);
    }
    close(servidor->socket);
    free(servidor);
}

int abrir_arquivo_envio(ServidorEstado* servidor, const char* nome_arquivo) {
    if (!servidor) return -1;
    
    if (servidor->arquivo_atual) {
        fclose(servidor->arquivo_atual);
    }
    
    servidor->arquivo_atual = fopen(nome_arquivo, "rb");
    if (!servidor->arquivo_atual) {
        printf("Erro ao abrir arquivo: %s\n", nome_arquivo);
        return -1;
    }
    
    strncpy(servidor->nome_arquivo, nome_arquivo, sizeof(servidor->nome_arquivo) - 1);
    return 0;
}

int enviar_arquivo(ServidorEstado* servidor) {
    if (!servidor || !servidor->arquivo_atual) return -1;
    
    unsigned char read_buffer[MAX_DATA];
    unsigned char buffer[sizeof(Package) + MAX_DATA];
    unsigned char backup_buffer[sizeof(Package) + MAX_DATA];
    int bytes_read;
    int counter = 1;
    unsigned char type;
    
    while ((bytes_read = read_data_with_escape(servidor->arquivo_atual, read_buffer, MAX_DATA)) > 0) {
        do {
            do {
                package_assembler(buffer, bytes_read, servidor->sequencia, DADOS, read_buffer);
                memcpy(backup_buffer, buffer, sizeof(Package) + bytes_read);
                
                if (envia_mensagem(servidor->socket, buffer, sizeof(Package) + bytes_read) < 0) {
                    perror("Erro ao enviar");
                    return -1;
                }
                
                printf("%dº Pacote enviado (tamanho: %d bytes)\n", counter, bytes_read);
                
            } while (recebe_mensagem(servidor->socket, 1000, (char*)buffer, sizeof(buffer), servidor->sequencia) < 0);

            type = get_type(buffer);
            
        } while (checksum(buffer) != buffer[3] || type != ACK);

        servidor->sequencia = (servidor->sequencia + 1) % 32;
        counter++;
        usleep(10000); // 10ms delay
    }
    
    // Enviar mensagem de fim de arquivo
    package_assembler(buffer, 0, servidor->sequencia, FIM_ARQUIVO, NULL);
    envia_mensagem(servidor->socket, buffer, sizeof(Package));
    printf("Sinal de fim de arquivo enviado\n");
    
    return 0;
}

int processar_mensagem_cliente(ServidorEstado* servidor, unsigned char* buffer, int tamanho) {
    if (!servidor) return -1;
    
    unsigned char tipo = get_type(buffer);
    unsigned char seq = get_sequence(buffer);
    
    switch(tipo) {
        case DIREITA:
            printf("Cliente moveu para DIREITA (seq: %d)\n", seq);
            return envia_ack(servidor->socket, seq);
            
        case CIMA:
            printf("Cliente moveu para CIMA (seq: %d)\n", seq);
            return envia_ack(servidor->socket, seq);
            
        case BAIXO:
            printf("Cliente moveu para BAIXO (seq: %d)\n", seq);
            return envia_ack(servidor->socket, seq);
            
        case ESQUERDA:
            printf("Cliente moveu para ESQUERDA (seq: %d)\n", seq);
            return envia_ack(servidor->socket, seq);
            
        case ACK:
            printf("ACK recebido (seq: %d)\n", seq);
            return 0;
            
        case NACK:
            printf("NACK recebido (seq: %d)\n", seq);
            return 0;
            
        default:
            printf("Tipo de mensagem não reconhecido: %d (seq: %d)\n", tipo, seq);
            return envia_nack(servidor->socket, seq);
    }
}