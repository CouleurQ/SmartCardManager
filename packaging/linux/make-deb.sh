#!/usr/bin/env bash
# Builds a .deb from an already built binary.
#   make-deb.sh <binary> <version> <output dir>
set -euo pipefail

BINARY="$(realpath "$1")"
VERSION="$2"
OUT="$(realpath "$3")"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$HERE/../.." && pwd)"

# Debian versions: '-' separates the packaging revision, so keep the whole version in the upstream part
DEB_VERSION="${VERSION//-/+}"
STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT

install -Dm755 "$BINARY" "$STAGE/pkg/usr/bin/SmartCardManager"
install -Dm644 "$HERE/smartcardmanager.desktop" "$STAGE/pkg/usr/share/applications/smartcardmanager.desktop"
install -Dm644 "$ROOT/res/icons/logo.svg" "$STAGE/pkg/usr/share/icons/hicolor/scalable/apps/smartcardmanager.svg"
install -Dm644 "$ROOT/LICENSE" "$STAGE/pkg/usr/share/doc/smartcardmanager/copyright"

# dependencies from the libraries the binary links against
mkdir -p "$STAGE/debian" "$STAGE/pkg/DEBIAN"
touch "$STAGE/debian/control"
DEPENDS="$(cd "$STAGE" && dpkg-shlibdeps -O -e"$STAGE/pkg/usr/bin/SmartCardManager" 2>/dev/null | sed 's/^shlibs:Depends=//')"

cat > "$STAGE/pkg/DEBIAN/control" <<CONTROL
Package: smartcardmanager
Version: $DEB_VERSION
Section: utils
Priority: optional
Architecture: amd64
Maintainer: CouleurQ <couleurq@gmail.com>
Depends: $DEPENDS
Recommends: pcscd
Homepage: https://github.com/CouleurQ/SmartCardManager
Description: Smart card communication and Java Card management
 Talks to smart cards through PC/SC readers: send APDUs and scripts,
 inspect the ATR and manage Java Cards.
CONTROL

mkdir -p "$OUT"
dpkg-deb --build --root-owner-group "$STAGE/pkg" "$OUT/smartcardmanager_${DEB_VERSION}_amd64.deb"
