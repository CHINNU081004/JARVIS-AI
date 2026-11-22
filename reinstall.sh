#!/usr/bin/env bash
set -e

echo "=== Reinstalling JARVIS ==="

# 1. Kill any existing instances
pkill -9 jarvis 2>/dev/null || true

# 2. Build latest code
cd /home/chinnu/Desktop/JARVIS/build
ninja

# 3. Generate .deb package
cpack -G DEB

# 4. Install user binary and icons
mkdir -p ~/.local/bin ~/.local/share/applications ~/.local/share/pixmaps
cp -f /home/chinnu/Desktop/JARVIS/build/jarvis ~/.local/bin/jarvis
cp -f /home/chinnu/Desktop/JARVIS/src/ui/assets/icon.png ~/.local/share/pixmaps/jarvis.png

# 5. Refresh desktop entries
update-desktop-database ~/.local/share/applications/ 2>/dev/null || true

# 6. Install system-wide if passwordless sudo available, else remind user
DEB_FILE=$(ls -t /home/chinnu/Desktop/JARVIS/build/jarvis-*-Linux.deb 2>/dev/null | head -n 1)
if [ -n "$DEB_FILE" ]; then
    if sudo -n true 2>/dev/null; then
        sudo dpkg -i "$DEB_FILE"
        echo "✓ Reinstalled system-wide (/usr/bin/jarvis and ~/.local/bin/jarvis)."
    else
        echo "✓ Reinstalled in ~/.local/bin/jarvis."
        echo "  (For full system-wide /usr/bin/jarvis, run: sudo dpkg -i $DEB_FILE)"
    fi
fi

echo "=== Done! You can now run: jarvis ==="
