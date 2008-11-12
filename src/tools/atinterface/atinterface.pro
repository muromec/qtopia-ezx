qtopia_project(qtopia core app)
TARGET=atinterface
CONFIG+=no_tr singleexec_main

!platform:enable_cell:DEFINES+=ATINTERFACE_SMS

HEADERS	= \
    atcustom.h \
    atcallmanager.h \
    atparseutils.h \
    atcommands.h \
    atgsmnoncellcommands.h \
    atv250commands.h \
    atfrontend.h \
    atindicators.h \
    atinterface.h \
    atoptions.h \
    atsessionmanager.h \
    modememulatorservice.h

SOURCES	= \
    main.cpp \
    atcallmanager.cpp \
    atparseutils.cpp \
    atcommands.cpp \
    atgsmnoncellcommands.cpp \
    atv250commands.cpp \
    atfrontend.cpp \
    atindicators.cpp \
    atinterface.cpp \
    atoptions.cpp \
    atsessionmanager.cpp \
    modememulatorservice.cpp

enable_bluetooth {
    HEADERS	+= \
        atbluetoothcommands.h
    SOURCES	+= \
        atbluetoothcommands.cpp
}

enable_cell {
    HEADERS	+= \
        atgsmcellcommands.h
    SOURCES	+= \
        atgsmcellcommands.cpp

    !platform {
        HEADERS	+= \
            atsmscommands.h
        SOURCES	+= \
            atsmscommands.cpp

        depends(libraries/qtopiamail)
    }
}

depends(libraries/qtopiaphone)
depends(libraries/qtopiapim)
depends(libraries/qtopiacomm/serial)

pkg.desc=AT interface for remote access to the device over a cable
pkg.domain=trusted

modememulservice.files=$$QTOPIA_DEPOT_PATH/services/ModemEmulator/atinterface
modememulservice.path=/services/ModemEmulator
INSTALLS+=modememulservice
