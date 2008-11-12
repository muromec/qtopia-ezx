qtopia_project(qtopiacore)
QTOPIACORE_CONFIG=subdirs qt_plugins
qt=target
dir=src/plugins
depends(libraries/qtopiacore/*)
EXTRA_TARGETS=singleexec_pri
enable_singleexec {
    singleexec_pri_commands=$$LINE_SEP\
        :> $$OUT_PWD/singleexec.pri $$LINE_SEP\
        $(MAKE) singleexec_pri
    runlast(redirect_all.commands+=\$$singleexec_pri_commands)
    runlast(redirect_install.depends-=redirect_all)
}
