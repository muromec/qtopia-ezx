qtopia_project(qtopia plugin)
TARGET=greenphonevendor

CONFIG+=no_tr

HEADERS		=  vendor_greenphone_p.h greenphoneplugin.h
SOURCES	        =  vendor_greenphone.cpp greenphoneplugin.cpp

requires(enable_modem)
depends(libraries/qtopiaphonemodem)
