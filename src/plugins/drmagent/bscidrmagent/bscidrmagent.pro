qtopia_project(qtopia plugin)
TARGET=bscidrmagent

HEADERS		=  bscidrmcontentplugin.h \
                   bscidrm.h \
                   bscirightsmanager.h \
                   bscidrmagentservice.h \
                   bscidrmagentplugin.h \
                   bscifileengine.h \
                   bsciprompts.h

SOURCES	        =  bscidrmcontentplugin.cpp \
                   bscidrm.cpp \
                   bscirightsmanager.cpp \
                   bscidrmagentservice.cpp \
                   bscidrmagentplugin.cpp \
                   bscifileengine.cpp \
                   bsciprompts.cpp

pki.files = $$QTOPIA_DEPOT_PATH/etc/bscidrm/*
pki.path = /etc/bscidrm/

INSTALLS += pki

depends(libraries/qtopia)
depends(3rdparty/libraries/drmagent)
