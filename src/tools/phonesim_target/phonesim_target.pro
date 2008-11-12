qtopia_project(embedded app)
TARGET=phonesim_target
CONFIG+=no_tr no_install no_singleexec
DEFINES += PHONESIM PHONESIM_TARGET
VPATH += ../phonesim
VPATH += ../phonesim/lib

VPATH+=$$QTOPIA_DEPOT_PATH/src/libraries/qtopiaphone
VPATH+=$$QTOPIA_DEPOT_PATH/src/libraries/qtopiacomm
VPATH+=$$QTOPIA_DEPOT_PATH/src/libraries/qtopiabase

INCLUDEPATH+=$$QPEDIR/include/qtopiacomm
depends(libraries/qtopiacomm,fake)
INCLUDEPATH+=$$QTOPIA_DEPOT_PATH/src/libraries/qtopiaphone
depends(libraries/qtopiaphone,fake)
INCLUDEPATH+=$$QPEDIR/include/qtopiabase
depends(libraries/qtopiabase,fake)
INCLUDEPATH+=$$QPEDIR/include/qtopia
depends(libraries/qtopia,fake)

HEADERS		= phonesim.h server.h simfilesystem.h callmanager.h \
                  serial/qgsmcodec.h serial/qatutils.h \
                  serial/qatresultparser.h serial/qatresult.h \
                  simapplication.h qsimcommand.h qsimterminalresponse.h \
                  qsimenvelope.h qsimcontrolevent.h

SOURCES		= main.cpp phonesim.cpp server.cpp callmanager.cpp \
                  simfilesystem.cpp \
                  serial/qgsmcodec.cpp serial/qatutils.cpp \
                  serial/qatresultparser.cpp serial/qatresult.cpp \
                  simapplication.cpp qsimcommand.cpp qsimterminalresponse.cpp \
                  qsimenvelope.cpp qsimcontrolevent.cpp

