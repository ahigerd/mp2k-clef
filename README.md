gbamp2wav
=========

gbamp2wav is a player for MusicPlayer2000 tracks from Game Boy Advance games.

Building
--------
To build on POSIX platforms or MinGW using GNU Make, simply run `make`. The following make
targets are recognized:

* `cli`: builds the command-line tool. (default)
* `gui`: builds the graphical interface.
* `plugins`: builds all player plugins supported by the current platform.
* `all`: builds the command-line tool and all plugins supported by the current platform.
* `debug`: builds a debug version of the command-line tool.
* `guidebug`: builds a debug version of the graphical interface.
* `audacious`: builds just the Audacious player plugin, if supported.
* `winamp`: builds just the Winamp player plugin, if supported.
* `foobar`: builds just the Foobar2000 player plugin, if supported.
* `clap`: builds the CLAP instrument plugin, if supported.
* `clapdebug`: builds a debug version of the CLAP instrument plugin, if supported.
* `aud_gbamp2wav_d.dll`: builds a debug version of the Audacious plugin, if supported.
* `in_gbamp2wav_d.dll`: builds a debug version of the Winamp plugin, if supported.

The following make variables are also recognized:

* `CROSS=mingw`: If building on Linux, use MinGW to build 32-bit Windows binaries.
* `CROSS=mingw64`: If building on Linux, use MinGW to build 64-bit Windows binaries.
* `CROSS=msvc`: Use Microsoft Visual C++ to build Windows binaries, using Wine if the current
  platform is not Windows. (Required to build the Foobar2000 plugin.)
* `WINE=[command]`: Sets the command used to run Wine. (Default: `wine`)
* `QMAKE=[command]`: Sets the command used to invoke qmake for GUI builds. (Default: `qmake`)

To build using Microsoft Visual C++ on Windows without using GNU Make, run `buildvs.cmd`,
optionally with one or more build targets. The following build targets are supported:

* `cli`: builds the command-line tool. (default)
* `plugins`: builds the Winamp and Foobar2000 plugins.
* `all`: builds the command-line tool and the Winamp and Foobar2000 plugins.
* `winamp`: builds just the Winamp plugin.
* `foobar`: builds just the Foobar2000 plugin.

Separate debug builds are not supported with Microsoft Visual C++, but the build flags may be
edited in `msvc.mak`.

License
-------
gbamp2wav is copyright (c) 2021-2023 Adam Higerd and distributed under the terms of the
[MIT license](LICENSE.md).

This project is based upon seq2wav, copyright (c) 2020-2023 Adam Higerd and distributed
under the terms of the [MIT license](LICENSE.md).

[CLAP](https://cleveraudio.org/) is an open-source audio plugin format. The
[CLAP SDK](https://github.com/free-audio/clap) is copyright (c) 2021 Alexander BIQUE
and distributed under the terms of the [MIT license](LICENSE.CLAP).
