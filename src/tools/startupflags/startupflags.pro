qtopia_project(qtopia app)

TARGET=startupflags
CONFIG+=qtopia_main

HEADERS		= startupflags.h
SOURCES		= startupflags.cpp main.cpp

TRANSLATABLES += startupflags.cpp

desktop.files=$$QTOPIA_DEPOT_PATH/apps/Settings/startupflags.desktop
desktop.path=/apps/Settings
desktop.hint=desktop
INSTALLS+=desktop

# This is documented in src/build/doc/src/deviceprofiles.qdoc
!isEmpty(DEVICE_CONFIG_PATH):exists($$DEVICE_CONFIG_PATH/etc/default/Trolltech/StartupFlags.conf) {
    conf.files=$$DEVICE_CONFIG_PATH/etc/default/Trolltech/StartupFlags.conf
} else {
    conf.files=$$QTOPIA_DEPOT_PATH/etc/default/Trolltech/StartupFlags.conf
}
conf.path=/etc/default/Trolltech
INSTALLS+=conf

script.files=$$QTOPIA_DEPOT_PATH/src/tools/startupflags/startupflags.sh
script.path=/bin
script.hint=script
INSTALLS+=script

pkg.desc=Startup flags editor for Qtopia.
pkg.domain=trusted

