#!/bin/bash

# Atualiza a lista de pacotes
sudo apt update

# Instala o compilador GCC
sudo apt install -y gcc

# Instala o utilitário make
sudo apt install -y make

# Instala o player de vídeo mpv
sudo apt install -y mpv

# Define o mpv como aplicativo padrão para arquivos .mp4
xdg-mime default mpv.desktop video/mp4

# Instala as bibliotecas SDL2 e SDL2_image
sudo apt install -y libsdl2-dev libsdl2-image-dev

echo "Todos os pacotes foram instalados com sucesso."
