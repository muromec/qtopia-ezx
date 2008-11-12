# can be built in two different ways.
# the first is as an application to demonstrate the composing nature of the 
# input method without bringing in complications to do with making it a plugin.
# the second method shows how it can then be turned into a plugin.

# coment the following line if you want to make the input method as an
# application

TARGET=pinyinim

qtopia_project(qtopia plugin)

depends(3rdparty/libraries/inputmatch)

HEADERS      = pinyinim.h pinyinimpl.h
SOURCES      = pinyinim.cpp pinyinimpl.cpp
plugin_type  = inputmethods

im.files=$$QTOPIA_DEPOT_PATH/etc/im/pyim/*
im.path=/etc/im/pyim
INSTALLS+=im
