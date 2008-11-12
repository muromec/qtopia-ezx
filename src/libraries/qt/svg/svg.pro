qtopia_project(qtopiacore)
QTOPIACORE_CONFIG=lib
qt=host
dir=src/$$TARGET
depends(libraries/qt/xml)
depends(libraries/qt/gui)
depends(libraries/qt/corelib)
