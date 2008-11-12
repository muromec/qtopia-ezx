qtopia_project(qtopia app)
TARGET=packagemanager
CONFIG+=qtopia_main
# Give us a direct connection to the document system
DEFINES+=QTOPIA_DIRECT_DOCUMENT_SYSTEM_CONNECTION

FORMS           = serveredit.ui \
                    packagedetails.ui
HEADERS         = packageview.h \
                    packagemodel.h \
                    packagecontroller.h \
                    packageinformationreader.h \
                    installcontrol.h \
                    serveredit.h \
                    httpfetcher.h \
                    installedpackagescanner.h\
                    targz.h \
                    sandboxinstall.h \
                    md5file.h \
                    packagemanagerservice.h \
                    version.h \
                    utils.h

SOURCES         = main.cpp \
                    packageview.cpp \
                    packagemodel.cpp \
                    packagecontroller.cpp \
                    packageinformationreader.cpp \
                    installcontrol.cpp \
                    serveredit.cpp \
                    httpfetcher.cpp \
                    installedpackagescanner.cpp\
                    targz.cpp \
                    sandboxinstall.cpp \
                    md5file.cpp \
                    packagemanagerservice.cpp \
                    version.cpp \
                    utils.cpp

INCLUDEPATH+=$$QT_DEPOT_PATH/src/3rdparty/md5

depends(3rdparty/libraries/tar)
depends(3rdparty/libraries/md5)
enable_sxe:depends(libraries/qtopiasecurity)

help.source=$$QTOPIA_DEPOT_PATH/help
help.files=packagemanager*
help.hint=help
INSTALLS+=help
desktop.files=$$QTOPIA_DEPOT_PATH/apps/Settings/packagemanager.desktop
desktop.path=/apps/Settings
desktop.hint=desktop
INSTALLS+=desktop
pics.files=$$QTOPIA_DEPOT_PATH/pics/packagemanager/*
pics.path=/pics/packagemanager
pics.hint=pics
            
# This is documented in src/build/doc/src/deviceprofiles.qdoc
!isEmpty(DEVICE_CONFIG_PATH):exists($$DEVICE_CONFIG_PATH/etc/default/Trolltech/PackageManager.conf) {
    secsettings.files+=$$DEVICE_CONFIG_PATH/etc/default/Trolltech/PackageManager.conf
} else {
    secsettings.files+=$$QTOPIA_DEPOT_PATH/etc/default/Trolltech/PackageManager.conf
}
INSTALLS+=pics
secsettings.path=/etc/default/Trolltech
secsettings.hint=secsettings
INSTALLS+=secsettings
packagemanagerservice.files=$$QTOPIA_DEPOT_PATH/services/PackageManager/packagemanager
packagemanagerservice.path=/services/PackageManager
INSTALLS+=packagemanagerservice
qdspackagemanagerservice.files=$$QTOPIA_DEPOT_PATH/etc/qds/PackageManager
qdspackagemanagerservice.path=/etc/qds
INSTALLS+=qdspackagemanagerservice

packages_category.files=$$QTOPIA_DEPOT_PATH/apps/Packages/.directory
packages_category.path=/apps/Packages
packages_category.hint=desktop
INSTALLS+=packages_category

enable_sxe {
    SOURCES+=domaininfo.cpp
    HEADERS+=domaininfo.h
}

pkg.desc=Safely download and install programs for Qtopia.
pkg.domain=trusted
