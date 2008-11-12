qtopia_project(qtopia app)
TARGET=photoedit
CONFIG+=qtopia_main

HEADERS    = photoeditui.h \
                imageviewer.h \
                thumbnailmodel.h \
                editor/imageui.h \
                editor/slider.h \
                editor/navigator.h \
                editor/regionselector.h \
                editor/matrix.h \
                editor/imageprocessor.h \
                editor/imageio.h \
                slideshow/slideshowui.h \
                slideshow/slideshowdialog.h \
                slideshow/slideshow.h

SOURCES    = main.cpp \
                photoeditui.cpp \
                imageviewer.cpp \
                thumbnailmodel.cpp \
                editor/imageui.cpp \
                editor/slider.cpp \
                editor/navigator.cpp \
                editor/regionselector.cpp \
                editor/imageprocessor.cpp \
                editor/imageio.cpp \
                slideshow/slideshowui.cpp \
                slideshow/slideshowdialog.cpp \
                slideshow/slideshow.cpp

help.source=$$QTOPIA_DEPOT_PATH/help
help.files=photoedit*
help.hint=help
INSTALLS+=help
desktop.files=$$QTOPIA_DEPOT_PATH/apps/Applications/photoedit.desktop
desktop.path=/apps/Applications
desktop.hint=desktop
INSTALLS+=desktop
pics.files=$$QTOPIA_DEPOT_PATH/pics/photoedit/*
pics.path=/pics/photoedit
pics.hint=pics
INSTALLS+=pics
service.files=$$QTOPIA_DEPOT_PATH/services/PhotoEdit/photoedit
service.path=/services/PhotoEdit
INSTALLS+=service
qdsservice.files=$$QTOPIA_DEPOT_PATH/etc/qds/PhotoEdit
qdsservice.path=/etc/qds
INSTALLS+=qdsservice

pkg.desc=An image manager for Qtopia.
pkg.domain=trusted
