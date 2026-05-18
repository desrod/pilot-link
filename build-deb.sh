#!/usr/bin/env bash
#
# Build a Debian package (.deb) from the pilot-link source tree.
# Run from the project root. Requires build dependencies (see debian/control).
#
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR" && pwd)"
cd "$ROOT_DIR"

# Optional: regenerate configure and Makefiles (use when building from git without committed configure)
RECONFIGURE=false
if [[ "${1:-}" == "--reconfigure" ]]; then
	RECONFIGURE=true
	shift
fi

if [[ ! -f debian/control ]]; then
	echo "Error: debian/control not found. Run this script from the pilot-link source root." >&2
	exit 1
fi

# Regenerate configure if missing or if configure.ac is newer (e.g. after a fix)
if [[ ! -f configure ]] || [[ configure.ac -nt configure ]] || [[ "$RECONFIGURE" == true ]]; then
	echo "Generating configure and Makefiles..."
	NOCONFIGURE=1 ./autogen.sh
fi

# dpkg-buildpackage writes the .deb to the parent directory; it must be writable
PARENT_DIR="$(dirname "$ROOT_DIR")"
if [[ ! -w "$PARENT_DIR" ]]; then
	echo "Error: Parent directory '$PARENT_DIR' is not writable. The .deb is written there." >&2
	echo "Run from a tree where the parent directory is writable." >&2
	exit 1
fi

# Clear stale debhelper cache. Path hashes can become invalid when
# the source tree location changes (e.g. moved/renamed checkout).
if [[ -d debian/.debhelper ]]; then
	echo "Removing stale debhelper state..."
	rm -rf debian/.debhelper
fi

echo "Building Debian package..."
# -us: do not sign source package; -uc: do not sign .changes; -b: binary-only
dpkg-buildpackage -us -uc -b "$@"

echo "Done. .deb and related files are in: $PARENT_DIR"
