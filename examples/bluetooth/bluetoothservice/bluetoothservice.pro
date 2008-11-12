qtopia_project(qtopia app)
TARGET=bluetoothservice
CONFIG+=qtopia_main no_quicklaunch

# Specify the languages that make lupdate should produce .ts files for
AVAILABLE_LANGUAGES=en_US
# Specify the langauges we want to install translations for
LANGUAGES=$$AVAILABLE_LANGUAGES

HEADERS=bluetoothservice.h
SOURCES=main.cpp bluetoothservice.cpp

desktop.files=bluetoothservice.desktop
desktop.path=/apps/Applications
desktop.hint=desktop
INSTALLS+=desktop

# Package information (used for make packages)
pkg.name=bluetoothservice
pkg.desc=Bluetooth service example
pkg.domain=trusted

# include the SDP record XML file
# /etc/bluetooth/sdp is where other Qtopia SDP record XML files are located
sdprecord.files=SerialPortSDPRecord.xml
sdprecord.path=/etc/bluetooth/sdp
INSTALLS+=sdprecord

# depends on the qtopia bluetooth libraries
depends(libraries/qtopiacomm)

# only build if bluetooth is enabled in Qtopia
requires(enable_bluetooth)
