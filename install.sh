#!/usr/bin/env bash

set -e

echo "+--------------------------+"
echo "|                          |"
echo "|      Sulfur++ Lang       |"
echo "| Made by: Enginostek Labs |"
echo "|                          |"
echo "+--------------------------+"

echo ""

echo "[Installer] Installing required packages..."
# Detect package manager and install dependencies
if command -v apt-get >/dev/null 2>&1; then
  echo "Using apt-get..."
  apt-get update -y
  apt-get install -y git llvm clang make gcc g++ cmake build-essential libedit-dev
elif command -v pacman >/dev/null 2>&1; then
  echo "Using pacman..."
  pacman -Sy --noconfirm git llvm clang make gcc g++ cmake libedit
else
  echo "Unsupported package manager. Please install git, llvm, clang, g++, cmake, libedit-dev manually."
  exit 1
fi

echo "[Installer] Running build..."
mkdir -p build
cmake -S . -B build
cmake --build build -- -j$(nproc)

echo "[Installer] Build completed."
