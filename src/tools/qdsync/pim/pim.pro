qtopia_project(qtopia plugin)
TARGET=pim
CONFIG+=no_tr
# Packaged by tools/qdsync/app
CONFIG+=no_pkg
plugin_type=qdsync

DEFINES+=PIMXML_NAMESPACE=QDSync

VPATH+=..
INCLUDEPATH+=..

HEADERS+=\
    qpimsyncstorage.h\
    qpimxml_p.h\

SOURCES+=\
    qpimsyncstorage.cpp\
    qpimxml.cpp\

depends(libraries/qtopiapim)
depends(tools/qdsync/common)

