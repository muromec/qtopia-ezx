qtopia_project(qtopia app)
TARGET=mediarecorder
CONFIG+=qtopia_main

FORMS      = mediarecorderbase.ui confrecorderbase.ui
HEADERS		= mediarecorder.h pluginlist.h \
		  samplebuffer.h timeprogressbar.h confrecorder.h waveform.h audioparameters_p.h
SOURCES		= mediarecorder.cpp pluginlist.cpp samplebuffer.cpp \
                  timeprogressbar.cpp confrecorder.cpp waveform.cpp main.cpp audioparameters.cpp

depends(libraries/qtopiaaudio)

desktop.files=$$QTOPIA_DEPOT_PATH/apps/Applications/mediarecorder.desktop
desktop.path=/apps/Applications
desktop.hint=desktop
INSTALLS+=desktop
help.source=$$QTOPIA_DEPOT_PATH/help
help.files=mediarecorder*
help.hint=help
INSTALLS+=help
pics.files=$$QTOPIA_DEPOT_PATH/pics/mediarecorder/*
pics.path=/pics/mediarecorder
pics.hint=pics
INSTALLS+=pics
voicerecorderservice.files=$$QTOPIA_DEPOT_PATH/services/VoiceRecording/mediarecorder
voicerecorderservice.path=/services/VoiceRecording
INSTALLS+=voicerecorderservice
qdsservice.files=$$QTOPIA_DEPOT_PATH/etc/qds/VoiceRecording
qdsservice.path=/etc/qds
INSTALLS+=qdsservice

# the server does this for us
#categories.files=$$QTOPIA_DEPOT_PATH/etc/categories/mediarecorder.conf
#categories.trtarget=QtopiaCategories
#categories.hint=nct
#INSTALLS+=categories

pkg.desc=Media recorder for Qtopia.
pkg.domain=trusted
