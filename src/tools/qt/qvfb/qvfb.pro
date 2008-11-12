qtopia_project(desktop app)
TARGET=qvfb
CONFIG+=no_tr

VPATH+=$$QT_DEPOT_PATH/tools/qvfb $$QT_DEPOT_PATH/src/gui/embedded
INCLUDEPATH+=$$QT_DEPOT_PATH/tools/qvfb $$QT_DEPOT_PATH/src/gui/embedded

fetch_vars_from_file($$QT_DEPOT_PATH/tools/qvfb/qvfb.pro,QVFB,FORMS,HEADERS,SOURCES,RESOURCES,INCLUDEPATH)
FORMS=$$QVFB.FORMS
HEADERS=$$QVFB.HEADERS
SOURCES=$$QVFB.SOURCES
RESOURCES=$$QVFB.RESOURCES
INCLUDEPATH*=$$QVFB.INCLUDEPATH
LIBS+=-lXtst

commands=
skinfiles=$$files($$QT_DEPOT_PATH/tools/qvfb/*.skin)
for(s,skinfiles) {
    #!exists($$s/$$tail($$s)):next()
    !isEmpty(commands):commands+=$$LINE_SEP_VERBOSE
    commands+=\
        rm -f $$QPEDIR/src/tools/qt/qvfb/$$tail($$s) $$LINE_SEP_VERBOSE\
        ln -s $$s $$QPEDIR/src/tools/qt/qvfb
}
devicedirs=$$files($$QTOPIA_DEPOT_PATH/devices/*)
for(d,devicedirs) {
    skinfiles=$$files($$d/*.skin)
    for(s,skinfiles) {
        !exists($$s/$$tail($$s)):next()
        !isEmpty(commands):commands+=$$LINE_SEP_VERBOSE
        commands+=\
            rm -f $$QPEDIR/src/tools/qt/qvfb/$$tail($$s) $$LINE_SEP_VERBOSE\
            ln -s $$s $$QPEDIR/src/tools/qt/qvfb
    }
}
symlink_skins.commands=$$commands
QMAKE_EXTRA_TARGETS+=symlink_skins
ALL_DEPS+=symlink_skins

