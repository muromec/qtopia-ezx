qtopia_project( qtopia app )
TARGET=sxemonitor
CONFIG+=no_quicklaunch


depends(libraries/qtopia)
 
# Input
HEADERS += sxemonitor.h sxemonqlog.h  
SOURCES += main.cpp sxemonitor.cpp sxemonqlog.cpp

# SXE permissions required
pkg.domain=trusted

