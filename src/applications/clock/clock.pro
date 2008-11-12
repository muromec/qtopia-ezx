qtopia_project(qtopia app)
TARGET=clock
CONFIG+=qtopia_main

FORMS	= clockbase.ui stopwatchbase.ui alarmbase.ui
HEADERS		= clockmain.h \
		    clock.h \
		    stopwatch.h \
		    alarm.h
SOURCES		= clockmain.cpp \
		    clock.cpp \
		    stopwatch.cpp \
		    alarm.cpp \
		    main.cpp

desktop.files=$$QTOPIA_DEPOT_PATH/apps/Applications/clock.desktop
desktop.path=/apps/Applications
desktop.hint=desktop
INSTALLS+=desktop
servicea.files=$$QTOPIA_DEPOT_PATH/services/Alarm/clock
servicea.path=/services/Alarm
INSTALLS+=servicea
serviceb.files=$$QTOPIA_DEPOT_PATH/services/Clock/clock
serviceb.path=/services/Clock
INSTALLS+=serviceb
timeservice.files=$$QTOPIA_DEPOT_PATH/services/TimeMonitor/clock
timeservice.path=/services/TimeMonitor
INSTALLS+=timeservice
sound.files=$$QTOPIA_DEPOT_PATH/sounds/alarm.wav
sound.path=/sounds
INSTALLS+=sound
help.source=$$QTOPIA_DEPOT_PATH/help
help.files=clock*
help.hint=help
INSTALLS+=help
pics.files=$$QTOPIA_DEPOT_PATH/pics/clock/*
pics.path=/pics/clock
pics.hint=pics
INSTALLS+=pics

pkg.desc=A simple clock and stop-watch for the Qtopia environment.
pkg.domain=trusted
