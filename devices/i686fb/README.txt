
For details on creating a Device Configuration Profile see:
    http://doc.trolltech.com/qtopia/tut-deviceexample.html.


Device Configuration Profile Status
===================================

-Current status: supported

-gcc details:
Reading specs from /opt/toolchains/i686/gcc-3.4.3-glibc-2.3.4/i686-linux/lib/gcc/i686-linux/3.4.3/specs
Configured with: /backup/crosstool-0.42/build/i686-linux/gcc-3.4.3-glibc-2.3.4/gcc-3.4.3/configure --target=i686-linux --host=i686-host_pc-linux-gnu --prefix=/opt/toolchains/i686/gcc-3.4.3-glibc-2.3.4/i686-linux --with-headers=/opt/toolchains/i686/gcc-3.4.3-glibc-2.3.4/i686-linux/i686-linux/include --with-local-prefix=/opt/toolchains/i686/gcc-3.4.3-glibc-2.3.4/i686-linux/i686-linux --disable-nls --enable-threads=posix --enable-symvers=gnu --enable-__cxa_atexit --enable-languages=c,c++ --enable-shared --enable-c99 --enable-long-long
Thread model: posix
gcc version 3.4.3

-This profile is used with a vmware image to test Qtopia running on x86 architecture using the linux framebuffer. It also uses the mouse turning the mouse cursor on so you can see where the mouse is. For keyboard input it uses tty0, see src/plugins/qtopiacore/kbddrivers/i686fb/kbdhandler.cpp for key mappings.A

-See src/devtools/startup/qpe.sh for startup details


Requirements
============

* Device model               : x86 with framebuffer
* Device manufacturer        : NA
* Supported hardware options : mouse and tty0 keyboard
* Toolchain version          : gcc 3.4.3, but should work with newer gcc version
* Toolchain download location: Use crosstool to create toolchain, standard settings
* Kernel version             : 2.6.x
* Kernel flags               : NA
* glibc version              : 2.3.4
* floating point support used: NA


Configuration for this device
=============================
* see src/devtools/startup/qpe.sh for details

