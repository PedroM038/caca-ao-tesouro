#include "utils.h"

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
    return (buffer[0] == 0x7e /*&& sequencia == get_sequence(buffer)*/);
}

unsigned char checksum (unsigned char *buffer) {
    unsigned int sum = buffer[1] + buffer[2]; //tamanho + sequencia + tipo
    int size = buffer[1] >> 1; // tamanho do buffer
    for (int i = 4; i < 4 + size; ++i) { // comeca depois do byte do checksum
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

// Retorna o tamanho do arquivo em bytes ou -1 em caso de erro
off_t tamanho_arquivo(const char *caminho_arquivo) {
    struct stat st;

    // Verifica se o stat teve sucesso
    if (stat(caminho_arquivo, &st) == 0) {
        // Verifica se é um arquivo regular
        if (S_ISREG(st.st_mode)) {
            return st.st_size;
        } else {
            // Não é um arquivo regular
            return 0;
        }
    } else {
        // Erro ao obter informações do arquivo
        return 0;
    }
}

void print_data (unsigned char* buffer) {
    unsigned int string_size = get_size(buffer) + 1; // adiciona espaco para /0
    unsigned char string[string_size];
    memcpy (string, &buffer[DATA], string_size -1); // copia sem considerar /0
    string[string_size-1] = '\0'; 
    fprintf(stdout, "%s", string);
}

// Retorna espaço livre em bytes, ou -1 em caso de erro
int64_t espaco_livre(unsigned char *caminho) {
    struct statvfs st;

    if (statvfs(caminho, &st) == 0) {
        return (int64_t)st.f_bsize * (int64_t)st.f_bavail;
    } else {
        // Erro ao acessar informações do sistema de arquivos
        return -1;
    }
}

uint64_t le_uint64(unsigned char *src) {
    uint64_t resultado = 0;
    for (int i = sizeof(uint64_t) - 1; i >= 0; --i) {
        resultado <<= 8;
        resultado |= src[i];
    }
    return resultado;
}

FILE *abre_arquivo(unsigned char *nome_arquivo) {
    FILE *fd_write = fopen((char *)nome_arquivo, "wb");
    if (fd_write == NULL) {
        perror("Erro ao criar o arquivo");
        return NULL;
    }
    
    // Pega o usuário que chamou o sudo (ex: "ubuntu")
    const char *user = getenv("SUDO_USER");
    struct passwd *pw = getpwnam(user);
    if (pw == NULL) {
        printf("Usuário não encontrado\n");
        fclose(fd_write);
        return NULL;
    }

    uid_t uid = pw->pw_uid;
    uid_t gid = pw->pw_gid;

    if (chown((char *)nome_arquivo, uid, gid) != 0) {
        perror("Erro ao mudar propriedade do arquivo");
        fclose(fd_write);
        return NULL;
    }

    return fd_write;
}

// abre programa do tipo do arquivo
void play(unsigned char *arquivo) {
    if (arquivo == NULL) {
        fprintf(stderr, "Arquivo nulo\n");
        return;
    }

    // Pega o usuário que chamou o sudo (ex: "ubuntu")
    const char *usuario = getenv("SUDO_USER");
    if (usuario == NULL) {
        fprintf(stderr, "Nao foi possivel detectar o usuario que chamou sudo\n");
        return;
    }

    // Monta o comando para abrir com o usuário correto e variáveis do ambiente
    char comando[512];
    snprintf(comando, sizeof(comando),
        "sudo -u %s DISPLAY=$DISPLAY DBUS_SESSION_BUS_ADDRESS=$DBUS_SESSION_BUS_ADDRESS xdg-open \"%s\"",
        usuario, arquivo);

    printf("Executando: %s\n", comando);

    int status = system(comando);
    if (status == -1) {
        perror("Erro ao executar o comando");
    }
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
