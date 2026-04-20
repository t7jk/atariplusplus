#!/bin/bash
set -e

echo "=== Instalacja zależności do kompilacji atari++ na Fedorze ==="

sudo dnf install -y \
    gcc \
    gcc-c++ \
    make \
    autoconf \
    SDL-devel \
    libX11-devel \
    libXext-devel \
    libXv-devel \
    alsa-lib-devel \
    ncurses-devel \
    zlib-devel \
    libpng-devel

echo ""
echo "=== Wszystkie zależności zainstalowane. Możesz teraz uruchomić: ==="
echo "  ./configure && make"
