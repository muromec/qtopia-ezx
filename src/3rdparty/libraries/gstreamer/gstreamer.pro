qtopia_project(stub)

requires(exists($$QPEDIR/config.tests/mediaengines/gstreamer.pri))
include($$QPEDIR/config.tests/mediaengines/gstreamer.pri)

idep(QMAKE_CFLAGS+=$$GSTREAMER_CFLAGS)
idep(QMAKE_CXXFLAGS+=$$GSTREAMER_CFLAGS)
idep(LIBS+=$$GSTREAMER_LIBS)

license(LGPL)
