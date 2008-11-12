qtopia_project(desktop app)
TARGET=content_installer
CONFIG+=no_tr

RESOURCES+=qtopia.qrc
# for 4.3, link in the qrc and use QResource to iterate over the tables
# RESOURCES+=qtopiapim.qrc

### Needs routines from qtopia
VPATH+=$$QTOPIA_DEPOT_PATH/src/libraries/qtopia
INCLUDEPATH+=$$QPEDIR/include/qtopia

### Also needs qLog
VPATH+=$$QTOPIA_DEPOT_PATH/src/libraries/qtopiabase
INCLUDEPATH+=$$QPEDIR/include/qtopiabase

### Also needs the QtopiaPIM database stuff
VPATH+=$$QTOPIA_DEPOT_PATH/src/libraries/qtopiapim
INCLUDEPATH+=$$QPEDIR/include/qtopiapim
RESOURCES+=qtopiapim.qrc

VPATH+=$$QTOPIA_DEPOT_PATH/src/tools/dbmigrate
VPATH+=$$QTOPIA_DEPOT_PATH/src/tools/dbmigrate/qtopiapim
INCLUDEPATH+=$$QPEDIR/src/tools/dbmigrate

### Also needs the QtopiaPhone database schema
VPATH+=$$QTOPIA_DEPOT_PATH/src/tools/dbmigrate/qtopiaphone
RESOURCES+=phonemigrate.qrc

DEFINES+=QTOPIA_CONTENT_INSTALLER

SOURCES	= main.cpp \
    qtopiasql.cpp \
    qtopiasql_p.cpp \
    qtopialog.cpp \
    qlog.cpp \
    qcontentfilter.cpp \
    qcontent.cpp \
    qthumbnail.cpp \
    thumbnailview_p.cpp \
    qcontentset.cpp \
    qdrmcontent.cpp \
    qdrmrights.cpp \
    drmcontent_p.cpp \
    qcontentplugin.cpp \
    contentpluginmanager_p.cpp \
    qcategorymanager.cpp \
    qtopiaipcadaptor.cpp \
    qtopiaipcenvelope.cpp \
    qtopiachannel.cpp \
    qtopianamespace.cpp \
    qpluginmanager.cpp \
    qstorage.cpp \
    qmimetype.cpp \
    qtopiaservices.cpp \
    qslotinvoker.cpp \
    qsignalintercepter.cpp \
    quniqueid.cpp \
    migrateengine.cpp \
    pimmigrate.cpp \
    phonemigrate.cpp \
    qcategorystore.cpp \
    qsqlcategorystore.cpp \
    qcontentengine.cpp \
    qfscontentengine.cpp \
    qcontentstore.cpp \
    qsqlcontentstore.cpp \
    qcontentsetengine.cpp \
    qmimetypedata.cpp \
    qcontentsortcriteria.cpp \
    qsqlcontentsetengine.cpp

HEADERS = \
    qtopiasql.h \
    qtopiasql_p.h \
    qtopialog.h \
    qlog.h \
    qcontentfilter.h \
    qcontent.h \
    qcontentset.h \
    thumbnailview_p.h \
    qdrmcontent.h \
    qdrmrights.h \
    drmcontent_p.h \
    qcontentplugin.h \
    qthumbnail.h \
    contentpluginmanager_p.h \
    qcategorymanager.h \
    qtopiaipcadaptor.h \
    qtopiaipcenvelope.h \
    qtopiachannel.h \
    qtopianamespace.h \
    qpluginmanager.h \
    qstorage.h \
    qmimetype.h \
    qtopiaservices.h \
    qslotinvoker.h \
    qsignalintercepter.h \
    quniqueid.h \
    migrateengine.h \
    pimmigrate.h \
    phonemigrate.h \
    qcategorystore_p.h \
    qsqlcategorystore_p.h \
    qcontentstore_p.h \
    qsqlcontentstore_p.h \
    qcontentsetengine_p.h \
    qmimetypedata_p.h \
    qsqlcontentsetengine_p.h

RESOURCES += pimmigrate.qrc

INCLUDEPATH+=$$QTOPIA_DEPOT_PATH/src/3rdparty/libraries/sqlite
backup.TARGET=$$TARGET
VPATH+=$$QTOPIA_DEPOT_PATH/src/3rdparty/libraries/sqlite
!build_qtopia_sqlite:CONFIG+=set_build_qtopia_sqlite
set_build_qtopia_sqlite:CONFIG+=build_qtopia_sqlite
include($$QTOPIA_DEPOT_PATH/src/3rdparty/libraries/sqlite/sqlite.pro)
set_build_qtopia_sqlite:CONFIG-=build_qtopia_sqlite
CONFIG-=syncqtopia
TARGET=$$backup.TARGET

