qtopia_project(qtopia plugin)
TARGET=examplevendor

CONFIG+=no_tr

HEADERS		=  vendor_example_p.h exampleplugin.h
SOURCES	        =  vendor_example.cpp exampleplugin.cpp

requires(enable_modem)
depends(libraries/qtopiaphonemodem)
