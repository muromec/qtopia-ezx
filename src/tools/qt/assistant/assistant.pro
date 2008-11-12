qtopia_project(qtopiacore)
QTOPIACORE_CONFIG=app rpath ignore_errors no_symlink
qt=host
dir=tools/$$TARGET

unix:build_dqt {
    link_qtopia_docs.commands=$$COMMAND_HEADER\
        [ -e $$QPEDIR/doc/html/qtopia.dcf ] && $$DQTDIR/bin/assistant -addContentFile $$QPEDIR/doc/html/qtopia.dcf; true
    equals(QTOPIA_SDKROOT,$$QPEDIR) {
        link_qtopia_docs.depends+=redirect_all
        QMAKE_EXTRA_TARGETS+=link_qtopia_docs
        ALL_DEPS+=link_qtopia_docs
    } else {
        link_qtopia_docs.CONFIG=no_path
        link_qtopia_docs.hint=sdk
        INSTALLS+=link_qtopia_docs
    }
}

depends(libraries/qt/xml)
depends(libraries/qt/gui)
depends(libraries/qt/network)
depends(libraries/qt/corelib)
