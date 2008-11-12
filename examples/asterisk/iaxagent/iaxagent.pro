qtopia_project(qtopia app)
TARGET=iaxagent
CONFIG+=no_tr no_singleexec
!no_singleexec:CONFIG+=singleexec_main
requires(enable_voip)

INCLUDEPATH+=$$QTOPIA_DEPOT_PATH/examples/asterisk/iaxclient
INCLUDEPATH+=$$QTOPIA_DEPOT_PATH/src/3rdparty/libraries/gsm

HEADERS		= iaxcallprovider.h \
                  iaxconfiguration.h \
                  iaxnetworkregistration.h \
                  iaxservicechecker.h \
                  iaxtelephonyservice.h
SOURCES		= main.cpp \
                  iaxcallprovider.cpp \
                  iaxconfiguration.cpp \
                  iaxnetworkregistration.cpp \
                  iaxservicechecker.cpp \
                  iaxtelephonyservice.cpp

pkg.desc=IAX telephony agent for use with Asterisk servers
pkg.domain=trusted

depends(libraries/qtopiaphone)
depends(libraries/qtopiaaudio)
depends(asterisk/iaxclient)

telephonyservice.files=$$QTOPIA_DEPOT_PATH/examples/asterisk/services/Telephony/iaxagent
telephonyservice.path=/services/Telephony
INSTALLS+=telephonyservice

