qtopia_project(external app)
TARGET=apm.bin
CONFIG-=enable_singleexec

SOURCES=apm.c

script.files=$$QTOPIA_DEPOT_PATH/devices/greenphone/src/devtools/apm/apm
script.path=/bin
script.hint=script

pkg.domain=trusted

INSTALLS+=script
