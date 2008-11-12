qtopia_project(qtopia lib)
TARGET=qmstroke
CONFIG += qtopia_visibility

HEADERS += char.h \
           combining_p.h \
           profile.h \
           signature.h \
           stroke.h

SOURCES += char.cpp \
           combining.cpp \
           profile.cpp \
           signature.cpp \
           stroke.cpp

help.source=$$QTOPIA_DEPOT_PATH/help
help.files=handwriting*.html
help.hint=help
INSTALLS+=help

sdk_hw_headers.files=$${HEADERS}
sdk_hw_headers.path=/include/qtopia/mstroke
sdk_hw_headers.hint=sdk headers
INSTALLS+=sdk_hw_headers

pkg.desc=Multi-stroke gesture recognition library for Qtopia.
pkg.domain=trusted

idep(LIBS+=-l$$TARGET)
qt_inc($$TARGET)
