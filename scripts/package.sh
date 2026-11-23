#!/bin/bash
# Builds a .pkg installer containing the AU, VST3 and standalone app.
# Usage: scripts/package.sh <version>
set -euo pipefail

VERSION="${1:-0.0.0}"
ART="build/ProTunezFXToys_artefacts/Release"
NAME="ProTunez FX Toys"
ID="com.revivalistsmedia.protunezfxtoys"
STAGE="$(mktemp -d)"
mkdir -p dist

stage() { # stage <bundle> <install folder> <pkg id suffix>
    local root="$STAGE/$3/root"
    mkdir -p "$root"
    cp -R "$1" "$root/"
    # Ad-hoc signature so Apple Silicon will load it.
    codesign --force --deep --sign - "$root/$(basename "$1")"
    pkgbuild --root "$root" --install-location "$2" --identifier "$ID.$3" --version "$VERSION" "$STAGE/$3.pkg"
}

stage "$ART/AU/$NAME.component"   "/Library/Audio/Plug-Ins/Components" au
stage "$ART/VST3/$NAME.vst3"      "/Library/Audio/Plug-Ins/VST3"       vst3
stage "$ART/Standalone/$NAME.app" "/Applications"                      app

cat > "$STAGE/distribution.xml" <<EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
  <title>$NAME $VERSION</title>
  <options customize="allow" require-scripts="false" hostArchitectures="arm64"/>
  <choices-outline>
    <line choice="au"/><line choice="vst3"/><line choice="app"/>
  </choices-outline>
  <choice id="au" title="Audio Unit (Logic Pro, GarageBand)"><pkg-ref id="$ID.au"/></choice>
  <choice id="vst3" title="VST3 (Ableton, Cubase, Studio One, etc.)"><pkg-ref id="$ID.vst3"/></choice>
  <choice id="app" title="Standalone app"><pkg-ref id="$ID.app"/></choice>
  <pkg-ref id="$ID.au">au.pkg</pkg-ref>
  <pkg-ref id="$ID.vst3">vst3.pkg</pkg-ref>
  <pkg-ref id="$ID.app">app.pkg</pkg-ref>
</installer-gui-script>
EOF

productbuild --distribution "$STAGE/distribution.xml" --package-path "$STAGE" "dist/ProTunez-FX-Toys-$VERSION.pkg"
echo "Built dist/ProTunez-FX-Toys-$VERSION.pkg"
