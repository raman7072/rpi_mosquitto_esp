#!/usr/bin/env bash
# ==============================================================================
# Helper script to start Mosquitto broker on Ubuntu PC
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONF_FILE="$SCRIPT_DIR/mosquitto.conf"

# Check if mosquitto is installed
if ! command -v mosquitto &> /dev/null; then
    echo "[ERROR] Mosquitto is not installed."
    echo "Install with: sudo apt update && sudo apt install -y mosquitto mosquitto-clients"
    exit 1
fi

echo "======================================================================"
echo " Starting Mosquitto MQTT Broker on port 1883"
echo " Configuration: $CONF_FILE"
echo "======================================================================"

# Display local IP addresses for easy configuration in ESP32 and RPi HMI
echo "[INFO] Local IP addresses on this Ubuntu PC:"
hostname -I || ip -4 addr show | grep -oP '(?<=inet\s)\d+(\.\d+){3}'
echo "======================================================================"
echo "Press Ctrl+C to stop the broker."
echo ""

mosquitto -c "$CONF_FILE" -v
