qtopia_project(qtopiacore)
QTOPIACORE_CONFIG=app rpath ignore_errors
qt=host
dir=tools/linguist/$$TARGET
depends(libraries/qt/network)
depends(tools/qt/designer/src/uitools)
depends(libraries/qt/script)
depends(libraries/qt/xml)
depends(libraries/qt/gui)
depends(libraries/qt/corelib)
depends(tools/qt/assistant/lib)
