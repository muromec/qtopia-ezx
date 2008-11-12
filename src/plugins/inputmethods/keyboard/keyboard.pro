qtopia_project(qtopia plugin)
TARGET=qkeyboard
CONFIG+=no_singleexec

HEADERS		= keyboard.h \
		    pickboardcfg.h \
		    pickboardpicks.h \
		    keyboardimpl.h\
                    keyboardframe.h
SOURCES		= keyboard.cpp \
		    pickboardcfg.cpp \
		    pickboardpicks.cpp \
		    keyboardimpl.cpp\
                    keyboardframe.cpp

pics.files=$${QTOPIA_DEPOT_PATH}/pics/keyboard/*
pics.path=/pics/keyboard
pics.hint=pics
INSTALLS += pics
 
pkg.name=qpe-keyboard
pkg.domain=trusted
