qtopia_project(qtopia app)
TARGET=radioplayer
CONFIG+=qtopia_main
requires(!platform)

DEFINES+=QTOPIA_DUMMY_RADIO

RESOURCES       = radioplayer.qrc
FORMS           = radioplayer.ui
HEADERS		= radioplayer.h \
                  radioband.h \
                  radiobandv4l.h \
                  radiobanddummy.h \
                  radiobandmanager.h \
                  radiopresets.h \
                  radioservice.h \
                  rdsgroup.h \
                  rdsprograminfo.h
SOURCES		= radioplayer.cpp \
                  radioband.cpp \
                  radiobandv4l.cpp \
                  radiobanddummy.cpp \
                  radiobandmanager.cpp \
                  radiopresets.cpp \
                  radioservice.cpp \
                  rdsgroup.cpp \
                  rdsprograminfo.cpp \
                  main.cpp

desktop.files=$$QTOPIA_DEPOT_PATH/examples/radioplayer/desktop/radioplayer.desktop
desktop.path=/apps/Applications
desktop.hint=desktop
INSTALLS+=desktop
#help.source=$$QTOPIA_DEPOT_PATH/help
#help.files=radioplayer*
#help.hint=help
#INSTALLS+=help
pics.files=$$QTOPIA_DEPOT_PATH/examples/radioplayer/pics/*
pics.path=/pics/radioplayer
pics.hint=pics
INSTALLS+=pics
service.files=$$QTOPIA_DEPOT_PATH/examples/radioplayer/services/Radio/radioplayer
service.path=/services/Radio
INSTALLS+=service
servicedef.files=$$QTOPIA_DEPOT_PATH/examples/radioplayer/services/Radio.service
servicedef.path=/services
INSTALLS+=servicedef

pkg.desc=Radio player for Qtopia.
pkg.domain=trusted
