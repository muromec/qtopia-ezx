qtopia_project(qtopia lib)
TARGET=qtopiasmil
CONFIG+=no_tr

HEADERS		= smil.h \
		    system.h \
		    transfer.h \
		    module.h \
		    element.h \
		    structure.h \
		    content.h \
		    layout.h \
		    timing.h \
		    media.h
SOURCES		= smil.cpp \
		    system.cpp \
		    transfer.cpp \
		    module.cpp \
		    element.cpp \
		    structure.cpp \
		    content.cpp \
		    layout.cpp \
		    timing.cpp \
		    media.cpp

sdk_qtopiasmil_headers.files=$${HEADERS}
sdk_qtopiasmil_headers.path=/include/qtopia/smil
sdk_qtopiasmil_headers.hint=sdk headers
INSTALLS+=sdk_qtopiasmil_headers

idep(LIBS+=-l$$TARGET)
