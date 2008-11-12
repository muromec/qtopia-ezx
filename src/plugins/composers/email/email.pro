qtopia_project(qtopia plugin)
TARGET=emailcomposer

HEADERS+=\
    addatt.h\
    addattdialogphone.h\
    emailcomposer.h

SOURCES+=\
    addatt.cpp\
    emailcomposer.cpp

TRANSLATABLES +=    $$HEADERS\
                    $$SOURCES

depends(libraries/qtopiamail)
