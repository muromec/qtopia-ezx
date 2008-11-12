qtopia_project(qtopia plugin)
TARGET=dbmigrate
plugin_type = qtopiasqlmigrate
CONFIG+=no_tr

SOURCES	= \
    migrateengine.cpp\
    qtopiapim/pimmigrate.cpp\
    qtopiaphone/phonemigrate.cpp

HEADERS  = \
    migrateengine.h\
    qtopiapim/pimmigrate.h\
    qtopiaphone/phonemigrate.h

RESOURCES+=\
    qtopiapim/pimmigrate.qrc\
    qtopiaphone/phonemigrate.qrc

!enable_singleexec {
    RESOURCES+=\
        $$QTOPIA_DEPOT_PATH/src/libraries/qtopia/qtopia.qrc\
        $$QTOPIA_DEPOT_PATH/src/libraries/qtopiapim/qtopiapim.qrc
}

depends(libraries/qtopia)
depends(libraries/qtopiabase)
depends(3rdparty/libraries/sqlite)

pkg.desc=Database upgrade migration utility
pkg.domain=trusted

dbmigrateservice.files=$$QTOPIA_DEPOT_PATH/services/DBMigrationEngine/dbmigrate
dbmigrateservice.path=/services/DBMigrationEngine
INSTALLS+=dbmigrateservice

dbmigrateDSservice.files=$$QTOPIA_DEPOT_PATH/etc/qds/DBMigrationEngine
dbmigrateDSservice.path=/etc/qds
INSTALLS+=dbmigrateDSservice
