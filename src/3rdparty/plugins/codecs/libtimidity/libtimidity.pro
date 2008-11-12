qtopia_project(qtopia plugin)
TARGET=timidityplugin
CONFIG+=no_tr
license(FREEWARE)

HEADERS		= midiplugin.h mididecoder.h
SOURCES	    = midiplugin.cpp mididecoder.cpp

depends(3rdparty/libraries/libtimidity)
# libtimidity can't be built singleexec (except for GPL packages)
# We need to disable this plugin manually because of the way dependencies
# are pulled into the server for a singleexec build
!free_package:idep(CONFIG+=no_singleexec,CONFIG)
depends(libraries/qtopiamedia)
