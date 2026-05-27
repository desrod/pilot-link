# pilot-link

> The glue between your Palm and your computer.

**pilot-link** is the userland toolkit and library suite for talking to Palm OS handheld devices — Pilots, Visors, Tungstens, Treos, Zires, the whole family — from Linux, macOS, and the BSDs. It's been around since 1996, predating most of the desktop sync software the devices originally shipped with, and it's still maintained.

If you've found this project, you almost certainly already know why you need it. Welcome back.

---

## Status

Palm OS hardware hasn't shipped a new model in over fifteen years, but the devices that exist are still around, still loved, and still useful — and pilot-link is how you keep them talking to a modern computer. The codebase is actively maintained: bug reports get triaged, build errors on current toolchains get fixed, and contributions are welcome.

Recent focus has been on:

- Getting the source to compile cleanly under modern Clang and GCC (C23, `-Wall` clean)
- Real device testing against a Tungsten E2 over USB on macOS (Apple Silicon)
- A new `pilot-contacts` tool that round-trips Palm OS 5 `ContactsDB-PAdd` to and from standard vCard 3.0 files, with iconv charset conversion for non-Latin contact names
- Hardening against malformed records (bounds checks in the unpack paths)
- Modernized Perl bindings (PL_-prefixed globals) and Python bindings ported off SWIG 1.3 / Python 2 onto SWIG 4 / Python 3

If you're trying it on a modern distro and hitting a build error, please open an issue — most of those are quick fixes.

---

## Quick start

Plug in your Palm, put it in HotSync mode, and:

```bash
# Linux
pilot-xfer -p /dev/ttyUSB1 --list

# macOS (Darwin native USB)
pilot-xfer -p usb: --list
```

That should print every database on the device. From there, `pilot-xfer --backup ~/PalmBackup/` is the workhorse for a full backup; the rest of the tools handle individual data types.

For contacts specifically:

```bash
# Export contacts to a vCard 3.0 file
pilot-contacts -p usb: --write ~/contacts.vcf

# Import a vCard file (use --delete-all first if you want a clean slate)
pilot-contacts -p usb: --delete-all --read ~/contacts.vcf
```

---

## What's in the box

The project has four layers, in roughly this order:

### Libraries (`libpisock/`, `libpisync/`)

