qtopia_project(qtopia app) # see buildsystem.html for more project keywords
TARGET=messageviewer
# main.cpp uses the QTOPIA_ADD_APPLICATION/QTOPIA_MAIN macros
# It can also build without these macros if you disable this and change the define in main.cpp
CONFIG+=qtopia_main
# Despite using the QTOPIA_MAIN macro, do not build this app as a
# quicklaunch plugin unless -force-quicklaunch was passed to configure
CONFIG+=no_quicklaunch
# Do not build this app into a singleexec binary
CONFIG+=no_singleexec
# Disable i18n support
CONFIG+=no_tr

# Messaging library is not available in platform edition
requires(!platform)

# We use the PIM and messaging libraries
depends(libraries/qtopiamail)
depends(libraries/qtopiapim)

# These are the source files that get built to create the application
FORMS=messageviewerbase.ui
HEADERS=messageviewer.h messagemodel.h messagedelegate.h
SOURCES=main.cpp messageviewer.cpp messagemodel.cpp messagedelegate.cpp

# Install the launcher item. The metadata comes from the .desktop file
# and the path here.
desktop.files=messageviewer.desktop
desktop.path=/apps/Applications
desktop.trtarget=messageviewer-nct
desktop.hint=nct desktop
INSTALLS+=desktop

# Install some pictures.
pics.files=pics/*
pics.path=/pics/messageviewer
pics.hint=pics
INSTALLS+=pics

# Install help files
help.source=help
help.files=messageviewer.html
help.hint=help
INSTALLS+=help

# Package information (used for make packages)
pkg.name=messageviewer
pkg.desc=Message Viewer Application
pkg.version=1.0.0-1
pkg.maintainer=Trolltech (www.trolltech.com)
pkg.license=Commercial
pkg.domain=trusted
