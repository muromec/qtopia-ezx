qtopia_project(external app)
license(LGPL)
TARGET=atd

SOURCES = atd.cpp greenphone.cpp
HEADERS = greenphone.h

at.files=$$QTOPIA_DEPOT_PATH/src/3rdparty/tools/atd/at
at.path=/bin
at.hint=script
INSTALLS+=at

pkg.desc=atd daemon for Sharp
pkg.domain=trusted