The C library that does all the real work — packet framing, the DLP protocol, USB and serial transports, and the dozen-plus record-type encoders/decoders (Address, Calendar, Memo, ToDo, Mail, Contacts, Photos, and so on). Other projects link against this directly; [J-Pilot](http://www.jpilot.org/), the GTK desktop sync app, is a prominent example.

### Conduits (`src/`)

The command-line tools you actually run. A non-exhaustive tour:

| Tool | What it does |
|---|---|
| `pilot-xfer` | Backup, restore, install, and inspect databases. The Swiss Army knife. |
| `dlpsh` | Interactive DLP shell — poke at the device protocol-level |
| `pilot-addresses` | Classic AddressDB import/export (CSV, vCard) |
| `pilot-contacts` | Palm OS 5 ContactsDB-PAdd (vCard 3.0, iconv-aware) |
| `pilot-memos` | MemoDB import/export |
| `pilot-read-todos` | ToDo database reader |
| `pilot-foto`, `pilot-read-palmpix` | Pull photos off the device |
| `pilot-read-screenshot` | Grab screenshots from devices that support it |
| `pilot-install-*` | Install datebooks, expenses, memos, todos, .prc apps |

`./configure --help` will show the full set, and `<tool> --help` shows per-tool options.

### Language bindings (`bindings/`)

If you want to script the sync logic instead of stringing the C tools together:

- **Perl** (XS) — the most mature binding
- **Python** (SWIG 4 / Python 3)
- **Java** (JNI)
- **Tcl**

Each lives under its own subdirectory with its own build wiring. They all expose roughly the same surface as the C library.

### Tests (`tests/`)

Regression tests for the pack/unpack functions and DLP protocol. `make check` runs them.

---

## Building

pilot-link uses autotools. The high-level shape is the same everywhere:

```bash
./autogen.sh        # only needed for a git checkout
./configure         # see --help for the long list of options
make
sudo make install   # only when you're satisfied
```

A bare `./configure` will give you a minimal build. To get the conduits, language bindings, and man pages, pass the corresponding `--enable-` / `--with-` flags. `./configure --help` is your friend here — there are a lot of knobs.

A reasonable "build everything" line for development:

```bash
./configure --enable-conduits --enable-xsltproc=yes --with-python=yes
```

### Linux

Most distributions package the build dependencies. On Debian/Ubuntu:

```bash
sudo apt install build-essential autoconf automake libtool pkg-config \
                 libusb-dev libpng-dev libreadline-dev libpopt-dev \
                 docbook-xsl xsltproc
```

Add `python3-dev`, `perl`, `swig`, etc. if you want those bindings.

### macOS

Install Xcode command-line tools, then Homebrew, then:

```bash
brew install autoconf automake libtool pkg-config \
             libusb libpng readline popt \
             docbook-xsl xsltproc swig
```

The macOS USB backend uses IOKit directly (no libusb needed). After building, devices show up as port `usb:`:

```bash
pilot-xfer -p usb: --list
```

### FreeBSD

```bash
pkg install autoconf automake libtool pkg-config \
            libusb libpng readline popt \
            docbook-xsl libxslt
```

You may need to point the build at `/usr/local`:

```bash
ACLOCAL_FLAGS="-I /usr/local/share/aclocal/" \
LDFLAGS="-L/usr/local/lib" \
CFLAGS="-I/usr/local/include" \
CPPFLAGS="-I/usr/local/include" \
./configure --enable-conduits
```

### Windows

Building under Cygwin and talking to the device through a virtual COM port has worked historically. There's no active support for it now — patches welcome from anyone who runs that setup.

---

## USB and serial notes

- **USB on Linux** needs either the kernel `visor` driver (`sudo modprobe visor`) — which exposes the device as `/dev/ttyUSB*` — or libusb. See [`doc/README.usb`](doc/README.usb) and [`doc/README.libusb`](doc/README.libusb).
- **USB on macOS** uses IOKit; specify `-p usb:` and it just works.
- **Serial** still works fine if you have an old cradle and a USB-to-serial adapter. The default rate is 9600 baud; faster rates can be requested via the `PILOTRATE` environment variable, but USB is faster and more reliable when it's an option.

---

## Contributing

Bug reports, feature requests, and pull requests all welcome:

- **Issues:** <https://github.com/desrod/pilot-link/issues>
- **Pull requests:** <https://github.com/desrod/pilot-link/pulls>
- **Discussions:** <https://github.com/desrod/pilot-link/discussions>

Before filing a bug, please check whether someone else has already reported it. When you do file one, include:

- OS and version
- pilot-link version (`pilot-xfer --version`)
- Palm device model and Palm OS version
- The exact command that failed and its output
- Steps to reproduce

If GitHub isn't an option for you, the project maintainer is reachable at [desrod@gnu-designs.com](mailto:desrod@gnu-designs.com).

There's debugging guidance — including how to capture protocol traces — in [`doc/README.debugging`](doc/README.debugging). It's the fastest way to get a hardware-specific bug diagnosed.

---

## Embedding pilot-link in your own code

If you're writing software that needs to talk to a Palm device, link against `libpisock` rather than shelling out to the command-line tools. The headers are organized so you only need to include what you use:

| Header | What it gives you |
|---|---|
| `pi-source.h` | Portability layer (only if you want it; autoconf-configured) |
| `pi-file.h` | `.pdb` and `.prc` file access |
| `pi-dlp.h` | DLP sync functions |
| `pi-macros.h` | Portable byte-access helpers (`get_short`, `set_long`, etc.) |
| `pi-address.h`, `pi-contact.h`, `pi-memo.h`, ... | Per-record-type pack/unpack |

The `bindings/` subdirectories double as fairly complete usage examples — particularly the Perl XS code, which exercises most of the public API surface.

---

## History and archive links

The project's older websites are gone, but the Wayback Machine has them:

- [Main website (archived)](https://web.archive.org/web/2016/http://www.pilot-link.org/)
- [HOWTO documents (archived)](https://web.archive.org/web/2010/http://howto.pilot-link.org/)
- [Code documentation / Doxygen (archived)](https://web.archive.org/web/2010/http://doxygen.pilot-link.org/)
- [Mailing list archive (archived)](https://web.archive.org/web/2015/http://lists.pilot-link.org/)

Before GitHub Discussions, the project hung out on IRC — `#pilot-link` on `irc.pilot-link.org`, where the maintainer went by `setuid` and there was usually a dozen people around at any given time. Those days are gone, but the welcome hasn't changed.

---

## License

pilot-link is dual-licensed:

- **Tools** under `src/` (the conduits — `pilot-xfer`, `dlpsh`, `pilot-contacts`, and friends) are **GPL**. See [`COPYING`](COPYING).
- **Libraries** (`libpisock`, `libpisync`) and the **language bindings** are **LGPL**. See [`COPYING.LIB`](COPYING.LIB).

The LGPL on the library side is deliberate: it means proprietary applications can link against `libpisock` to add Palm sync support without becoming GPL themselves. If you ship such a thing, please at least let us know it exists — we like hearing about it.

Individual files carry their own copyright notices; check the header of any file for specifics.

---

## Credits

pilot-link was originally written by Kenneth Albanowski (`<kjahds@kjahds.com>`) in 1996, with contributions over the years from a long list of people — many of whose names are in [`AUTHORS`](AUTHORS) and the commit history. The project is currently maintained by David A. Desrosiers (`<desrod@gnu-designs.com>`).

If you're using pilot-link in something interesting, drop a note in [Discussions](https://github.com/desrod/pilot-link/discussions). After thirty years, there are still new stories to hear.

Happy hacking.
