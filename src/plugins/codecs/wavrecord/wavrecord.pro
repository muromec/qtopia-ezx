qtopia_project(qtopia plugin)
TARGET=wavrecord

HEADERS		=  wavrecord.h wavrecordimpl.h
SOURCES	        =  wavrecord.cpp wavrecordimpl.cpp

depends(3rdparty/libraries/gsm)
