qtopia_project(qtopia plugin)
TARGET=wavecomvendor

CONFIG+=no_tr

HEADERS		=  vendor_wavecom_p.h wavecomplugin.h
SOURCES	        =  vendor_wavecom.cpp wavecomplugin.cpp

depends(libraries/qtopiaphonemodem)

