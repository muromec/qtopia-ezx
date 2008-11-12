# can be built in two different ways.
# the first is as an application to demonstrate the composing nature of the 
# input method without bringing in complications to do with making it a plugin.
# the second method shows how it can then be turned into a plugin.

# coment the following line if you want to make the input method as an
# application

TARGET=composeim

qtopia_project(qtopia plugin)
# Disable i18n support
CONFIG+=no_tr
HEADERS      = composeim.h composeimpl.h
SOURCES      = composeim.cpp composeimpl.cpp
plugin_type  = inputmethods
