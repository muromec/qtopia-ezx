qtopia_project(qtopia lib)
TARGET=qtopiacomm
depends(3rdparty/libraries/openobex)

PREFIX=COMM
COMM_HEADERS += qcommdevicecontroller.h \
                qcommdevicesession.h \
                qcomminterface.h \
                qcommservicemanager.h

COMM_SOURCES += qcommdevicecontroller.cpp \
                qcommdevicesession.cpp \
                qcomminterface.cpp \
                qcommservicemanager.cpp

#COMM_PRIVATE_HEADERS +=  

resolve_include()

defineTest(setupTranslatables) {
    file=$$1
    include($$file)
    VARS=HEADERS SOURCES FORMS PRIVATE_HEADERS
    for(v,VARS) {
        eval(TRANSLATABLES+=\$${$${PREFIX}_$$v})
    }
    export(TRANSLATABLES)
    export(VPATH)
}

include(obex/obex.pri)
resolve_include()
enable_bluetooth {
    depends(3rdparty/libraries/qtdbus)
    include(bluetooth/bluetooth.pri)
    resolve_include()
} else {
    setupTranslatables(bluetooth/bluetooth.pri)
}

enable_infrared {
    include(ir/ir.pri)
    resolve_include()
}
else {
    setupTranslatables(ir/ir.pri)
}

include(network/network.pri)
resolve_include()
include(serial/serial.pri)
resolve_include()
enable_vpn {
    include(vpn/vpn.pri)
    resolve_include()
} else {
    setupTranslatables(vpn/vpn.pri)
}

CONFIG += qtopia_visibility

sdk_qtopiacomm_headers.files=$${COMM_HEADERS}
sdk_qtopiacomm_headers.path=/include/qtopia/comm
sdk_qtopiacomm_headers.hint=sdk headers
INSTALLS+=sdk_qtopiacomm_headers

idep(LIBS+=-l$$TARGET)
qt_inc($$TARGET)

