qtopia_project(qtopia plugin)
TARGET=smilviewer
requires(enable_cell)

enable_cell:contains(PROJECTS,libraries/qtopiasmil) {

    HEADERS+=\
        smilviewer.h

    SOURCES+=\
        smilviewer.cpp

    TRANSLATABLES +=    $$HEADERS\
                        $$SOURCES

    depends(libraries/qtopiamail)
    depends(libraries/qtopiasmil)
}
