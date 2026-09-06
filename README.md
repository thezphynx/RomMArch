# RomMArch

A Nintendo 3DS-focused modification of RetroArch that adds RomM server
integration for ROM library browsing, downloads, and save synchronization.

This project is under development.

## Credits

RomMArch builds on [RetroArch](https://github.com/libretro/RetroArch),
developed by the Libretro project and its contributors.

RomMArch-specific modifications and integration are maintained by
[thezphynx](https://github.com/thezphynx).

Additional projects that make this work possible:

- [RomM](https://github.com/rommapp/romm): server and API.
- [devkitPro](https://devkitpro.org/): Nintendo 3DS build toolchain.
- [libctru](https://github.com/devkitPro/libctru): Nintendo 3DS homebrew library.

RomMArch is an independent modification and does not imply endorsement
by these projects.

## Upstream documentation and licenses

The original RetroArch README is preserved in
[README.upstream.md](README.upstream.md). Its download and support links
refer to upstream RetroArch.

See [COPYING](COPYING) for the included GNU GPL v3 license text.
Existing copyright notices, author credits, and component licenses
are retained.

## Configuration

Personal settings are stored on the 3DS in
`sdmc:/retroarch/rommarch.cfg`.

Do not publish your API token or personal configuration file.

## Feedback

Report RomMArch-specific issues in
[this repository](https://github.com/thezphynx/RomMArch/issues).

## Building the Nintendo 3DS frontend

The development setup uses Windows with the devkitPro MSYS terminal.

Recorded toolchain versions:

- devkitARM r68
- ARM GCC 16.1.0
- libctru 2.7.0
- 3dstools 1.3.1
- picasso 2.7.2

The Makefile expects `DEVKITPRO` and `DEVKITARM` to be set.
The repository includes the bannertool and makerom executables.

From the repository root:

```bash
make -f Makefile.ctr clean
make -f Makefile.ctr HAVE_STATIC_DUMMY=1 USE_CTRULIB_2=1 -j2
```

The default output filenames are `retroarch_3ds.3dsx` and
`retroarch_3ds.cia`.

This builds the RomMArch frontend with a dummy core. It does not
include a playable emulator core. Gameplay requires separate core
builds; instructions for those are not yet provided here.

Personal configuration and generated binaries are excluded from
new Git additions. Compiled releases can be distributed through
GitHub Releases.

## Building a complete package

With the build prerequisites installed, run from the repository root:

```bash
make -f Makefile.ctr clean
make -f Makefile.ctr HAVE_STATIC_DUMMY=1 USE_CTRULIB_2=1 -j2
bash package-rommarch.sh
```

Packaging requires curl, sha256sum, and 7-Zip. On Windows, the script
also checks the standard `C:\Program Files\7-Zip\7z.exe` location.

The script automatically downloads the RetroArch 1.17.0 Nintendo 3DS
CIA distribution, verifies its pinned SHA-256 checksum, and combines
its supporting folders and prebuilt cores with the compiled RomMArch
frontend. No manual download or selection of cores is required.

The completed package is written to a fresh
`RomMArch-package.XXXXXX` folder in your home directory. It includes
installation instructions and the supporting archive's source URL
and checksum.

### Core compatibility baseline

RomMArch intentionally packages Nintendo 3DS emulator cores and
supporting files from RetroArch 1.17.0. Later core builds tested by
the maintainer exhibited touchscreen crashes that remain unresolved
in this project. RetroArch 1.17.0 remains the supported baseline until a newer version is
explicitly validated.

Not every bundled core has been individually tested. The bundled core
executables retain their original RetroArch frontend; the RomMArch
submenu belongs to the separately built frontend.

### Installing the CIA package

1. Copy the supplied `retroarch` folder to the SD card root, producing
   `sdmc:/retroarch/`.
2. Copy `retroarch_3ds.cia` onto the SD card and install it using FBI.
3. Open RomMArch and launch the desired core through the frontend,
   allowing it to install the bundled core on first launch.
4. Configure your server, API token, ROM directory, and save sync.

The `.3dsx` frontend is an alternative launch format and is not required
for CIA installation.

Back up an existing installation before copying. Merging folders can
leave extra newer core installers behind. Existing RetroArch settings
may override default directory paths.
