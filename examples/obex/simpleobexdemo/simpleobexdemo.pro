qtopia_project(qtopia app)
TARGET=simpleobexdemo
CONFIG+=qtopia_main no_quicklaunch

# Specify the languages that make lupdate should produce .ts files for
AVAILABLE_LANGUAGES=en_US
# Specify the langauges we want to install translations for
LANGUAGES=$$AVAILABLE_LANGUAGES

HEADERS=obexclientwindow.h \
        obexquoteserver.h
SOURCES=main.cpp \
        obexclientwindow.cpp \
        obexquoteserver.cpp

desktop.files=$$QTOPIA_DEPOT_PATH/examples/obex/simpleobexdemo/simpleobexdemo.desktop
desktop.path=/apps/Applications
desktop.hint=desktop
INSTALLS+=desktop

# Package information (used for make packages)
pkg.name=simpleobexdemo
pkg.desc=Simple OBEX example
pkg.domain=trusted

# depends on the qtopia obex libraries
depends(libraries/qtopiacomm/obex)
