#!/usr/bin/env bash
# ==============================================================================
# Launch script for Raspberry Pi CM5 Lite (CLI mode)
# Handles platform plugin selection (eglfs KMS hardware acceleration or linuxfb)
# ==============================================================================

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BIN_PATH="$SCRIPT_DIR/../rpi_hmi/build/rpi_esp_hmi"

if [ ! -f "$BIN_PATH" ]; then
    echo "[ERROR] Binary not found at $BIN_PATH"
    echo "Please build the project first:"
    echo "  cd $SCRIPT_DIR/../rpi_hmi && mkdir -p build && cd build && cmake .. && make -j\$(nproc)"
    exit 1
fi

# Detect display environment
if [ -n "$WAYLAND_DISPLAY" ]; then
    echo "[INFO] Running under Wayland compositor"
    export QT_QPA_PLATFORM="wayland;xcb"
elif [ -n "$DISPLAY" ]; then
    echo "[INFO] Running under X11"
    export QT_QPA_PLATFORM="xcb"
else
    echo "[INFO] Running on Raspberry Pi OS Lite console (Direct to display via KMS/DRM)"
    # Prefer eglfs for hardware acceleration on Raspberry Pi CM5
    export QT_QPA_PLATFORM="eglfs"
    export QT_QPA_EGLFS_ALWAYS_SET_MODE="1"
    
    # Enable evdev / libinput touch & mouse support
    export QT_QPA_GENERIC_PLUGINS="evdevtouch,evdevmouse,evdevkeyboard"
fi

echo "[INFO] Starting HMI: $BIN_PATH"
"$BIN_PATH" "$@" || {
    EXIT_CODE=$?
    if [ "$QT_QPA_PLATFORM" = "eglfs" ] && [ $EXIT_CODE -ne 0 ]; then
        echo "[WARN] eglfs failed (exit code $EXIT_CODE). Attempting fallback to linuxfb..."
        export QT_QPA_PLATFORM="linuxfb"
        "$BIN_PATH" "$@"
    fi
}

