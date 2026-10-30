nabbitemu
=========

Nabbitemu is the maintained Linux continuation of wxWabbitemu: a TI-8x
emulator built on wxWidgets, packaged for Fedora, Debian and Arch from the
Nebuff Repo (https://repo.nebuff.xyz/nabbitemu/).

What is wxWabbitemu?
---------------------
wxWabbitemu is a TI-8x emulator that uses the wxWidgets toolkit.
It is based on a decent TI-8x emulator, Wabbitemu.
Hence the name: wxWabbitemu!

It is designed to mainly run on Linux, but it can be run on other
platforms as well.

wxWabbitemu supports loading any TI-8x variables, programs, apps, etc.
It also supports loading and saving calculator states, capturing
screenshots, debugging programs, and much more!

wxWabbitemu History
--------------------
This project was originally started by [Albert Huang (alberthdev)][albert]
as an attempt to port a good TI-8x emulator to Linux.

(At the time, there was TilEm, a GTK+ TI-8X emulator, but it was buggy,
slow, and rarely updated.**)

Thankfully, a Wabbitemu developer, [Chris Shappel (Buckeye)][buckeye],
joined in to do most of the porting. Later, [David Gomes][dgomes] joined
development and helped develop the GUI.

Nowadays, these contributers are part of the project:

 * [Jon "Jonimus" Storm][jonimus]
 * [geekboy][geekboy]

** TilEm2:

At the time of writing, TilEm2, written by new developers
Thibault "contra-sh" Duponchelle and Benjamin "FloppusMaximus" Moody,
was released.

This version sports new emulation code, skinning support, and many
improvements. If you find that wxWabbitemu is causing you too many
problems, development/bug fixes are too slow, and/or you want to play
with something new, give their emulator a try!

http://lpg.ticalc.org/prj_tilem/

Installing nabbitemu
--------------------
nabbitemu is packaged for Fedora, Debian/Ubuntu and Arch from the Nebuff Repo.
Add the repository once, then install it by name:

    curl -sSL https://install.nebuff.xyz/nabbitemu | bash
    sudo dnf install nabbitemu        # Fedora / RHEL
    sudo apt install nabbitemu        # Debian / Ubuntu
    sudo pacman -S nabbitemu          # Arch

Every release, with its checksums and the signing key, is listed at
https://repo.nebuff.xyz/nabbitemu/. Otherwise you can build it from source.

Building nabbitemu
------------------
You need the following:

  * A working C compiler. This includes, but is not limited to,
    GCC and Clang.

  * The wxWidgets development files. Basically, the headers and linking
    libraries for wxWidgets. And of course, the wxWidgets library
    itself.

  * CMake 3.16 or newer, and the GNU Make program (Make is only a thin
    wrapper around CMake here).

In all of the commands that you are instructed to type in, you should
follow each command with pressing the return key.

**Install Dependencies:**

  * Debian, Ubuntu, Mint Linux, and friends:
    
    Run:
    
        sudo apt-get update
        sudo apt-get install build-essential cmake libwxgtk3.2-dev

  * Fedora, RHEL, and friends:
    
    Run:
    
        sudo dnf install gcc gcc-c++ make cmake wxGTK-devel

  * Arch, Manjaro, and friends:
    
    Run:
    
        sudo pacman -S --needed base-devel cmake wxwidgets-gtk3

Once you have installed the dependencies, you can go ahead and
build wxWabbitemu!

**Build nabbitemu:**

In a terminal type:

    make

or, without the make wrapper:

    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel

**Run nabbitemu:**

To run it, type:

    ./build/nabbitemu

Note, though, that wxwabbitemu is a portable file and it can be copied
to anywhere in your computer and ran from any location.

**Install nabbitemu:**

To install, you can type:

    sudo make install

The install step writes `/usr/local/bin/nabbitemu` plus the menu entry and
icon; `make DESTDIR=/tmp/pkg install` stages it for packaging instead.

**Notes:**

A `meson.build` is also provided (`meson setup build && ninja -C build`), and
the legacy `premake/` directory is kept for reference only.

For more information and/or building instructions, visit the
Building page on our website:

https://github.com/alberthdev/wxwabbitemu/wiki/Building

License
--------
See [LICENSE](License)

[albert]: https://github.com/alberthdev
[buckeye]: https://github.com/BuckeyeDude
[dgomes]: https://github.com/davidgomes
[jonimus]: https://github.com/Jonimoose
[geekboy]: https://github.com/geekbozu
