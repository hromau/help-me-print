#!/usr/bin/env bash
set -euo pipefail

if [ "${1:-}" = "" ]; then
  echo "usage: $0 <app-bundle> [codesign-identity]" >&2
  exit 2
fi

APP_BUNDLE="$1"
IDENTITY="${2:--}"

if [ ! -d "$APP_BUNDLE" ]; then
  echo "App bundle not found: $APP_BUNDLE" >&2
  exit 1
fi

codesign_args=(
  --force
  --deep
  --sign "$IDENTITY"
)

if [ "$IDENTITY" = "-" ]; then
  codesign_args+=(--timestamp=none)
else
  codesign_args+=(--options runtime --timestamp)
fi

codesign "${codesign_args[@]}" "$APP_BUNDLE"
codesign --verify --deep --strict --verbose=2 "$APP_BUNDLE"
