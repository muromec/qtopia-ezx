# Setup the PROJECTS variables

# Global stuff
PROJECTS*=\
    tools/pngscale\
    tools/svgtopicture

# Qt files
DQT_PROJECTS=\
    libraries/qt/tools/bootstrap\
    libraries/qt/tools/moc\
    libraries/qt/tools/uic\
    libraries/qt/tools/rcc\
    libraries/qt/corelib\
    libraries/qt/gui\
    libraries/qt/network\
    libraries/qt/sql\
    libraries/qt/xml\
    libraries/qt/svg\
    plugins/qt\
    tools/qt/lupdate\
    tools/qt/lrelease\
    tools/qt/assistant\
    tools/qt/assistant/lib\
    tools/qt/linguist\
    tools/qt/designer/src/uitools\
    tools/qt/designer/src/lib\
    tools/qt/designer/src/components\
    tools/qt/designer/src/designer\
    tools/qt/designer/src/plugins
!equals(DQT_MINOR_VERSION,2):DQT_PROJECTS+=libraries/qt/script
!win32:DQT_PROJECTS+=\
    tools/qt/qvfb\
    libraries/qt/qt3support
contains(DQT_CONFIG,opengl):DQT_PROJECTS+=libraries/qt/opengl
qtopia_depot:DQT_PROJECTS*=tools/qt/qdoc3
win32:DQT_PROJECTS*=\
    libraries/qt/winmain
PROJECTS*=$$DQT_PROJECTS
# When building Qtopia and skipping Qt, build QVFb anyway
build_qtopia:!build_dqt:DQT_PROJECTS-=tools/qt/qvfb

build_qtopia {
    # Qtopia Core files
    QTE_PROJECTS=\
	libraries/qtopiacore/tools/bootstrap\
        libraries/qtopiacore/tools/moc\
        libraries/qtopiacore/tools/uic\
        libraries/qtopiacore/tools/rcc\
        libraries/qtopiacore/corelib\
        libraries/qtopiacore/gui\
        libraries/qtopiacore/network\
        libraries/qtopiacore/sql\
        libraries/qtopiacore/xml\
        libraries/qtopiacore/script\
        libraries/qtopiacore/svg\
	libraries/qtopiacore/webkit\
        plugins/qtopiacore
    contains(QTE_CONFIG,opengl):QTE_PROJECTS+=libraries/qtopiacore/opengl
    PROJECTS*=$$QTE_PROJECTS

    !no_general_pri:include(general.pri)
    # Load a device-specific file (if it exists)
    !isEmpty(DEVICE_CONFIG_PATH):exists($$DEVICE_CONFIG_PATH/projects.pri):include($$DEVICE_CONFIG_PATH/projects.pri)
    !isEmpty(DEVICE_CONFIG_PATH):exists($$DEVICE_CONFIG_PATH/custom.pri):include($$DEVICE_CONFIG_PATH/custom.pri)
    else:include(custom.pri)
    include($$QPEDIR/src/local.pri)
} else {
    PROJECTS*=\
        3rdparty/libraries/pthread
}

