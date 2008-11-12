qtopia_project(qtopia app)
TARGET=sysinfo
CONFIG+=qtopia_main

HEADERS		= memory.h \
		  graph.h \
		  load.h \
		  storage.h \
		  versioninfo.h \
		  sysinfo.h \
                  dataview.h \
                  securityinfo.h 
#                  cleanupwizard.h

SOURCES		= memory.cpp \
		  graph.cpp \
		  load.cpp \
		  storage.cpp \
		  versioninfo.cpp \
		  sysinfo.cpp \
		  main.cpp \
                  dataview.cpp \
                  securityinfo.cpp 
#                  cleanupwizard.cpp

# Always rebuild versioninfo.o so that the reported build date is correct
create_raw_dependency($$OBJECTS_DIR/versioninfo.o,FORCE)

depends(libraries/qtopiacomm)
enable_cell {
    HEADERS += siminfo.h modeminfo.h
    SOURCES += siminfo.cpp modeminfo.cpp
    depends(libraries/qtopiaphone)
}

TRANSLATABLES   += \
                    cleanupwizard.h

help.source=$$QTOPIA_DEPOT_PATH/help
help.files=sysinfo*
help.hint=help
INSTALLS+=help
pics.files=$$QTOPIA_DEPOT_PATH/pics/sysinfo/*
pics.path=/pics/sysinfo
pics.hint=pics
INSTALLS+=pics
desktop.files=$$QTOPIA_DEPOT_PATH/apps/Applications/sysinfo.desktop
desktop.path=/apps/Applications
desktop.hint=desktop
INSTALLS+=desktop
cleanupservice.files=$$QTOPIA_DEPOT_PATH/services/CleanupWizard/sysinfo
cleanupservice.path=/services/CleanupWizard
INSTALLS+=cleanupservice

pkg.desc=System information for Qtopia.
pkg.domain=trusted
