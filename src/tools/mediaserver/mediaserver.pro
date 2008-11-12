qtopia_project(qtopia app)

TARGET=mediaserver
CONFIG+=singleexec_main

# Give us a direct connection to the document system
DEFINES+=QTOPIA_DIRECT_DOCUMENT_SYSTEM_CONNECTION

DEFINES+="CONFIGURED_ENGINES=$$LITERAL_QUOTE$$LITERAL_ESCAPED_QUOTE$$QTOPIAMEDIA_ENGINES$$LITERAL_ESCAPED_QUOTE$$LITERAL_QUOTE"

HEADERS =   \
            sessionmanager.h \
            engineloader.h \
            buildermanager.h \
            buildernegotiator.h \
            urinegotiator.h \
            mediaagent.h \
            qsoundprovider.h \
            qtopiamediaprovider.h \
            mediacontrolserver.h \
            mediacontentserver.h \
            domainmanager.h \
            sessionmanagersession.h \
            mediaagentsession.h \
            drmsession.h \
            mediapowercontrol.h\
            mediavolumecontrol.h

SOURCES =   \
            main.cpp \
            sessionmanager.cpp \
            engineloader.cpp \
            buildermanager.cpp \
            buildernegotiator.cpp \
            urinegotiator.cpp \
            mediaagent.cpp \
            qsoundprovider.cpp \
            qtopiamediaprovider.cpp \
            mediacontrolserver.cpp \
            mediacontentserver.cpp \
            domainmanager.cpp \
            sessionmanagersession.cpp \
            mediaagentsession.cpp \
            drmsession.cpp \
            mediapowercontrol.cpp\
            mediavolumecontrol.cpp
 
contains(DEFINES,QTOPIA_TELEPHONY) {
    HEADERS += callmonitor.h
    SOURCES += callmonitor.cpp

    depends(libraries/qtopiaphone)
}

pkg.desc = Media Server
pkg.domain = trusted 

mediaserverservice.files = $$QTOPIA_DEPOT_PATH/services/MediaServer/mediaserver
mediaserverservice.path = /services/MediaServer
INSTALLS += mediaserverservice

domainmanager.files = $$QTOPIA_DEPOT_PATH/etc/default/Trolltech/AudioDomains.conf
domainmanager.path = /etc/default/Trolltech
INSTALLS += domainmanager

depends(libraries/qtopiamedia)
depends(libraries/qtopiaaudio)
