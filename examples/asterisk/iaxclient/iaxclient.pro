qtopia_project(external lib)
license(LGPL)
TARGET		=   iaxclient
VERSION		=   1.0.0
CONFIG         +=   warn_off
requires(enable_voip)

INCLUDEPATH+=$$QTOPIA_DEPOT_PATH/src/3rdparty/libraries/gsm
INCLUDEPATH+=$$QTOPIA_DEPOT_PATH/examples/asterisk/speex/include

DESTDIR = $$QPEDIR/lib

HEADERS		= audio_encode.h \
                  audio_file.h \
                  codec_alaw.h \
                  codec_gsm.h \
                  codec_speex.h \
                  codec_ulaw.h \
                  frame.h \
                  iax2.h \
                  iax2-parser.h \
                  iax-client.h \
                  iaxclient.h \
                  iaxclient_lib.h \
                  iax.h \
                  jitterbuf.h \
                  md5.h \
                  plc.h
SOURCES		= audio_encode.c \
                  audio_file.c \
                  codec_alaw.c \
                  codec_gsm.c \
                  codec_speex.c \
                  codec_ulaw.c \
                  iax2-parser.c \
                  iax.c \
                  iaxclient_lib.c \
                  jitterbuf.c \
                  md5.c \
                  plc.c \
                  unixfuncs.c
DEFINES         += LIBIAX

depends(3rdparty/libraries/gsm)
depends(asterisk/speex)

pkg.desc=IAX client library
pkg.domain=trusted

# FIXME "make syncqtopia"
dep(INCLUDEPATH+=$$PWD)
idep(LIBS+=-l$$TARGET)
