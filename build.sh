#!/bin/bash
# BKR_NWIPE Build-Skript
# Baut nwipe aus dem aktuellen Quellcode und installiert es.
# Verwendung: ./build.sh [install]

set -e

echo "=== BKR_NWIPE Build ==="
echo ""

# Ins Skript-Verzeichnis wechseln
cd "$(dirname "$0")"

# autogen.sh nur ausführen wenn configure nicht existiert oder configure.ac neuer ist
if [ ! -f configure ] || [ configure.ac -nt configure ]; then
    echo ">>> autogen.sh ..."
    ./autogen.sh
fi

# configure nur ausführen wenn Makefile nicht existiert oder configure neuer ist
if [ ! -f Makefile ] || [ configure -nt Makefile ]; then
    echo ">>> configure ..."
    ./configure
fi

echo ">>> make ..."
make -j$(nproc)

echo ""
echo "=== Build erfolgreich ==="
echo ""

if [ "$1" = "install" ]; then
    echo ">>> make install ..."
    sudo make install
    echo ""
    echo "=== Installation abgeschlossen ==="
    echo "Starten mit: sudo nwipe"
else
    echo "Binary: src/nwipe"
    echo "Direkt starten: sudo src/nwipe"
    echo "Installieren:   ./build.sh install"
fi
