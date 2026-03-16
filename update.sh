#!/bin/bash
# WYPE Update-Skript
# Pullt die neueste Version von main und baut/installiert wype.

set -e

cd "$(dirname "$0")"

echo "=== WYPE Update ==="
echo ""

echo ">>> git pull origin main ..."
git pull origin main

echo ""

# build.sh aufrufen (fragt am Ende ob wype gestartet werden soll)
./build.sh
