qtopia_project(qtopia plugin)
TARGET=base
CONFIG+=no_tr
# Packaged by tools/qdsync/app
CONFIG+=no_pkg
plugin_type=qdsync

VPATH+=..
INCLUDEPATH+=..

HEADERS+=\
    qdsync.h\
    qcopbridge.h\
    syncauthentication.h\
    log.h\
    qdglobal.h\

SOURCES+=\
    qdsync.cpp\
    qcopbridge.cpp\
    syncauthentication.cpp\

equals(QTE_MINOR_VERSION,2):include(qpe_version.pri)

depends(3rdparty/libraries/crypt)
depends(tools/qdsync/common)

