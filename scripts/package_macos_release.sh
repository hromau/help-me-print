#!/usr/bin/env bash
set -euo pipefail

if [ "${1:-}" = "" ] || [ "${2:-}" = "" ]; then
  echo "usage: $0 <app-bundle> <release-dir>" >&2
  exit 2
fi

APP_BUNDLE="$1"
RELEASE_DIR="$2"
SIGN_IDENTITY="${MACOS_CERTIFICATE_NAME:-}"
NOTARY_PROFILE="${MACOS_NOTARY_PROFILE:-}"

if [ ! -d "$APP_BUNDLE" ]; then
  echo "App bundle not found: $APP_BUNDLE" >&2
  exit 1
fi

mkdir -p "$RELEASE_DIR"

app_dir="$(cd "$(dirname "$APP_BUNDLE")" && pwd)"
app_name="$(basename "$APP_BUNDLE" .app)"
dmg_path="$RELEASE_DIR/$app_name.dmg"

if [ -n "$SIGN_IDENTITY" ]; then
  "$(cd "$(dirname "$0")" && pwd)/sign_macos_app.sh" "$APP_BUNDLE" "$SIGN_IDENTITY"
else
  "$(cd "$(dirname "$0")" && pwd)/sign_macos_app.sh" "$APP_BUNDLE"
fi

codesign --verify --deep --strict --verbose=2 "$APP_BUNDLE"

hdiutil create \
  -volname "$app_name" \
  -srcfolder "$app_dir" \
  -format UDZO \
  -ov \
  "$dmg_path"

if [ -n "$SIGN_IDENTITY" ] && [ -n "$NOTARY_PROFILE" ]; then
  xcrun notarytool submit "$dmg_path" --keychain-profile "$NOTARY_PROFILE" --wait
  xcrun stapler staple "$APP_BUNDLE"
  xcrun stapler staple "$dmg_path"
  spctl -a -vv "$dmg_path"
else
  echo "warning: macOS release artifact is not notarized; Gatekeeper may reject it." >&2
fi
