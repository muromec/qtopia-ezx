qtopia_project(qtopia app)
TARGET=helpbrowser
CONFIG+=qtopia_main

HEADERS		= helpbrowser.h \
		    helppreprocessor.h \
		navigationbar_p.h
SOURCES		= helpbrowser.cpp main.cpp \
		    helppreprocessor.cpp \
		navigationbar_p.cpp

# See $QTOPIA_SOURCE_TREE/src/server/server.pro for the definition of extra html help files
#    not associted with a specific application. Each application is responsible 
#    for installing its own help
help.source=$$QTOPIA_DEPOT_PATH/help
help.files=helpbrowser*
help.hint=help
INSTALLS+=help
desktop.files=$$QTOPIA_DEPOT_PATH/apps/Applications/helpbrowser.desktop
desktop.path=/apps/Applications
desktop.hint=desktop
INSTALLS+=desktop
pics.files=$$QTOPIA_DEPOT_PATH/pics/helpbrowser/*
pics.path=/pics/helpbrowser
pics.hint=pics
INSTALLS+=pics
pics2.files=$$QTOPIA_DEPOT_PATH/pics/help/*
pics2.path=/pics/help
pics2.hint=pics
INSTALLS+=pics2
helpservice.files=$$QTOPIA_DEPOT_PATH/services/Help/helpbrowser
helpservice.path=/services/Help
INSTALLS+=helpservice

pkg.desc=Help browser for Qtopia.
pkg.domain=trusted
