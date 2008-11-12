qtopia_project(qtopia plugin)
TARGET=omap730vendor

CONFIG+=no_tr

HEADERS		=  vendor_omap730_p.h omap730plugin.h
SOURCES	        =  vendor_omap730.cpp omap730plugin.cpp

depends(libraries/qtopiaphonemodem)

