#!/usr/bin/env bash

set -euo pipefail

if [ "$#" -ne 3 ]; then
  cat >&2 <<'EOF'
Usage: scripts/generate-winget-manifests.sh <version> <installer-url> <sha256>

Example:
  scripts/generate-winget-manifests.sh \
    0.1.5 \
    https://github.com/hromau/help-me-print/releases/download/v0.1.5/help-me-print-0.1.5-win64.exe \
    0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF0123456789ABCDEF
EOF
  exit 1
fi

version="$1"
installer_url="$2"
sha256="$(printf '%s' "$3" | tr '[:lower:]' '[:upper:]')"

package_id="Easure.HelpMePrint"
manifest_dir="packaging/winget/manifests/e/Easure/HelpMePrint/${version}"

mkdir -p "$manifest_dir"

cat > "${manifest_dir}/${package_id}.yaml" <<EOF
# yaml-language-server: \$schema=https://aka.ms/winget-manifest.defaultLocale.1.10.0.schema.json
PackageIdentifier: ${package_id}
PackageVersion: ${version}
ManifestType: version
ManifestVersion: 1.10.0
EOF

cat > "${manifest_dir}/${package_id}.installer.yaml" <<EOF
# yaml-language-server: \$schema=https://aka.ms/winget-manifest.installer.1.10.0.schema.json
PackageIdentifier: ${package_id}
PackageVersion: ${version}
InstallerType: nullsoft
Scope: machine
UpgradeBehavior: install
Installers:
  - Architecture: x64
    InstallerUrl: ${installer_url}
    InstallerSha256: ${sha256}
ManifestType: installer
ManifestVersion: 1.10.0
EOF

cat > "${manifest_dir}/${package_id}.locale.en-US.yaml" <<EOF
# yaml-language-server: \$schema=https://aka.ms/winget-manifest.defaultLocale.1.10.0.schema.json
PackageIdentifier: ${package_id}
PackageVersion: ${version}
PackageLocale: en-US
Publisher: Easure
PublisherUrl: https://github.com/hromau/help-me-print
PublisherSupportUrl: https://github.com/hromau/help-me-print/issues
PackageName: Help Me Print
PackageUrl: https://github.com/hromau/help-me-print
ShortDescription: Manual duplex printing for printers without automatic double-sided support
Moniker: duplexprint
Tags:
  - duplex
  - double-sided
  - printing
  - printer
  - pdf
  - manual-duplex
License: Proprietary
Copyright: Copyright (c) Easure
ReleaseNotesUrl: https://github.com/hromau/help-me-print/releases/tag/v${version}
ManifestType: defaultLocale
ManifestVersion: 1.10.0
EOF

printf 'Generated winget manifests in %s\n' "$manifest_dir"
