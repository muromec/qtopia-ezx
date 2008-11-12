qtopia_project(qtopiacore)
QTOPIACORE_CONFIG=lib
qt=target
dir=src/$$TARGET
depends(libraries/qtopiacore/corelib)
contains(QTE_CONFIG,x11sm):idep(CONFIG+=x11sm)
