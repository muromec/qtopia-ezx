# can be built in two different ways.
# the first is as an application to demonstrate the 
# input method without bringing in complications to do with making it a plugin.
# the second method shows how it can then be turned into a plugin.

# Disable i18n support
CONFIG+=no_tr

# coment the following line if you want to make the input method as an
# application
CONFIG += popupim_as_plugin

# comment the following line if you want to make the input method as
# and older style InputMethodInterface, compatible with Qtopia 1.5
CONFIG += popupim_extended_interface

TARGET=popupim

popupim_as_plugin {
    qtopia_project(qtopia qtopia plugin)
    HEADERS      = popupim.h
    SOURCES      = popupim.cpp
    popupim_extended_interface {
	HEADERS += popupextimpl.h
	SOURCES += popupextimpl.cpp
    } else {
	HEADERS += popupimpl.h
	SOURCES += popupimpl.cpp
    }
    plugin_type  = inputmethods
} else {
    qtopia_project(qtopia qtopia app)
    HEADERS      = popupim.h
    SOURCES      = popupim.cpp main.cpp
}

