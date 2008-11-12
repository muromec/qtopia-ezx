qtopia_project(qtopia plugin)
TARGET=greenphoneaudiohardware

HEADERS		=  greenphoneaudioplugin.h
SOURCES	        =  greenphoneaudioplugin.cpp

depends(libraries/qtopiaaudio)
enable_bluetooth {
    depends(libraries/qtopiacomm)
}
