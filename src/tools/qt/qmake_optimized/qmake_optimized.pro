qtopia_project(desktop external app)
QMAKE=$$QT_DEPOT_PATH/qmake
VPATH+=$$QMAKE
include($$QMAKE/qmake.pro)
# force a release (optimized) build
CONFIG+=release
# fix the include path
tmp=$$INCPATH
INTPATH=
for(p,tmp) {
    exists($$p) {
        INCPATH+=$$p
    }
    !exists($$p):exists($$QMAKE/$$p) {
        INCPATH+=$$QMAKE/$$p
    }
}
INCPATH+=$$DQTDIR/src/corelib/global
# fix the destpath
DESTDIR=$$QPEDIR/bin
