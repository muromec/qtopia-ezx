qtopia_project(qtopia plugin)
TARGET=ficgta01vendor

CONFIG+=no_tr

HEADERS		=  vendor_ficgta01_p.h ficgta01plugin.h
SOURCES	        =  vendor_ficgta01.cpp ficgta01plugin.cpp

depends(libraries/qtopiaphonemodem)

