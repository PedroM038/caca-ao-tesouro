#include "libClient.h"
#include <signal.h>

// Variável global para controle de saída
ClienteEstado* cliente_global = NULL;

void signal_handler(int sig) {
    printf("\nEncerrando cliente...\n");
    if (cliente_global) {
        destruir_cliente(cliente_global);
    }
    exit(0);
}

void mostrar_menu() {
    printf("\n=== CLIENTE CAÇA AO TESOURO ===\n");
    printf("1. Receber arquivo\n");
    printf("2. Enviar movimento - Direita\n");
    printf("3. Enviar movimento - Cima\n");
    printf("4. Enviar movimento - Baixo\n");
    printf("5. Enviar movimento - Esquerda\n");
    printf("6. Testar protocolo (enviar ACK)\n");
    printf("7. Sair\n");
    printf("Escolha uma opção: ");
}

void mostrar_info_cliente() {
    if (cliente_global) {
        printf("Socket: %d\n", cliente_global->socket);
        printf("Sequência atual: %d\n", cliente_global->sequencia);
        printf("Arquivo de destino: %s\n", 
               strlen(cliente_global->nome_arquivo_destino) > 0 ? 
               cliente_global->nome_arquivo_destino : "Nenhum");
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Uso: sudo ./client <nome_da_interface>\n");
        printf("Exemplo: sudo ./client lo\n");
        return 1;
    }

    // Configurar handler para sinais
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("Iniciando cliente na interface: %s\n", argv[1]);
    
    // Criar cliente
    cliente_global = criar_cliente(argv[1]);
    if (!cliente_global) {
        printf("Erro ao criar cliente\n");
        return 1;
    }

    printf("Cliente criado com sucesso!\n");
    mostrar_info_cliente();

    int opcao;
    char nome_arquivo[256];

    while (1) {
        mostrar_menu();
        
        if (scanf("%d", &opcao) != 1) {
            printf("Opção inválida!\n");
            while (getchar() != '\n'); // Limpar buffer
            continue;
        }
        getchar(); // Consumir o \n

        switch (opcao) {
            case 1:
                printf("Digite o nome do arquivo de destino: ");
                if (fgets(nome_arquivo, sizeof(nome_arquivo), stdin) != NULL) {
                    // Remover \n do final
                    nome_arquivo[strcspn(nome_arquivo, "\n")] = 0;
                    
                    if (abrir_arquivo_recebimento(cliente_global, nome_arquivo) == 0) {
                        printf("Aguardando recebimento do arquivo: %s\n", nome_arquivo);
                        printf("Pressione Ctrl+C para cancelar...\n");
                        
                        if (receber_arquivo(cliente_global) == 0) {
                            printf("Arquivo recebido com sucesso!\n");
                        } else {
                            printf("Erro ao receber arquivo!\n");
                        }
                    } else {
                        printf("Erro ao criar arquivo: %s\n", nome_arquivo);
                    }
                }
                break;

            case 2:
                printf("Enviando movimento: DIREITA\n");
                if (enviar_movimento(cliente_global, DIREITA) >= 0) {
                    printf("Movimento enviado com sucesso! Nova sequência: %d\n", 
                           cliente_global->sequencia);
                } else {
                    printf("Erro ao enviar movimento!\n");
                }
                break;

            case 3:
                printf("Enviando movimento: CIMA\n");
                if (enviar_movimento(cliente_global, CIMA) >= 0) {
                    printf("Movimento enviado com sucesso! Nova sequência: %d\n", 
                           cliente_global->sequencia);
                } else {
                    printf("Erro ao enviar movimento!\n");
                }
                break;

            case 4:
                printf("Enviando movimento: BAIXO\n");
                if (enviar_movimento(cliente_global, BAIXO) >= 0) {
                    printf("Movimento enviado com sucesso! Nova sequência: %d\n", 
                           cliente_global->sequencia);
                } else {
                    printf("Erro ao enviar movimento!\n");
                }
                break;

            case 5:
                printf("Enviando movimento: ESQUERDA\n");
                if (enviar_movimento(cliente_global, ESQUERDA) >= 0) {
                    printf("Movimento enviado com sucesso! Nova sequência: %d\n", 
                           cliente_global->sequencia);
                } else {
                    printf("Erro ao enviar movimento!\n");
                }
                break;

            case 6:
                printf("Enviando ACK de teste...\n");
                if (envia_ack(cliente_global->socket, cliente_global->sequencia) >= 0) {
                    printf("ACK enviado com sucesso!\n");
                    cliente_global->sequencia = (cliente_global->sequencia + 1) % 32;
                } else {
                    printf("Erro ao enviar ACK!\n");
                }
                break;

            case 7:
                printf("Encerrando cliente...\n");
                destruir_cliente(cliente_global);
                return 0;

            default:
                printf("Opção inválida!\n");
                break;
        }
        
        printf("\nInformações atuais do cliente:\n");
        mostrar_info_cliente();
    }

    return 0;
}