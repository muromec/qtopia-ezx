qtopia_project(qtopia plugin)
TARGET=genericviewer

!enable_cell {
    DEFINES+=QTOPIA_NO_SMS
    !enable_voip:DEFINES+=QTOPIA_NO_DIAL_FUNCTION
}

HEADERS+=\
    attachmentoptions.h\
    browser.h\
    genericviewer.h

SOURCES+=\
    attachmentoptions.cpp\
    browser.cpp\
    genericviewer.cpp

TRANSLATABLES +=    $$HEADERS\
                    $$SOURCES

depends(libraries/qtopiamail)
