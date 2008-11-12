qtopia_project(qtopia plugin)

TARGET=cruxus
CONFIG+=no_tr

HEADERS	= \
        contentdevice.h \
        cruxusengine.h \
        cruxusenginefactory.h \
        cruxusurisessionbuilder.h \
        cruxussimplesession.h \
        cruxusurihandlers.h \
        cruxusoutputdevices.h \
        cruxusoutputthread.h

SOURCES	= \
        contentdevice.cpp \
        cruxusengine.cpp \
        cruxusenginefactory.cpp \
        cruxusurisessionbuilder.cpp \
        cruxussimplesession.cpp \
        cruxusurihandlers.cpp \
        cruxusoutputdevices.cpp \
        cruxusoutputthread.cpp

equals(QTOPIA_SOUND_SYSTEM,oss) {
    DEFINES+=HAVE_OSS
}

depends(libraries/qtopiamedia)
depends(libraries/qtopiaaudio)

