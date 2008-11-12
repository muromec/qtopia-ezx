qtopia_project(qtopia plugin)
TARGET=lan

depends(libraries/qtopiacomm/network)

# Packaged by settings/network
CONFIG+=no_pkg

FORMS       = wirelessbase.ui \
              wirelessencryptbase.ui \
              roamingbase.ui

HEADERS	    = lanplugin.h \
              lan.h \
              config.h \
              wirelessconfig.h \
              encryptionconfig.h \
              wirelessscan.h \
              roamingconfig.h \
              roamingmonitor.h \
              wnet.h\
              wlanregistrationprovider.h
                
SOURCES	    = lanplugin.cpp \
              lan.cpp \
              config.cpp \
              wirelessconfig.cpp \
              encryptionconfig.cpp \
              wirelessscan.cpp \
              roamingconfig.cpp \
              roamingmonitor.cpp \
              wnet.cpp\
              wlanregistrationprovider.cpp

conf.files	= $$QTOPIA_DEPOT_PATH/etc/network/lan.conf \
                  $$QTOPIA_DEPOT_PATH/etc/network/lan-pcmcia.conf \
                  $$QTOPIA_DEPOT_PATH/etc/network/wlan-pcmcia.conf \
                  $$QTOPIA_DEPOT_PATH/etc/network/wlan.conf 
                  
conf.path	= /etc/network
INSTALLS += conf

script=$$PWD/lan-network
variable=$$LAN_NETWORK_SCRIPT
include(../network_script.pri)

pics.files	= $$QTOPIA_DEPOT_PATH/pics/Network/lan/* 
pics.path	= /pics/Network/lan
pics.hint=pics
INSTALLS+=pics

icons.files     = $$QTOPIA_DEPOT_PATH/pics/Network/icons/*
icons.path      = /pics/Network/icons
icons.hint=pics
INSTALLS+=icons

