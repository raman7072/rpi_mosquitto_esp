#!/usr/bin/env bash
# ==============================================================================
# Setup script for Raspberry Pi OS Lite (CLI) on Raspberry Pi CM5 Lite (2GB RAM)
# Installs Qt6, build tools, Mesa DRM/KMS graphics drivers, and configures permissions
# ==============================================================================

set -e

echo "=== [1/4] Updating package index ==="
sudo apt update

echo "=== [2/4] Installing Build Tools & Qt6 QML Packages ==="
sudo apt install -y \
    build-essential \
    cmake \
    git \
    qt6-base-dev \
    qt6-declarative-dev \
    qml6-module-qtquick \
    qml6-module-qtquick-controls \
    qml6-module-qtquick-layouts \
    qml6-module-qtquick-window \
    qml6-module-qtquick-templates \
    libgl1-mesa-dev \
    libegl1-mesa-dev \
    libdrm-dev \
    libgbm-dev \
    libinput-dev \
    libxkbcommon-dev \
    mosquitto-clients

echo "=== [3/4] Configuring Hardware & DRM Permissions ==="
# Add current user to video, render, and input groups for DRM/KMS console access
sudo usermod -a -G video,render,input "$USER"

echo "=== [4/4] Building the HMI Application ==="
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
BUILD_DIR="$PROJECT_ROOT/rpi_hmi/build"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake ..
make -j"$(nproc)"

echo ""
echo "======================================================================"
echo " Setup complete! "
echo " Remember to log out and log back in (or reboot) so group permissions"
echo " (video, render, input) take effect for DRM/KMS console rendering."
echo ""
echo " To run the HMI on your display from the CLI:"
echo "   $PROJECT_ROOT/scripts/run_hmi.sh"
echo "======================================================================"
