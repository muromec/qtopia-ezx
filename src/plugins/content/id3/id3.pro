qtopia_project(qtopia plugin)
TARGET=id3

CONFIG+=no_tr

HEADERS		=  id3contentplugin.h \ 
                   id3tag.h \
                   id3frame.h

SOURCES	        =  id3contentplugin.cpp \
                   id3tag.cpp \
                   id3frame.cpp

depends(libraries/qtopia)