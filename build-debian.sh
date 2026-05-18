#!/usr/bin/env bash
set -e

# Backward-compatible entrypoint.
exec "$(dirname "$0")/build-deb.sh" "$@"
