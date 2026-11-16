#!/usr/bin/env bash
#
# Build Debian packages from the pilot-link source tree.
# Run from the project root. Requires build dependencies (see debian/control).
#
# By default this produces everything a release needs:
#
#   pilot-link-VERSION.tar.gz / .tar.xz    upstream tarball (make dist)
#   pilot-link_VERSION.orig.tar.gz         the same tarball, named for dpkg
#   pilot-link_VERSION-REV.dsc             Debian source package
#   pilot-link_VERSION-REV.debian.tar.xz   Debian packaging
#   *_amd64.deb / *_amd64.ddeb             binary packages
#
# Use --binary-only to skip all of that and build just the .debs in place,
# which is the fast loop when you are iterating on the packaging itself.
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR" && pwd)"
cd "$ROOT_DIR"

usage() {
	cat <<'USAGE'
Usage: ./build-deb.sh [options] [-- dpkg-buildpackage args]

  --reconfigure   Force regeneration of configure and Makefiles
  --binary-only   Build only the .deb packages, in place (no tarball, no .dsc)
  --no-dist       Skip "make dist" and reuse an existing .orig.tar.gz
  -h, --help      Show this message

Any remaining arguments are passed through to dpkg-buildpackage.
USAGE
}

RECONFIGURE=false
BINARY_ONLY=false
BUILD_DIST=true
while [[ $# -gt 0 ]]; do
	case "$1" in
		--reconfigure) RECONFIGURE=true; shift ;;
		--binary-only) BINARY_ONLY=true; BUILD_DIST=false; shift ;;
		--no-dist)     BUILD_DIST=false; shift ;;
		-h|--help)     usage; exit 0 ;;
		--)            shift; break ;;
		*)             break ;;
	esac
done

if [[ ! -f debian/control ]]; then
	echo "Error: debian/control not found. Run this script from the pilot-link source root." >&2
	exit 1
fi

# src/parsedate.c is generated from parsedate.y and is not in git, so building
# from this tree needs a yacc: "make dist" generates it to ship it, and
# --binary-only compiles it. Building from the tarball does not, which is why
# it is not in Build-Depends. Without this check the failure is a bare
# "yacc: command not found" from deep inside make dist.
if { [[ "$BUILD_DIST" == true ]] ||
     { [[ "$BINARY_ONLY" == true ]] && [[ ! -f src/parsedate.c ]]; }; } &&
   ! command -v bison >/dev/null && ! command -v byacc >/dev/null &&
   ! command -v yacc >/dev/null; then
	echo "Error: building from git needs bison (or byacc) to generate src/parsedate.c." >&2
	echo "Install it with: sudo apt install bison" >&2
	exit 1
fi

# Aux scripts automake installs with --add-missing. configure refuses to run
# without them ("cannot find required auxiliary files"). They are gitignored,
# and dh_autoreconf_clean deletes them on every in-tree build, so the presence
# of configure does NOT imply they are still there.
AUX_FILES=(compile missing install-sh depcomp test-driver ylwrap config.guess config.sub)

build_system_ready() {
	[[ -f configure ]] || return 1
	[[ configure.ac -nt configure ]] && return 1
	local f
	for f in "${AUX_FILES[@]}"; do
		# -f follows symlinks, so a symlink left dangling by an automake
		# upgrade counts as missing, which is what we want.
		[[ -f "$f" ]] || return 1
	done
	return 0
}

# Regenerate configure if it is missing or stale, or if any aux script is gone
if [[ "$RECONFIGURE" == true ]] || ! build_system_ready; then
	echo "Generating configure and Makefiles..."
	NOCONFIGURE=1 ./autogen.sh
fi

# dpkg-buildpackage writes its output to the parent directory; it must be writable
PARENT_DIR="$(dirname "$ROOT_DIR")"
if [[ ! -w "$PARENT_DIR" ]]; then
	echo "Error: Parent directory '$PARENT_DIR' is not writable. The .deb is written there." >&2
	echo "Run from a tree where the parent directory is writable." >&2
	exit 1
fi

# Pass Debian's flags through unmodified, including hardening options like
# -Werror=format-security. configure.ac no longer rewrites CFLAGS.
#
# Call this from the directory the package is built in: dpkg-buildflags
# embeds the current directory in -ffile-prefix-map, and a map for the wrong
# directory leaves the build path in the debug info instead of /usr/src/....
export_build_flags() {
	export CFLAGS="$(dpkg-buildflags --get CFLAGS)"
	export CXXFLAGS="$(dpkg-buildflags --get CXXFLAGS)"
	export CPPFLAGS="$(dpkg-buildflags --get CPPFLAGS)"
	export LDFLAGS="$(dpkg-buildflags --get LDFLAGS)"
}
export_build_flags

# --binary-only is the old in-place behaviour: no source package, so dpkg-source
# never has to reconcile this working tree against an upstream tarball.
if [[ "$BINARY_ONLY" == true ]]; then
	echo "Building Debian packages (binary only)..."
	rm -rf debian/.debhelper
	dpkg-buildpackage -us -uc -b "$@"
	echo "Done. Artifacts are in: $PARENT_DIR"
	exit 0
