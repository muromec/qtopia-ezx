qtopia_project(qtopiacore)
QTOPIACORE_CONFIG=app rpath
qt=host
dir=tools/linguist/$$TARGET
depends(libraries/qt/xml)
depends(libraries/qt/gui)
depends(libraries/qt/corelib)
