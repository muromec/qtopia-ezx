qtopia_project(qtopia app)
TARGET=dbmigrate
CONFIG+=no_tr singleexec_main

SOURCES	= \
    main.cpp \
    dbmigrateservice.cpp\

HEADERS  = \
    dbmigrateservice.h

depends(libraries/qtopia)
depends(libraries/qtopiabase)
depends(3rdparty/libraries/sqlite)
depends(tools/dbmigrate)

pkg.desc=Database upgrade migration utility
pkg.domain=trusted

dbmigrateservice.files=$$QTOPIA_DEPOT_PATH/services/DBMigrationEngine/dbmigrate
dbmigrateservice.path=/services/DBMigrationEngine
INSTALLS+=dbmigrateservice

dbmigrateDSservice.files=$$QTOPIA_DEPOT_PATH/etc/qds/DBMigrationEngine
dbmigrateDSservice.path=/etc/qds
INSTALLS+=dbmigrateDSservice
