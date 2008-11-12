qtopia_project(qtopiacore)
QTOPIACORE_CONFIG=lib
qt=target
dir=src/$$TARGET
depends(libraries/qtopiacore/xml)
depends(libraries/qtopiacore/gui)
depends(libraries/qtopiacore/corelib)
