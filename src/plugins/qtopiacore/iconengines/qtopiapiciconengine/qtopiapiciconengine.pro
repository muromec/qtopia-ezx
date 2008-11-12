qtopia_project(embedded qtopia core plugin)
CONFIG+=no_tr

HEADERS += piciconengine.h
SOURCES += piciconengine.cpp

TARGET  = qtopiapiciconengine

depends(libraries/qtopiabase)
