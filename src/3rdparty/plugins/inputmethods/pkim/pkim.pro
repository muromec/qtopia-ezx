qtopia_project(qtopia plugin)
TARGET       = pkim
VERSION      = 4.1.0
license(FREEWARE)

depends(3rdparty/libraries/inputmatch)
# if commercial only?
depends(libraries/handwriting)
VPATH+=$$QTOPIA_DEPOT_PATH/src/3rdparty/libraries/inputmatch

HEADERS      = pkim.h \
               pkimpl.h \
	       modepicker.h \
               charmatch.h \
	       charlist.h
SOURCES      = pkimpl.cpp \
               pkim.cpp \
	       modepicker.cpp \
               charmatch.cpp \
	       charlist.cpp

im.files=$$QTOPIA_DEPOT_PATH/etc/im/pkim/*
im.path=/etc/im/pkim
INSTALLS+=im
pics.files=$$QTOPIA_DEPOT_PATH/pics/pkim/*
pics.path=/pics/pkim
INSTALLS+=pics
dictinternet.files=$$QTOPIA_DEPOT_PATH/etc/dict/internet
dictinternet.path=/etc/dict
dictinternet.hint=dawg
INSTALLS+=dictinternet
fsim.files = $$QTOPIA_DEPOT_PATH/etc/qimpen/fstext.qpt\
	$$QTOPIA_DEPOT_PATH/etc/qimpen/fsnum.qpt\
	$$QTOPIA_DEPOT_PATH/etc/qimpen/fscombining.qpt\
	$$QTOPIA_DEPOT_PATH/etc/qimpen/fs.conf
fsim.path=/etc/qimpen/
INSTALLS+=fsim
fspics.files=$$QTOPIA_DEPOT_PATH/pics/fshw/*.png
fspics.path=/pics/fshw
fspics.hint=pics
INSTALLS+=fspics

pkg.domain=trusted


