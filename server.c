#include "libServer.h"
#include <signal.h>

// Variável global para controle de saída
ServidorEstado* servidor_global = NULL;

void signal_handler(int sig) {
    printf("\nEncerrando servidor...\n");
    if (servidor_global) {
        destruir_servidor(servidor_global);
    }
    exit(0);
}

void mostrar_menu() {
    printf("\n=== SERVIDOR CAÇA AO TESOURO ===\n");
    printf("1. Enviar arquivo\n");
    printf("2. Aguardar mensagens do cliente\n");
    printf("3. Sair\n");
    printf("Escolha uma opção: ");
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Uso: sudo ./server <nome_da_interface>\n");
        printf("Exemplo: sudo ./server lo\n");
        return 1;
    }

    // Configurar handler para sinais
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    printf("Iniciando servidor na interface: %s\n", argv[1]);
    
    // Criar servidor
    servidor_global = criar_servidor(argv[1]);
    if (!servidor_global) {
        printf("Erro ao criar servidor\n");
        return 1;
    }

    printf("Servidor criado com sucesso!\n");
    printf("Socket: %d\n", servidor_global->socket);

    int opcao;
    char nome_arquivo[256];
    unsigned char buffer[sizeof(Package) + MAX_DATA];

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
                printf("Digite o nome do arquivo para enviar: ");
                if (fgets(nome_arquivo, sizeof(nome_arquivo), stdin) != NULL) {
                    // Remover \n do final
                    nome_arquivo[strcspn(nome_arquivo, "\n")] = 0;
                    
                    if (abrir_arquivo_envio(servidor_global, nome_arquivo) == 0) {
                        printf("Enviando arquivo: %s\n", nome_arquivo);
                        
                        if (enviar_arquivo(servidor_global) == 0) {
                            printf("Arquivo enviado com sucesso!\n");
                        } else {
                            printf("Erro ao enviar arquivo!\n");
                        }
                    } else {
                        printf("Erro ao abrir arquivo: %s\n", nome_arquivo);
                    }
                }
                break;

            case 2:
                printf("Aguardando mensagens do cliente (pressione Ctrl+C para voltar)...\n");
                
                while (1) {
                    ssize_t tamanho = recebe_mensagem(servidor_global->socket, 5000, 
                                                    (char*)buffer, sizeof(buffer), 
                                                    servidor_global->sequencia);
                    
                    if (tamanho > 0) {
                        printf("Mensagem recebida (%ld bytes)\n", tamanho);
                        
                        if (processar_mensagem_cliente(servidor_global, buffer, tamanho) == 0) {
                            printf("Mensagem processada com sucesso\n");
                        } else {
                            printf("Erro ao processar mensagem\n");
                        }
                    } else {
                        printf("Timeout aguardando mensagens...\n");
                        break;
                    }
                }
                break;

            case 3:
                printf("Encerrando servidor...\n");
                destruir_servidor(servidor_global);
                return 0;

            default:
                printf("Opção inválida!\n");
                break;
        }
    }

    return 0;
}