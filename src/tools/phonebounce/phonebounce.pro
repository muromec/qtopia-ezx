qtopia_project(external app)
TARGET=phonebounce
CONFIG-=enable_singleexec

SOURCES		= \
    phonebounce.cpp

pkg.desc=Phone bounce daemon

target.hint+=sxe
target.domain=trusted
