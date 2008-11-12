qtopia_project(qtopia lib)
TARGET=qtopiaphonemodem

HEADERS= \
    qmodemservice.h \
    qmodemserviceplugin.h \
    qmodemnetworkregistration.h \
    qmodempreferrednetworkoperators.h \
    qmodemcallbarring.h \
    qmodemcallforwarding.h \
    qmodemcallsettings.h \
    qmodemsmsreader.h \
    qmodemsmssender.h \
    qmodempinmanager.h \
    qmodemcall.h \
    qmodemcallprovider.h \
    qmodemsimtoolkit.h \
    qmodemphonebook.h \
    qmodemcellbroadcast.h \
    qmodemservicenumbers.h \
    qmodemsupplementaryservices.h \
    qmodemsiminfo.h \
    qmodemindicators.h \
    qmodemsimfiles.h \
    qmodemsimgenericaccess.h \
    qmodemadviceofcharge.h \
    qmodemconfiguration.h \
    qmodemvibrateaccessory.h \
    qmodemrffunctionality.h \
    qmodemgprsnetworkregistration.h \
    qmodemcallvolume.h
PRIVATE_HEADERS=\
    qmodempinmanager_p.h \
    qmodemsimfiles_p.h \
    qmodempppdmanager_p.h
SOURCES=\
    qmodemservice.cpp \
    qmodemserviceplugin.cpp \
    qmodemnetworkregistration.cpp \
    qmodempreferrednetworkoperators.cpp \
    qmodemcallbarring.cpp \
    qmodemcallforwarding.cpp \
    qmodemcallsettings.cpp \
    qmodemsmsreader.cpp \
    qmodemsmssender.cpp \
    qmodempinmanager.cpp \
    qmodemcall.cpp \
    qmodemcallprovider.cpp \
    qmodemsimtoolkit.cpp \
    qmodemphonebook.cpp \
    qmodemcellbroadcast.cpp \
    qmodemservicenumbers.cpp \
    qmodemsupplementaryservices.cpp \
    qmodemsiminfo.cpp \
    qmodemindicators.cpp \
    qmodemsimfiles.cpp \
    qmodemsimgenericaccess.cpp \
    qmodemadviceofcharge.cpp \
    qmodemconfiguration.cpp \
    qmodemvibrateaccessory.cpp \
    qmodemrffunctionality.cpp \
    qmodemgprsnetworkregistration.cpp \
    qmodempppdmanager.cpp \
    qmodemcallvolume.cpp

CONFIG += qtopia_visibility no_tr
    
depends(libraries/qtopiaphone)

sdk_qtopiaphonemodem_headers.files=$${HEADERS}
sdk_qtopiaphonemodem_headers.path=/include/qtopiaphonemodem
sdk_qtopiaphonemodem_headers.hint=sdk headers
INSTALLS+=sdk_qtopiaphonemodem_headers

sdk_qtopiaphonemodem_private_headers.files=$${PRIVATE_HEADERS}
sdk_qtopiaphonemodem_private_headers.path=/include/qtopiaphonemodem/private
sdk_qtopiaphonemodem_private_headers.hint=sdk headers
INSTALLS+=sdk_qtopiaphonemodem_private_headers


HEADERS+=$$PRIVATE_HEADERS

idep(LIBS+=-l$$TARGET)
qt_inc($$TARGET)
