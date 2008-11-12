qtopia_project(qtopia lib)
TARGET=qtopiaaudio
CONFIG+=no_tr qtopia_visibility

QTOPIAAUDIO_HEADERS = \
    qaudioinput.h \
    qaudiooutput.h \
    qaudiostateconfiguration.h \
    qaudiostateplugin.h \
    qaudiostate.h \
    qaudiostateinfo.h \
    qaudiostatemanager.h \
    qaudionamespace.h 

QTOPIAAUDIO_SOURCES = \
    qaudiostateconfiguration.cpp \
    qaudiostateplugin.cpp \
    qaudiostate.cpp \
    qaudiostateinfo.cpp \
    qaudiostatemanager.cpp \
    qaudionamespace.cpp \
    qaudiostatemanagerservice.cpp 

QTOPIAAUDIO_PRIVATE_HEADERS = \
    qaudiostatemanagerservice_p.h

equals(QTOPIA_SOUND_SYSTEM,alsa) {
    depends(3rdparty/libraries/alsa)
    QTOPIAAUDIO_SOURCES+=qaudioinput_alsa.cpp qaudiooutput_alsa.cpp
    DEFINES+=QTOPIA_HAVE_ALSA
}

equals(QTOPIA_SOUND_SYSTEM,oss) {
    QTOPIAAUDIO_SOURCES+=qaudioinput_oss.cpp

    enable_qtopiamedia {
        QTOPIAAUDIO_SOURCES+=qaudiooutput_oss.cpp
        DEFINES+=QTOPIA_HAVE_OSS
    }
    else {
        QTOPIAAUDIO_SOURCES+=qaudiooutput_qss.cpp
        DEFINES+=QTOPIA_HAVE_QSS
    }
}

PREFIX=QTOPIAAUDIO
resolve_include()

sdk_qtopiaaudio_headers.files=$${QTOPIAAUDIO_HEADERS}
sdk_qtopiaaudio_headers.path=/include/qtopia/audio
sdk_qtopiaaudio_headers.hint=sdk headers
INSTALLS+=sdk_qtopiaaudio_headers

sdk_qtopiaaudio_private_headers.files=$${QTOPIAAUDIO_PRIVATE_HEADERS}
sdk_qtopiaaudio_private_headers.path=/include/qtopia/audio/private
sdk_qtopiaaudio_private_headers.hint=sdk headers
INSTALLS+=sdk_qtopiaaudio_private_headers

idep(LIBS+=-l$$TARGET)
qt_inc($$TARGET)
