qtopia_project(qtopia plugin)
TARGET=madplugin
CONFIG+=no_tr
license(FREEWARE)

HEADERS = \
        mp3plugin.h \
        mp3decoder.h

SOURCES = \
        mp3plugin.cpp \
        mp3decoder.cpp


DEFINES += OPT_SPEED OPT_SSO PIC

contains(arch,i386):DEFINES += FPM_INTEL
else:contains(arch,arm):DEFINES += FPM_ARM
else:contains(arch,mips):DEFINES += FPM_MIPS
else:contains(arch,powerpc):DEFINES += FPM_PPC
else:DEFINES += FPM_64BIT

LIBS+=-lmad
depends(libraries/qtopiamedia)
