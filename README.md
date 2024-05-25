# EMF Info

A utility for browsing the [EMF Camp](https://www.emfcamp.org/) schedule and camp map (if I get around to implementing the map) entirely offline.

## Platforms

The primary supported platforms are:
- ZX Spectrum 48k or compatible
- Linux (most modern versions of)
- MS-DOS (with or without terminal emulation)

## Build dependencies

ZX Spectrum:

    sdcc (version 4.4.0 or compatible. Run ./get_sdcc to fetch and unpack it into the working directory)
    fuse-emulator-utils (optional, only used for making wav files from tap files with tap2wav)

Linux:

    nothing that isn't in build-essential (or your distro's equivalent package)

MS DOS:

    Watcom C compiler (Open Watcom V2, run ./get_watcom to fetch a copy to the working directory)

## Building

ZX Spectrum:

    make emfinfo_zxspec48.tap

Linux:

    make emfinfo_linux


MS DOS:

    make emfinfo_msdos.exe
    make emfinfo_msdos_textonly.exe # does not depend on terminal emulation
