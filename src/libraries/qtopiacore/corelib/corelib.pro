qtopia_project(qtopiacore)
QTOPIACORE_CONFIG=lib
qt=target
dir=src/$$TARGET
!enable_singleexec {
    ipatchqt.commands=$$COMMAND_HEADER\
        $$fixpath($$QBS_BIN/patchqt) $$fixpath($(INSTALL_ROOT)/lib/libQtCore.so) $$QTOPIA_PREFIX
    ipatchqt.CONFIG=no_path
    ipatchqt.depends=install_qtcore
    INSTALLS+=ipatchqt
}

depends(libraries/qtopiacore/tools/moc)
depends(libraries/qtopiacore/tools/uic)
depends(libraries/qtopiacore/tools/rcc)
unix:depends(3rdparty/libraries/pthread)
