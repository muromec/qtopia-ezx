CONFIG+=no_qtopiabase
qtopia_project(qtopia core lib)
TARGET=qtopiabase
CONFIG += qtopia_visibility

QTOPIABASE_HEADERS+=\
    qtopiaglobal.h \
    qtopiasxe.h \
    custom.h \
    qlog.h \
    qtopialog.h \
    qtopialog-config.h \
    qsignalintercepter.h \
    qslotinvoker.h \
    qstorage.h \
    qtopianamespace.h \
    qtopiaservices.h \
    qtopiaabstractservice.h \
    qtopiachannel.h \
    qtopiaipcadaptor.h \
    qtopiaipcenvelope.h \
    qtopiaipcmarshal.h \
    qabstractipcinterfacegroup.h \
    qabstractipcinterfacegroupmanager.h \
    qabstractipcinterface.h \
    qsoundcontrol.h \
    quniqueid.h \
    qtranslatablesettings.h \
    qtopiatimer.h \
    qdawg.h \
    version.h \
    qglobalpixmapcache.h

QTOPIABASE_PRIVATE_HEADERS+=\
    qactionconfirm_p.h\
    qcopenvelope_p.h \
    qmemoryfile_p.h

QTOPIABASE_SOURCES+=\
    qtopiasxe.cpp \
    qactionconfirm.cpp\
    qabstractipcinterface.cpp \
    qabstractipcinterfacegroup.cpp \
    qabstractipcinterfacegroupmanager.cpp \
    qcopenvelope.cpp \
    qlog.cpp \
    qsignalintercepter.cpp \
    qslotinvoker.cpp \
    qstorage.cpp \
    qtopianamespace.cpp\
    qtopiaservices.cpp \
    qtopiaabstractservice.cpp \
    qtopiachannel.cpp \
    qtopiaipcadaptor.cpp \
    qtopiaipcenvelope.cpp \
    qtopiaipcmarshal.cpp \
    qtopialog.cpp \
    qsoundcontrol.cpp \
    quniqueid.cpp \
    qtranslatablesettings.cpp \
    qtopiatimer.cpp \
    qdawg.cpp \
    qmemoryfile.cpp \
    qmemoryfile_unix.cpp

enable_sxe:QTOPIABASE_SOURCES+=qtopiasxe.cpp

VPATH+=$$QTOPIA_DEPOT_PATH/src/libraries/qtopiail
INCLUDEPATH+=$$QTOPIA_DEPOT_PATH/src/3rdparty/libraries/qtopiail
depends(3rdparty/libraries/dlmalloc)

QTOPIABASE_SOURCES+=\
    framework/applayer.cpp \
    framework/inilayer.cpp \
    framework/qfilemonitor.cpp \
    framework/qmallocpool.cpp \
    framework/qpacketprotocol.cpp \
    framework/qsystemlock.cpp \
    framework/qvaluespace.cpp \
    framework/qexpressionevaluator.cpp \
    framework/qfixedpointnumber.cpp

QTOPIABASE_HEADERS+=\
    framework/qfilemonitor.h \
    framework/qmallocpool.h \
    framework/qpacketprotocol.h \
    framework/qsystemlock.h \
    framework/qvaluespace.h \
    framework/qexpressionevaluator.h \
    framework/qtopiailglobal.h

QTOPIABASE_PRIVATE_HEADERS+=\
    framework/qfixedpointnumber_p.h

x11 {
    VPATH+=$$QT_DEPOT_PATH/src/gui/embedded
    INCLUDEPATH+=$$QT_DEPOT_PATH/src/gui/embedded

    QTOPIABASE_HEADERS+=\
        qcopchannel_x11.h

    QTOPIABASE_PRIVATE_HEADERS+=\
        qcopchannel_x11_p.h \
        qunixsocket_p.h \
        qunixsocketserver_p.h

    QTOPIABASE_SOURCES+=\
        qcopchannel_x11.cpp \
        qunixsocket.cpp \
        qunixsocketserver.cpp \
        qglobalpixmapcache_x11.cpp

    enable_sxe {
        QTOPIABASE_HEADERS+=\
            qtransportauth_qws.h

        QTOPIABASE_PRIVATE_HEADERS+=\
            qtransportauth_qws_p.h

        QTOPIABASE_SOURCES+=\
            qtransportauth_qws.cpp
    }
}

!x11 {
    QTOPIABASE_PRIVATE_HEADERS+=\
        qsharedmemorycache_p.h

    QTOPIABASE_SOURCES+=\
        qsharedmemorycache.cpp
}

QTOPIA_CUSTOM=$$QPEDIR/src/libraries/qtopiabase/custom-qtopia
QTOPIABASE_HEADERS+=$${QTOPIA_CUSTOM}.h
QTOPIABASE_SOURCES+=$${QTOPIA_CUSTOM}.cpp

PREFIX=QTOPIABASE
resolve_include()

enable_dbusipc {
    depends(3rdparty/libraries/qtdbus)

    QTOPIABASE_PRIVATE_HEADERS+=\
        dbusapplicationchannel_p.h \
        dbusipccommon_p.h

    QTOPIABASE_SOURCES+=\
        dbusapplicationchannel_p.cpp \
        dbusipccommon_p.cpp
}


LIBS          +=-lm

# We need to prevent some files from appearing in TRANSLATABLES
TRANSLATABLES*=$$FORMS $$HEADERS $$SOURCES
TRANSLATABLES-=$${QTOPIA_CUSTOM}.h $${QTOPIA_CUSTOM}.cpp
CONFIG+=no_auto_translatables

sdk_qtopiabase_headers.files=$$QTOPIABASE_HEADERS
sdk_qtopiabase_headers.path=/include/qtopiabase
sdk_qtopiabase_headers.hint=sdk headers
INSTALLS+=sdk_qtopiabase_headers

sdk_qtopiabase_private_headers.files=$$QTOPIABASE_PRIVATE_HEADERS
sdk_qtopiabase_private_headers.path=/include/qtopiabase/private
sdk_qtopiabase_private_headers.hint=sdk headers
INSTALLS+=sdk_qtopiabase_private_headers

sdk_qtopiabase_custom_headers.files=custom-*-*.h
sdk_qtopiabase_custom_headers.path=/src/libraries/qtopiabase
sdk_qtopiabase_custom_headers.hint=sdk
INSTALLS+=sdk_qtopiabase_custom_headers

idep(LIBS+=-l$$TARGET)
qt_inc($$TARGET)
