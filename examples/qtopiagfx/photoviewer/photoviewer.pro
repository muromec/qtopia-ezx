!qbuild {
qtopia_project(app)

depends(libraries/qtopiagfx)
QT += svg
CONFIG+=no_tr
}

TARGET = photoviewer
CONFIG += debug

# Input
SOURCES += main.cpp gfximageloader.cpp gfxcanvas.cpp imagecollection.cpp gfxcanvaslist.cpp simplehighlight.cpp gfxmenu.cpp timeview.cpp softkeybar.cpp camera.cpp tagdialog.cpp textedit.cpp photoriver.cpp header.cpp
HEADERS += gfximageloader.h gfxcanvas.h imagecollection.h gfxcanvaslist.h simplehighlight.h gfxmenu.h timeview.h softkeybar.h camera.h tagdialog.h textedit.h photoriver.h header.h

target.domain=trusted
