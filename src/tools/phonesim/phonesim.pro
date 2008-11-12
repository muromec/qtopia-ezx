qtopia_project(desktop app)

CONFIG+=no_tr no_install no_singleexec
FORMS*=controlbase.ui
HEADERS*=control.h attranslator.h gsmspec.h gsmitem.h 
SOURCES*=main.cpp control.cpp attranslator.cpp gsmspec.cpp gsmitem.cpp

depends(tools/phonesim/lib)

