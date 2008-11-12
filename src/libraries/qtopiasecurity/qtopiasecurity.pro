qtopia_project(qtopia core lib)
TARGET=qtopiasecurity
VERSION=4.0.0
CONFIG+=qtopia_visibility no_tr

QTOPIASECURITY_HEADERS = \
    qsxepolicy.h \
    keyfiler_p.h \
    qpackageregistry.h

QTOPIASECURITY_SOURCES = \
    qsxepolicy.cpp \
    keyfiler.cpp \
    qpackageregistry.cpp

PREFIX=QTOPIASECURITY
resolve_include()

headers.files=$${QTOPIASECURITY_HEADERS}
headers.path=/include/qtopia/security
headers.hint=headers
INSTALLS+=headers

#private_headers.files=$${QTOPIASECURITY_PRIVATE_HEADERS}
#private_headers.path=/include/qtopia/security/private
#private_headers.hint=headers
#INSTALLS+=private_headers

idep(LIBS+=-l$$TARGET)
qt_inc($$TARGET)
