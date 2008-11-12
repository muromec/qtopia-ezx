
For details on creating a Device Configuration Profile see:
    http://doc.trolltech.com/qtopia/tut-deviceexample.html.


Device Configuration Profile Status
===================================

-Current status: supported

-gcc details:
Using built-in specs.
Target: arm-angstrom-linux-gnueabi
Configured with: /home/toolchain/OE/tmp/work/armv5te-angstrom-linux-gnueabi/gcc-cross-4.1.1-r11/gcc-4.1.1/configure --build=i686-linux --host=i686-linux --target=arm-angstrom-linux-gnueabi --prefix=/opt/toolchains/armv5te-eabi --exec_prefix=/opt/toolchains/armv5te-eabi --bindir=/opt/toolchains/armv5te-eabi/bin --sbindir=/opt/toolchains/armv5te-eabi/bin --libexecdir=/opt/toolchains/armv5te-eabi/libexec --datadir=/opt/toolchains/armv5te-eabi/share --sysconfdir=/opt/toolchains/armv5te-eabi/etc --sharedstatedir=/opt/toolchains/armv5te-eabi/com --localstatedir=/opt/toolchains/armv5te-eabi/var --libdir=/opt/toolchains/armv5te-eabi/lib --includedir=/opt/toolchains/armv5te-eabi/include --oldincludedir=/opt/toolchains/armv5te-eabi/include --infodir=/opt/toolchains/armv5te-eabi/share/info --mandir=/opt/toolchains/armv5te-eabi/share/man --with-gnu-ld --enable-shared --enable-target-optspace --enable-languages=c,c++ --enable-threads=posix --enable-multilib --enable-c99 --enable-long-long --enable-symvers=gnu --enable-libstdcxx-pch --program-prefix=arm-angstrom-linux-gnueabi- --with-local-prefix=/opt/toolchains/armv5te-eabi/arm-angstrom-linux-gnueabi --with-gxx-include-dir=/opt/toolchains/armv5te-eabi/arm-angstrom-linux-gnueabi/include/c++ --disable-multilib --enable-__cxa_atexit --with-float=soft --disable-libssp --with-mpfr=/home/toolchain/OE/tmp/staging/i686-linux
Thread model: posix
gcc version 4.1.1

This toolchain is built using open embedded build system for the angstrom distribution, tested on the Zaurus C3200 model.


-This profile is used as a testing device (internally). EABI support, 640x480 screen, removable media issues, usb host functionality with Qtopia.

-See src/devtools/startup/qpe.sh for startup details


Requirements
============

* Device model               : Zaurus C3200
* Device manufacturer        : Sharp
* Supported hardware options : touchscreen and tty0 keyboard
* Toolchain version          : gcc 4.1.1
* Toolchain download location: Use OE to create toolchain, angstrom-image
* Kernel version             : 2.6.x
* Kernel flags               : NA
* glibc version              : 2.5
* floating point support used: EABI


Configuration for this device
=============================
* see src/devtools/startup/qpe.sh for details

* see DESCRIPTION file for more details on functionality, example implementations
