qtopia_project(qtopia app)
TARGET=scanner
CONFIG+=qtopia_main no_quicklaunch

# Specify the languages that make lupdate should produce .ts files for
AVAILABLE_LANGUAGES=en_US
# Specify the langauges we want to install translations for
LANGUAGES=$$AVAILABLE_LANGUAGES

HEADERS=scanner.h
SOURCES=scanner.cpp main.cpp

desktop.files=$$QTOPIA_DEPOT_PATH/examples/bluetooth/scanner/scanner.desktop
desktop.path=/apps/Applications
desktop.hint=desktop
INSTALLS+=desktop

# Package information (used for make packages)
pkg.name=scanner
pkg.desc=Bluetooth device discovery example
pkg.domain=trusted

# depends on the qtopia bluetooth libraries
depends(libraries/qtopiacomm)

# only build if bluetooth is enabled in Qtopia
requires(enable_bluetooth)
