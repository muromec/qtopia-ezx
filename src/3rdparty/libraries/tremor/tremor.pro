qtopia_project(external lib)
TARGET          = vorbisidec
VERSION         = 1.0.2
license(BSD GPL_COMPATIBLE)

HEADERS		= codebook.h \
                  misc.h \
                  mdct_lookup.h \
                  os.h \
                  mdct.h \
                  ivorbisfile.h \
                  lsp_lookup.h \
                  window_lookup.h \
                  floor_lookup.h \
                  codec_internal.h \
                  ogg.h \
                  asm_arm.h \
                  ivorbiscodec.h
SOURCES	        = mdct.c dsp.c info.c framing.c \
                  misc.c floor_lookup.c floor1.c floor0.c \
                  vorbisfile.c res012.c mapping0.c codebook.c \
                  framing.c bitwise.c framing2.c

depends(3rdparty/libraries/pthread)

DEFINES += REENTRANT _LOW_ACCURANCY_ LIMIT_TO_64kHz USE_MEMORY_H \
           STDC_HEADERS=1 HAVE_SYS_TYPES_H=1 HAVE_SYS_STAT_H=1 \
           HAVE_STDLIB_H=1 HAVE_STRING_H=1 HAVE_MEMORY_H=1 \
           HAVE_STRINGS_H=1 HAVE_INTTYPES_H=1 HAVE_STDINT_H=1 \
           HAVE_UNISTD_H=1 HAVE_DLFCN_H=1 HAVE_ALLOCA_H=1 \
           HAVE_ALLOCA=1 

QMAKE_CXXFLAGS += -fsigned-char

headers.files=$$HEADERS
headers.path=include/tremor
headers.hint=headers
INSTALLS+=headers

dep(INCLUDEPATH+=$$PWD)
idep(LIBS+=-l$$TARGET)
