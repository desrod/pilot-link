#!/usr/bin/env bash
#
# Build Debian packages from the pilot-link source tree.
# Run from the project root. Requires build dependencies (see debian/control).
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR" && pwd)"
cd "$ROOT_DIR"

sanitize_warning_error_flags() {
	local token
	local sanitized=()

	for token in $1; do
		case "$token" in
			-Werror|-Werror=*)
				;;
			*)
				sanitized+=("$token")
				;;
		esac
	done

	printf '%s ' "${sanitized[@]}"
}

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

echo "Building Debian package..."
# -us: do not sign source package; -uc: do not sign .changes; -b: binary-only
# Clear stale debhelper state so package splits/installs are recalculated cleanly.
rm -rf debian/.debhelper

# configure.ac strips "-Werror" naively, which corrupts Debian flags like
# "-Werror=format-security" into "=format-security". Preserve the normal Debian
# flags but remove the -Werror entries before running the package build.
# Capture dpkg-buildflags output before sanitizing: a command substitution
# nested inside the sanitizer would mask its exit status, so a dpkg-buildflags
# failure would not abort under "set -e" (unlike the CPPFLAGS/LDFLAGS lines).
RAW_CFLAGS="$(dpkg-buildflags --get CFLAGS)"
RAW_CXXFLAGS="$(dpkg-buildflags --get CXXFLAGS)"
BUILD_CFLAGS="$(sanitize_warning_error_flags "$RAW_CFLAGS")"
BUILD_CXXFLAGS="$(sanitize_warning_error_flags "$RAW_CXXFLAGS")"
BUILD_CPPFLAGS="$(dpkg-buildflags --get CPPFLAGS)"
BUILD_LDFLAGS="$(dpkg-buildflags --get LDFLAGS)"

CFLAGS="$BUILD_CFLAGS" \
CXXFLAGS="$BUILD_CXXFLAGS" \
CPPFLAGS="$BUILD_CPPFLAGS" \
LDFLAGS="$BUILD_LDFLAGS" \
dpkg-buildpackage -us -uc -b "$@"

echo "Done. .deb and related files are in: $PARENT_DIR"
