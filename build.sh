#!/bin/bash
# WYPE Build Script
# Builds wype and installs it to /usr/local/bin.

set -e

echo "=== WYPE Build ==="
echo ""

# Change to script directory
cd "$(dirname "$0")"

# Only run autogen.sh if configure does not exist or configure.ac is newer
if [ ! -f configure ] || [ configure.ac -nt configure ]; then
    echo ">>> autogen.sh ..."
    ./autogen.sh
fi

# Only run configure if Makefile does not exist or configure is newer
if [ ! -f Makefile ] || [ configure -nt Makefile ]; then
    echo ">>> configure ..."
    ./configure
fi

echo ">>> make ..."
make -j$(nproc)

echo ">>> make install ..."
sudo make install

echo ""
echo "=== Installation complete ==="
echo ""
read -p "Start wype now? [y/N] " answer
if [[ "$answer" =~ ^[jJyY]$ ]]; then
    sudo wype
fi
