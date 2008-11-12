# -*-mode:sh-*-
# Qt image handling

# Qt kernel module

HEADERS += \
        image/qbitmap.h \
        image/qicon.h \
        image/qimage.h \
        image/qimage_p.h \
        image/qimageiohandler.h \
        image/qimagereader.h \
        image/qimagewriter.h \
        image/qpaintengine_pic_p.h \
        image/qpicture.h \
        image/qpicture_p.h \
        image/qpictureformatplugin.h \
        image/qpixmap.h \
        image/qpixmap_p.h \
        image/qpixmapcache.h \
        image/qiconengine.h \
        image/qiconengineplugin.h \
        image/qmovie.h

SOURCES += \
        image/qbitmap.cpp \
        image/qicon.cpp \
        image/qimage.cpp \
        image/qimageiohandler.cpp \
        image/qimagereader.cpp \
        image/qimagewriter.cpp \
        image/qpaintengine_pic.cpp \
        image/qpicture.cpp \
        image/qpictureformatplugin.cpp \
        image/qpixmapcache.cpp \
        image/qiconengine.cpp \
        image/qiconengineplugin.cpp \
        image/qmovie.cpp

!embedded:!win32:SOURCES += image/qpixmap.cpp
win32 {
    SOURCES += \
            image/qpixmap_win.cpp \
            image/qpixmap_raster.cpp
}
unix:x11:SOURCES += image/qpixmap_x11.cpp
!embedded:!x11:mac:SOURCES += image/qpixmap_mac.cpp
embedded:SOURCES += image/qpixmap_qws.cpp image/qpixmap_raster.cpp

# Built-in image format support
HEADERS += \
        image/qbmphandler_p.h \
        image/qppmhandler_p.h \
        image/qxbmhandler_p.h \
        image/qxpmhandler_p.h

SOURCES += \
        image/qbmphandler.cpp \
        image/qppmhandler.cpp \
        image/qxbmhandler.cpp \
        image/qxpmhandler.cpp

# 3rd party / system PNG support
!contains(QT_CONFIG, no-png) {
    HEADERS += image/qpnghandler_p.h
    SOURCES += image/qpnghandler.cpp

    contains(QT_CONFIG, system-png) {
        unix:LIBS  += -lpng
        win32:LIBS += libpng.lib
    } else {
        INCLUDEPATH  += ../3rdparty/libpng ../3rdparty/zlib
        SOURCES += ../3rdparty/libpng/png.c \
          ../3rdparty/libpng/pngerror.c \
          ../3rdparty/libpng/pngget.c \
          ../3rdparty/libpng/pngmem.c \
          ../3rdparty/libpng/pngpread.c \
          ../3rdparty/libpng/pngread.c \
          ../3rdparty/libpng/pngrio.c \
          ../3rdparty/libpng/pngrtran.c \
          ../3rdparty/libpng/pngrutil.c \
          ../3rdparty/libpng/pngset.c \
          ../3rdparty/libpng/pngtrans.c \
          ../3rdparty/libpng/pngwio.c \
          ../3rdparty/libpng/pngwrite.c \
          ../3rdparty/libpng/pngwtran.c \
          ../3rdparty/libpng/pngwutil.c \
          ../3rdparty/libpng/pnggccrd.c
    }
} else {
    DEFINES *= QT_NO_IMAGEFORMAT_PNG
}
