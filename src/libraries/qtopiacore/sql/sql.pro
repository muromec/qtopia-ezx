qtopia_project(qtopiacore)
QTOPIACORE_CONFIG=lib
qt=target
dir=src/$$TARGET
depends(libraries/qtopiacore/corelib)
build_qtopia_sqlite:depends(3rdparty/libraries/sqlite,fake)
else:depends(3rdparty/libraries/sqlite)
