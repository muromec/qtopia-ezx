qtopia_project(qtopia app)
TARGET=camera
CONFIG+=qtopia_main

FORMS=camerabase.ui camerasettings.ui

HEADERS = mainwindow.h \
          thumbbutton.h \
          videocaptureview.h \
          videocapturedevice.h \
          videocapturedevicefactory.h \
          formatconverter.h \
          bayerconverter.h \
          phototimer.h \
          minsecspinbox.h\
          noeditspinbox.h\
          yuvconverter.h

SOURCES = mainwindow.cpp \
          videocaptureview.cpp \
          formatconverter.cpp \
          bayerconverter.cpp \
          main.cpp \
          phototimer.cpp \
          minsecspinbox.cpp\
          noeditspinbox.cpp\
          yuvconverter.cpp

# This is documented in src/build/doc/src/deviceprofiles.qdoc
!isEmpty(DEVICE_CONFIG_PATH) {
    DEVICE_HEADERS=$$files($$DEVICE_CONFIG_PATH/camera/*.h)
    DEVICE_SOURCES=$$files($$DEVICE_CONFIG_PATH/camera/*.cpp)
}

!isEmpty(DEVICE_SOURCES) {
    HEADERS+=$$DEVICE_HEADERS
    SOURCES+=$$DEVICE_SOURCES
} else {
    HEADERS+=\
            dummyvideocapturedevice.h \
            v4l1videocapturedevice.h \
            v4l2videocapturedevice.h \
            nodevice.h
    SOURCES+=\
            videocapturedevicefactory.cpp \
            dummyvideocapturedevice.cpp \
            v4l1videocapturedevice.cpp \
            v4l2videocapturedevice.cpp
}

desktop.files=$$QTOPIA_DEPOT_PATH/apps/Applications/camera.desktop
desktop.path=/apps/Applications
desktop.hint=desktop
INSTALLS+=desktop
help.source=$$QTOPIA_DEPOT_PATH/help
help.files=camera*
help.hint=help
INSTALLS+=help
pics.files=$$QTOPIA_DEPOT_PATH/pics/camera/*
pics.path=/pics/camera
pics.hint=pics
INSTALLS+=pics
service.files=$$QTOPIA_DEPOT_PATH/services/Camera/camera
service.path=/services/Camera
INSTALLS+=service
qdsservice.files=$$QTOPIA_DEPOT_PATH/etc/qds/Camera
qdsservice.path=/etc/qds
INSTALLS+=qdsservice

# the server does this for us
#categories.files=$$QTOPIA_DEPOT_PATH/etc/categories/camera.conf
#categories.trtarget=QtopiaCategories
#categories.hint=nct
#INSTALLS+=categories

pkg.desc=Camera
pkg.domain=trusted
