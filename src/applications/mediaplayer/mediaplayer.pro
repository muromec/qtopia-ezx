qtopia_project(qtopia app)
TARGET=mediaplayer
CONFIG+=qtopia_main

HEADERS = mediaplayer.h \
            playercontrol.h \
            statewidget.h \
            elidedlabel.h \
            playerwidget.h \
            mediabrowser.h \
            visualization.h \
            playlist.h \
            browser.h \
            keyhold.h \
            menumodel.h \
            menuview.h \
            requesthandler.h \
            servicerequest.h \
            keyfilter.h

SOURCES = main.cpp \
            mediaplayer.cpp \
            playercontrol.cpp \
            statewidget.cpp \
            elidedlabel.cpp \
            playerwidget.cpp \
            mediabrowser.cpp \
            visualization.cpp \
            playlist.cpp \
            browser.cpp \
            keyhold.cpp \
            menumodel.cpp \
            menuview.cpp \
            requesthandler.cpp \
            keyfilter.cpp

!contains(QTOPIAMEDIA_ENGINES,helix) {
    DEFINES += NO_HELIX
}

desktop.files=$$QTOPIA_DEPOT_PATH/apps/Applications/mediaplayer.desktop
desktop.path=/apps/Applications
desktop.hint=desktop
INSTALLS+=desktop

pics.files=$$QTOPIA_DEPOT_PATH/pics/mediaplayer/*
pics.path=/pics/mediaplayer
pics.hint=pics
INSTALLS+=pics

help.source=$$QTOPIA_DEPOT_PATH/help
help.files=mediaplayer*
help.hint=help
INSTALLS+=help


pkg.desc=Qtopia media player.
pkg.domain=trusted

depends(libraries/qtopiamedia)
