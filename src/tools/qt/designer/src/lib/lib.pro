qtopia_project(qtopiacore)
QTOPIACORE_CONFIG=ignore_errors lib
qt=host
dir=tools/designer/src/$$TARGET
depends(libraries/qt/script)
depends(libraries/qt/xml)
depends(libraries/qt/gui)
depends(libraries/qt/corelib)
