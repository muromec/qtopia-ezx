qtopia_project(qtopia app)
TARGET=query
CONFIG+=qtopia_main no_quicklaunch

# Specify the languages that make lupdate should produce .ts files for
AVAILABLE_LANGUAGES=en_US
# Specify the langauges we want to install translations for
LANGUAGES=$$AVAILABLE_LANGUAGES

HEADERS=query.h
SOURCES=query.cpp main.cpp

desktop.files=query.desktop
desktop.path=/apps/Applications
desktop.hint=desktop
INSTALLS+=desktop

# Package information (used for make packages)
pkg.name=query
pkg.desc=Bluetooth SDP query example
pkg.domain=trusted

# depends on the qtopia bluetooth libraries
depends(libraries/qtopiacomm)

# only build if bluetooth is enabled in Qtopia
requires(enable_bluetooth)
