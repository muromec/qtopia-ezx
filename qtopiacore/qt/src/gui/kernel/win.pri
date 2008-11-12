# Qt/Windows only configuration file
# --------------------------------------------------------------------

	wince-* {
		HEADERS += $$KERNEL_H/qfunctions_wce.h
		SOURCES -= $$KERNEL_CPP/qfontengine_win.cpp \
			   $$KERNEL_CPP/qregion_win.cpp
		SOURCES += $$KERNEL_CPP/qfunctions_wce.cpp \
			   $$KERNEL_CPP/qfontengine_wce.cpp \			   
			   $$KERNEL_CPP/qregion_wce.cpp
	}
        INCLUDEPATH += ../3rdparty/wintab
