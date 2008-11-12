qtopia_project(qtopia plugin)
TARGET=genericcomposer

!enable_cell:DEFINES+=QTOPIA_NO_SMS

HEADERS+=\
    genericcomposer.h\
    templatetext.h

SOURCES+=\
    genericcomposer.cpp\
    templatetext.cpp

TRANSLATABLES +=    $$HEADERS\
                    $$SOURCES

depends(libraries/qtopiamail)