fi

SOURCE_NAME="$(dpkg-parsechangelog -S Source)"
FULL_VERSION="$(dpkg-parsechangelog -S Version)"
VERSION_NO_EPOCH="${FULL_VERSION#*:}"
UPSTREAM_VERSION="${VERSION_NO_EPOCH%-*}"
ORIG_TARBALL="$PARENT_DIR/${SOURCE_NAME}_${UPSTREAM_VERSION}.orig.tar.gz"

# Build the upstream tarball first. dpkg-buildpackage runs "debian/rules clean",
# which distcleans the tree, so anything that needs a configured tree has to
# happen before it.
if [[ "$BUILD_DIST" == true ]]; then
	echo "Building upstream tarball (make dist)..."

	# This configures in place. An out-of-tree build is not an option while
	# the source tree holds a config.status (autoconf refuses: "source
	# directory already configured"), and nothing here is worth preserving:
	# the package is built from the tarball, not from this tree.
	if [[ -f Makefile ]]; then
		echo "Cleaning the existing build tree..."
		make distclean >/dev/null 2>&1 || true
	fi

	# distclean can take the aux scripts with it depending on automake
	# version, so re-check rather than assume the earlier pass still holds.
	if ! build_system_ready; then
		echo "Regenerating build system after clean..."
		NOCONFIGURE=1 ./autogen.sh
	fi

	# --enable-xsltproc matters: doc/man/*.1 are generated from the DocBook
	# source, and doc/man/Makefile.am only lists them in EXTRA_DIST when
	# xsltproc is enabled. Without it "make dist" happily ships a tarball
	# with every man page missing and no warning. Match the flags
	# debian/rules uses so the tarball matches what the packages are built from.
	./configure --enable-conduits --enable-xsltproc

	# doc/xml's "all" target is what actually runs xsltproc, writing the man
	# pages into $(top_srcdir)/doc/man/ (they are gitignored). They must
	# exist before "make dist" tries to collect them.
	make -C doc/xml

	make dist

	# The hyphenated tarballs are the upstream release artifacts; dpkg wants
	# the same bytes under an underscored .orig name.
	mv -f "${SOURCE_NAME}-${UPSTREAM_VERSION}".tar.gz "${SOURCE_NAME}-${UPSTREAM_VERSION}".tar.xz "$PARENT_DIR"/
	cp -f "$PARENT_DIR/${SOURCE_NAME}-${UPSTREAM_VERSION}.tar.gz" "$ORIG_TARBALL"
fi

if [[ ! -f "$ORIG_TARBALL" ]]; then
	echo "Error: $ORIG_TARBALL not found." >&2
	echo "Run without --no-dist to generate it with \"make dist\"." >&2
	exit 1
fi

# Clear debhelper's droppings (debian/tmp, debian/<pkg>/, *.substvars, editor
# backups) so none of them get copied into the staging tree and packaged.
if command -v dh_clean >/dev/null 2>&1; then
	dh_clean >/dev/null 2>&1 || true
fi

# Build the package from a pristine unpack of the orig tarball with debian/
# copied in, rather than from this working tree. Under 3.0 (quilt) dpkg-source
# diffs the build tree against the orig tarball and aborts on any change
# outside debian/ that is not a quilt patch. This tree would trip that on
# every file git tracks but "make dist" does not ship (README.md, build-deb.sh,
# .gitignore, ...), plus any stray untracked file. Unpacking the tarball
# guarantees the only difference is debian/, which is exactly what the
# .debian.tar.xz is for. It also means the .debs are built from the same bytes
# that get released.
BUILD_AREA="$PARENT_DIR/.${SOURCE_NAME}-build-area"
STAGE_DIR="$BUILD_AREA/${SOURCE_NAME}-${UPSTREAM_VERSION}"
rm -rf "$BUILD_AREA"
mkdir -p "$BUILD_AREA"
trap 'rm -rf "$BUILD_AREA"' EXIT

cp -f "$ORIG_TARBALL" "$BUILD_AREA/"
tar -xzf "$ORIG_TARBALL" -C "$BUILD_AREA"
cp -a "$ROOT_DIR/debian" "$STAGE_DIR/debian"

echo "Building Debian source and binary packages..."
# -us: do not sign source package; -uc: do not sign .changes. No -b, so this
# builds the .dsc as well as the binaries.
( cd "$STAGE_DIR" && export_build_flags && dpkg-buildpackage -us -uc "$@" )

# Collect the results next to the upstream tarballs and drop the staging tree.
find "$BUILD_AREA" -maxdepth 1 -type f \
	\( -name '*.dsc' -o -name '*.debian.tar.*' -o -name '*.deb' -o -name '*.ddeb' \
	   -o -name '*.changes' -o -name '*.buildinfo' \) \
	-exec mv -f {} "$PARENT_DIR"/ \;
rm -rf "$BUILD_AREA"
trap - EXIT

echo "Done. Artifacts are in: $PARENT_DIR"
