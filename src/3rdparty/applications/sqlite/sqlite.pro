qtopia_project(desktop external app)
license(FREEWARE)
include(../../libraries/sqlite/sqlite.pro)
CONFIG-=syncqtopia
INSTALLS-=sdk_headers
TARGET=sqlite
VPATH+=../../libraries/sqlite

SOURCES+=\
    shell.c

depends(3rdparty/libraries/pthread)

