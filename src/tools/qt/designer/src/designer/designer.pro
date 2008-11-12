qtopia_project(qtopiacore)
QTOPIACORE_CONFIG=ignore_errors app rpath
qt=host
dir=tools/designer/src/$$TARGET
depends(tools/qt/designer/src/components)
depends(tools/qt/designer/src/lib)
depends(tools/qt/assistant/lib)
depends(libraries/qt/script)
depends(libraries/qt/xml)
depends(libraries/qt/gui)
depends(libraries/qt/network)
depends(libraries/qt/corelib)
