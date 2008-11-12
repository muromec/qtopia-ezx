qtopia_project(qtopiacore)
QTOPIACORE_CONFIG=ignore_errors lib
qt=host
dir=tools/designer/src/$$TARGET/lib
depends(tools/qt/designer/src/lib)
depends(libraries/qt/xml)
depends(libraries/qt/gui)
depends(libraries/qt/corelib)
