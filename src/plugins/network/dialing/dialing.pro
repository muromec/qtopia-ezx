qtopia_project(qtopia plugin)
TARGET=dialing

# Packaged by settings/network
CONFIG+=no_pkg

FORMS       =   dialingbase.ui \
                advancedbase.ui
                
HEADERS	    =   dialupplugin.h \
                dialup.h \
                config.h \
                dialing.h \
                advanced.h \
                dialstring.h 

                
SOURCES	    =   dialupplugin.cpp \
                dialup.cpp \
                config.cpp \
                dialing.cpp \
                advanced.cpp \
                dialstring.cpp

depends(libraries/qtopiacomm/network)

phone {
    depends(libraries/qtopiaphone)
    conf.files = $$QTOPIA_DEPOT_PATH/etc/network/dialupGPRS.conf
}
                
conf.files	+= $$QTOPIA_DEPOT_PATH/etc/network/dialup.conf \
                  $$QTOPIA_DEPOT_PATH/etc/network/dialupIR.conf
conf.path	= /etc/network
INSTALLS+=conf

script=$$PWD/ppp-network
variable=$$DIALING_NETWORK_SCRIPT
include(../network_script.pri)

bin2.files=$$QTOPIA_DEPOT_PATH/bin/qtopia-pppd-internal
bin2.path=/bin
bin2.hint=script
INSTALLS+=bin2

pics.files	= $$QTOPIA_DEPOT_PATH/pics/Network/dialup/*  
pics.path	= /pics/Network/dialup
pics.hint=pics
INSTALLS+=pics

icons.files     = $$QTOPIA_DEPOT_PATH/pics/Network/icons/*
icons.path      = /pics/Network/icons
icons.hint=pics
INSTALLS+=icons

