#include "network_lib.h"

// ===== FUNÇÕES DE REDE =====

int cria_raw_socket(char* nome_interface_rede) {
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
    
    if (bind(soquete, (struct sockaddr*) &endereco, sizeof(endereco)) == -1) {
        fprintf(stderr, "Erro ao fazer bind no socket\n");
        exit(-1);
    }
 
    struct packet_mreq mr = {0};
    mr.mr_ifindex = ifindex;
    mr.mr_type = PACKET_MR_PROMISC;
    
    if (setsockopt(soquete, SOL_PACKET, PACKET_ADD_MEMBERSHIP, &mr, sizeof(mr)) == -1) {
        fprintf(stderr, "Erro ao fazer setsockopt: "
            "Verifique se a interface de rede foi especificada corretamente.\n");
        exit(-1);
    }
 
    return soquete;
}

long long timestamp(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

// ===== FUNÇÕES DE PROTOCOLO =====

unsigned char get_size(unsigned char *buffer) {
    return (buffer[1] >> 1);
}

unsigned char get_sequence(unsigned char *buffer) {
    return (((0x01 & buffer[1]) << 4) | ((0xf0 & buffer[2]) >> 4));
}

unsigned char get_type(unsigned char *buffer) {
    return (0xf & buffer[2]);
}

unsigned char checksum(unsigned char *buffer) {
    unsigned int sum = buffer[1] + buffer[2];
    int size = buffer[1] >> 1;
    for (int i = 4; i < 4 + size; ++i) {
        sum += buffer[i];
    }
    return (unsigned char) (0xff & sum);
}

void package_assembler(unsigned char *buffer, unsigned char size, unsigned char sequence, unsigned char type, unsigned char *data) {
    buffer[0] = 0x7e;
    buffer[1] = (0x7f & size) << 1;
    buffer[1] = buffer[1] | ((0x10 & sequence) >> 4);
    buffer[2] = (0x0f & sequence) << 4;
    buffer[2] = buffer[2] | (0x0f & type);
    
    if (data == NULL || size == 0)
        memset(&buffer[DATA], 0, MIN_SIZE);
    else    
        memcpy(&buffer[DATA], data, size);
    
    buffer[3] = checksum(buffer);
}

int protocolo_e_valido(char* buffer, int tamanho_buffer, unsigned char sequencia) {
    if (tamanho_buffer <= 0) return 0;
    return (buffer[0] == 0x7e);
}

// ===== FUNÇÕES DE COMUNICAÇÃO =====

int recebe_mensagem(int soquete, int timeoutMillis, char* buffer, int tamanho_buffer, unsigned char sequencia) {
    long long comeco = timestamp();
    struct timeval timeout;
    timeout.tv_sec = timeoutMillis/1000;
    timeout.tv_usec = (timeoutMillis%1000) * 1000;
    setsockopt(soquete, SOL_SOCKET, SO_RCVTIMEO, (char*) &timeout, sizeof(timeout));
    
    int bytes_lidos;
    do {
        bytes_lidos = recv(soquete, buffer, tamanho_buffer, 0);
        if (protocolo_e_valido(buffer, bytes_lidos, sequencia)) {
            return bytes_lidos;
        }
    } while ((timestamp() - comeco <= timeoutMillis));
    
    return -1;
}

int envia_mensagem(int soquete, unsigned char *buffer, int tamanho_buffer) {
    return send(soquete, buffer, tamanho_buffer, 0);
}

int envia_ack(int soquete, unsigned char sequencia) {
    unsigned char buffer[sizeof(Package) + MIN_SIZE];
    package_assembler(buffer, 0, sequencia, ACK, NULL);
    return envia_mensagem(soquete, buffer, sizeof(buffer));
}

int envia_nack(int soquete, unsigned char sequencia) {
    unsigned char buffer[sizeof(Package) + MIN_SIZE];
    package_assembler(buffer, 0, sequencia, NACK, NULL);
    return envia_mensagem(soquete, buffer, sizeof(buffer));
}

// ===== FUNÇÕES DE TRANSFERÊNCIA DE ARQUIVO =====

int read_data_with_escape(FILE* fd_read, unsigned char* read_buffer, int max_size) {
    unsigned char byte;
    int i = 0;
    int b;

    while(i < max_size) {
        b = fgetc(fd_read);
        if (b == EOF)
            break;

        byte = (unsigned char)b;
        read_buffer[i++] = byte;

        if ((byte == 0x88 || byte == 0x81 || byte == 0xFF)) {
            if (i >= max_size) {
                ungetc(byte, fd_read);
                i--;
                break;
            }
            read_buffer[i++] = ESCAPE_BYTE;
        }
    }

    return i;
}

void write_data_without_escape(FILE* fd_write, unsigned char* buffer, int size) {
    int i = 0;

    while (i < size) {
        unsigned char byte = buffer[i];
        fputc(byte, fd_write);

        if ((byte == 0x81 || byte == 0x88 || byte == 0xFF) &&
            i < size - 1 && buffer[i + 1] == 0xFF) {
            i += 2;
        } else {
            i++;
        }
    }
    fflush(fd_write);
}