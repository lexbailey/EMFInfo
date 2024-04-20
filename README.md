# EMF Info

!! Warning !! This is a work-in-progress and is not finished yet.

A utility for browsing the [EMF Camp](https://www.emfcamp.org/) schedule and camp map (if I get around to implementing the map) entirely offline.

## Platforms

The primary supported platforms are:
- ZX Spectrum 48k or compatible
- Linux (most modern versions of)

## Build dependencies

ZX Spectrum:

    sdcc (version 4.4.0 or compatible. Run ./get_sdcc to fetch and unpack it into the working directory)
    fuse-emulator-utils (optional, only used for making wav files from tap files with tap2wav)

Linux:

    nothing that isn't in build-essential (or your distro's equivalent package)

## Building

ZX Spectrum:

    make emfinfo_zxspec48.tap

Linux:

    make emfinfo_linux
