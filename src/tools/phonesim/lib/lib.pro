qtopia_project(desktop lib)
TARGET=phonesim
CONFIG+=no_tr no_singleexec
DEFINES+=PHONESIM

VPATH+=$$QTOPIA_DEPOT_PATH/src/libraries/qtopiaphone
VPATH+=$$QTOPIA_DEPOT_PATH/src/libraries/qtopiacomm
VPATH+=$$QTOPIA_DEPOT_PATH/src/libraries/qtopiabase

INCLUDEPATH+=$$QPEDIR/include/qtopiacomm
INCLUDEPATH+=$$QPEDIR/include/qtopiaphone
INCLUDEPATH+=$$QPEDIR/include/qtopiabase
INCLUDEPATH+=$$QPEDIR/include/qtopia

PREFIX=PHONESIM
PHONESIM_HEADERS= phonesim.h server.h hardwaremanipulator.h \
                  qsmsmessagelist.h \
                  qsmsmessage.h \
                  qcbsmessage.h \
                  callmanager.h \
                  simfilesystem.h \
                  simapplication.h \
                  serial/qgsmcodec.h \
                  serial/qatutils.h \
                  serial/qatresultparser.h \
                  serial/qatresult.h \
		  wap/qwsppdu.h \
                  qsimcommand.h \
                  qsimenvelope.h \
                  qsimterminalresponse.h \
                  qsimcontrolevent.h
PHONESIM_SOURCES= phonesim.cpp server.cpp hardwaremanipulator.cpp \
                  qsmsmessagelist.cpp \
		  qsmsmessage.cpp \
		  qcbsmessage.cpp \
                  callmanager.cpp \
                  simfilesystem.cpp \
                  simapplication.cpp \
                  serial/qgsmcodec.cpp \
                  serial/qatutils.cpp \
                  serial/qatresultparser.cpp \
                  serial/qatresult.cpp \
		  wap/qwsppdu.cpp \
                  qsimcommand.cpp \
                  qsimenvelope.cpp \
                  qsimterminalresponse.cpp \
                  qsimcontrolevent.cpp

resolve_include()

phonesim_headers.files=$$PHONESIM_HEADERS
phonesim_headers.path=/include/phonesim
phonesim_headers.hint=headers

INSTALLS+=phonesim_headers

idep(LIBS+=-l$$TARGET)
qt_inc($$TARGET)

