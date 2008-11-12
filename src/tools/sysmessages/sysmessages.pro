qtopia_project(qtopia app)
TARGET=sysmessages
CONFIG+=no_quicklaunch

# Input
HEADERS += sysmessages.h 

SOURCES += main.cpp sysmessages.cpp

contains(PROJECTS,libraries/qtopiamail) {
    DEFINES+=MAIL_EXISTS
}

sysmsgservice.files=$$QTOPIA_DEPOT_PATH/services/SystemMessages/sysmessages
sysmsgservice.path=/services/SystemMessages

INSTALLS+=sysmsgservice

# SXE permissions required
pkg.domain=trusted

