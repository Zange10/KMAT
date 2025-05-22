#!/bin/bash

# Set names
LAUNCHER_SRC="cpl.c"
LINUX_OUT="Launch_KMAT"
WINDOWS_OUT="Launch_KMAT.exe"

# Compile for Linux
echo "Compiling for Linux..."
gcc -o "$LINUX_OUT" "$LAUNCHER_SRC"
if [ $? -ne 0 ]; then
    echo "Linux build failed."
    exit 1
fi
echo "✅ Linux executable: $LINUX_OUT"

# Compile for Windows
echo "Compiling for Windows..."
x86_64-w64-mingw32-gcc -o "$WINDOWS_OUT" "$LAUNCHER_SRC" -mwindows
if [ $? -ne 0 ]; then
    echo "Windows build failed. Make sure mingw-w64 is installed."
    exit 1
fi
echo "✅ Windows executable: $WINDOWS_OUT"
