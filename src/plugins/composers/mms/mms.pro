qtopia_project(qtopia plugin)
TARGET=mmscomposer

enable_cell:contains(PROJECTS,libraries/qtopiasmil) {

    HEADERS+=\
        mmscomposer.h

    SOURCES+=\
        mmscomposer.cpp

    TRANSLATABLES +=    $$HEADERS\
                        $$SOURCES

    depends(libraries/qtopiamail)
    depends(libraries/qtopia)
    depends(libraries/qtopiasmil)
} else {
    DEFINES+=QTOPIA_NO_MMS
}
