#include "libClient.h"

ClienteEstado* criar_cliente(char* interface_rede) {
    ClienteEstado* cliente = malloc(sizeof(ClienteEstado));
    if (!cliente) return NULL;
    
    cliente->socket = cria_raw_socket(interface_rede);
    cliente->sequencia = 0;
    cliente->arquivo_recebimento = NULL;
    memset(cliente->nome_arquivo_destino, 0, sizeof(cliente->nome_arquivo_destino));
    
    return cliente;
}

void destruir_cliente(ClienteEstado* cliente) {
    if (!cliente) return;
    
    if (cliente->arquivo_recebimento) {
        fclose(cliente->arquivo_recebimento);
    }
    close(cliente->socket);
    free(cliente);
}

int abrir_arquivo_recebimento(ClienteEstado* cliente, const char* nome_arquivo) {
    if (!cliente) return -1;
    
    if (cliente->arquivo_recebimento) {
        fclose(cliente->arquivo_recebimento);
    }
    
    // Tentar mudar propriedade do arquivo para ubuntu (opcional em teste local)
    const char *user = "ubuntu";
    struct passwd *pw = getpwnam(user);
    if (pw != NULL) {
        chown(nome_arquivo, pw->pw_uid, pw->pw_gid);
    }
    
    cliente->arquivo_recebimento = fopen(nome_arquivo, "wb");
    if (!cliente->arquivo_recebimento) {
        printf("Erro ao criar arquivo: %s\n", nome_arquivo);
        return -1;
    }
    
    strncpy(cliente->nome_arquivo_destino, nome_arquivo, sizeof(cliente->nome_arquivo_destino) - 1);
    return 0;
}

int receber_arquivo(ClienteEstado* cliente) {
    if (!cliente || !cliente->arquivo_recebimento) return -1;
    
    unsigned char buffer[sizeof(Package) + MAX_DATA];
    int counter = 1;
    
    while (1) {
        ssize_t tamanho = recebe_mensagem(cliente->socket, 100000, (char*)buffer, sizeof(buffer), cliente->sequencia);
        if (tamanho < 0) {
            printf("Timeout ou erro ao receber dados\n");
            return -1;
        }
        
        // Verificar se é fim de arquivo
        if (get_type(buffer) == FIM_ARQUIVO) {
            printf("Fim de arquivo recebido\n");
            envia_ack(cliente->socket, get_sequence(buffer));
            break;
        }
        
        while (checksum(buffer) != buffer[3]) {
            if (envia_nack(cliente->socket, cliente->sequencia) < 0) {
                perror("Erro ao enviar NACK");
                return -1;
            }
            printf("NACK do %dº pacote enviado (checksum incorreto)\n", counter);

            tamanho = recebe_mensagem(cliente->socket, 100000, (char*)buffer, sizeof(buffer), cliente->sequencia);
            if (tamanho < 0) {
                printf("Timeout ou erro ao receber dados após NACK\n");
                return -1;
            }
        }
        
        unsigned char seq_recebida = get_sequence(buffer);
        
        if (seq_recebida == (cliente->sequencia - 1) % 32) {
            // Pacote duplicado, apenas enviar ACK novamente
            envia_ack(cliente->socket, seq_recebida);
            printf("Pacote duplicado recebido (seq: %d)\n", seq_recebida);
        } else if (seq_recebida == cliente->sequencia) {
            // Pacote correto
            if (get_type(buffer) == DADOS) {
                write_data_without_escape(cliente->arquivo_recebimento, &buffer[DATA], get_size(buffer));
                printf("%dº Pacote recebido e escrito (%d bytes, seq: %d)\n", 
                       counter, get_size(buffer), seq_recebida);
            }
            
            envia_ack(cliente->socket, cliente->sequencia);
            cliente->sequencia = (cliente->sequencia + 1) % 32;
            counter++;
        } else {
            printf("Sequência incorreta: esperado %d, recebido %d\n", 
                   cliente->sequencia, seq_recebida);
            envia_nack(cliente->socket, cliente->sequencia);
        }
        
        usleep(1000); // 1ms delay
    }
    
    return 0;
}

int enviar_movimento(ClienteEstado* cliente, unsigned char direcao) {
    if (!cliente) return -1;
    
    unsigned char buffer[sizeof(Package) + MIN_SIZE];
    package_assembler(buffer, 0, cliente->sequencia, direcao, NULL);
    
    int resultado = envia_mensagem(cliente->socket, buffer, sizeof(Package));
    if (resultado >= 0) {
        cliente->sequencia = (cliente->sequencia + 1) % 32;
    }
    
    return resultado;
}