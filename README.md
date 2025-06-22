# Caça ao Tesouro

## 1. Descrição

O objetivo deste trabalho é implementar um jogo de caça ao tesouro no contexto de Redes I, trabalhando com protocolos de nível 2 na comunicação entre computadores por mensagens curtas e transferência de arquivos. 

### Características do Projeto
- **Modelo**: Cliente-servidor
- **Comunicação**: Raw sockets
- **Implementação**: C, C++ ou Go
- **Requisitos**: Máquinas distintas em modo root, interligadas por cabo de rede direto

## 2. Funcionamento do Jogo

### Servidor
- Controla o mapa e os tesouros
- Exibe posição dos tesouros, movimentos do usuário e posição atual
- Sorteia 8 posições no grid 8x8 para localização dos tesouros
- Gerencia arquivos de tesouro no diretório "objetos"

### Cliente
- Interface de usuário para interação
- Exibe mapa 8x8 (coordenadas: canto inferior esquerdo = (0,0))
- Marca posições percorridas e tesouros encontrados
- Permite movimentação do jogador no grid

### Tesouros
Os tesouros são arquivos localizados no diretório "objetos" com nomes de 1 a 8:
- **Imagens**: `.jpg`
- **Vídeos**: `.mp4` 
- **Textos**: `.txt`

**Limitações**:
- Nome do arquivo: máximo 63 bytes
- Caracteres ASCII: faixa 0x20 a 0x7E

## 3. Protocolo de Comunicação

Protocolo inspirado no Kermit com os seguintes campos:

| Campo | Tamanho | Descrição |
|-------|---------|-----------|
| Marcador início | 8 bits | `0111 1110` |
| Tamanho | 7 bits | Bytes de dados |
| Sequência | 5 bits | Número da sequência |
| Tipo | 4 bits | Tipo da mensagem |
| Checksum | 8 bits | Sobre campos tamanho, seq, tipo e dados |
| Dados | 0-127 bytes | Payload da mensagem |

### Tipos de Mensagem

| Código | Tipo |
|--------|------|
| 0 | ACK |
| 1 | NACK |
| 2 | OK + ACK |
| 3 | Livre |
| 4 | Tamanho |
| 5 | Dados |
| 6 | Texto + ACK + Nome |
| 7 | Vídeo + ACK + Nome |
| 8 | Imagem + ACK + Nome |
| 9 | Fim arquivo |
| 10 | Desloca para direita |
| 11 | Desloca para cima |
| 12 | Desloca para baixo |
| 13 | Desloca para esquerda |
| 14 | Livre |
| 15 | Erro |

### Códigos de Erro

| Código | Descrição |
|--------|-----------|
| 0 | Sem permissão de acesso |
| 1 | Espaço insuficiente |

## 4. Implementação

### Controle de Fluxo
- **Obrigatório**: Stop-and-wait
- **Pontos extras**: Janelas deslizantes (tamanho 3) com Go-Back-N

### Requisitos Técnicos
- Campos do frame, códigos de erro e variáveis de controle devem ser `unsigned`
- Usar `unsigned char` sempre que possível
- Não utilizar `float` em campos de protocolo
- Tratar timeouts e falhas na comunicação
- Implementar aritmética de números seriais para verificação de sequência

## 6. Como Usar

### Compilação
```bash
make
```

### Execução

**Servidor:**
```bash
sudo ./server <nome_da_porta>
```

**Cliente:**
```bash
sudo ./client <nome_da_porta>
```

## 7. Observações Importantes

- **Responsabilidade ACK/NACK**: Apenas uma das partes é responsável pelo envio
- **Armazenamento**: Recomenda-se armazenar última mensagem recebida e enviada
- **Modo root**: Necessário para uso de raw sockets
- **Conexão direta**: Cabo de rede direto entre máquinas (sem switches)

## 8. Entrega

- Relatório obrigatório (formato artigo SBC)
- Apresentação em laboratório do DINF
- Todos os membros devem dominar o trabalho
- **Não pode haver atraso na entrega**
