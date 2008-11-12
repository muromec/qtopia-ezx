qtopia_project(stub)

QMAKE_STRIP=

defineReplace(maketarget) {
    target=$$1
    target~=s,/,_,g
    target~=s,\.\._,,g
    return($$target)
}

# Depend on everything so we can guarantee that this directory is processed last
for(p,PROJECTS) {
    depends($$p,fake)
}

# Stuff for the SDK
unix {
    # Qtopia files
    bins.files=\
        $$QPEDIR/bin/checktrans\
        $$QPEDIR/bin/phonesim\
        $$QPEDIR/bin/pngscale\
        $$QPEDIR/bin/qdawggen\
        $$QPEDIR/bin/content_installer\
        $$QPEDIR/bin/sxe_installer\
        $$QPEDIR/bin/qvfb\
        $$QTOPIA_DEPOT_PATH/bin/assistant\
        $$QTOPIA_DEPOT_PATH/bin/qtopiamake\
        $$QTOPIA_DEPOT_PATH/bin/runqtopia\
        $$QTOPIA_DEPOT_PATH/bin/dumpsec.pl\
        $$QTOPIA_DEPOT_PATH/bin/mkPackages
    bins.path=/bin
    bins.hint=sdk
    INSTALLS+=bins

    phonesim.files=\
        $$QTOPIA_DEPOT_PATH/src/tools/phonesim/*.xml
    phonesim.path=/src/tools/phonesim
    phonesim.hint=sdk
    INSTALLS+=phonesim

    build.files=\
        $$QTOPIA_DEPOT_PATH/src/build
    build.path=/src
    build.hint=sdk
    INSTALLS+=build

    qtopialibs.commands=$$COMMAND_HEADER\
        for file in $$QPEDIR/lib/*; do\
            cp -aRpf \$$file $(INSTALL_ROOT)/lib;\
        done
    qtopialibs.path=/lib
    qtopialibs.hint=sdk
    INSTALLS+=qtopialibs

    qtopiahostlibs.commands=$$COMMAND_HEADER\
        for file in $$QPEDIR/lib/host/*; do\
            if [ $${LITERAL_QUOTE}\$$file$${LITERAL_QUOTE} != $${LITERAL_QUOTE}$$QPEDIR/lib/host/*$${LITERAL_QUOTE} ]; then\
                cp -aRpf \$$file $(INSTALL_ROOT)/lib/host;\
            fi;\
        done
    qtopiahostlibs.path=/lib/host
    qtopiahostlibs.hint=sdk
    INSTALLS+=qtopiahostlibs

    qtopiadocs.commands=$$COMMAND_HEADER
    qtopia_depot:qtopiadocs.commands+=$$QPEDIR/scripts/mkdocs -qpedir $$QPEDIR -force $$LINE_SEP
    qtopiadocs.commands+=cp -Rf $$QPEDIR/doc/html $(INSTALL_ROOT)/doc
    qtopiadocs.path=/doc
    qtopiadocs.hint=sdk
    INSTALLS+=qtopiadocs

    for(pr,PROJECT_ROOTS) {
        alt=$$pr
        alt~=s,$$QTOPIA_DEPOT_PATH,$$QPEDIR,q
        offset=$$pr
        offset~=s,$$QTOPIA_DEPOT_PATH,,q
        isEmpty(offset):offset=/
        !equals(offset,$$pr) {
            target=root$$maketarget($$offset)
            files=\
                $$alt/.qmake.cache\
                $$pr/tree_config.pri\
                $$pr/features
            eval($${target}.files=\$$files)
            eval($${target}.path=\$$offset)
            eval($${target}.hint=sdk)
            INSTALLS+=$$target
        }
    }

    for(pr,PROJECTS) {
        target=proj_$$maketarget($$pr)
        files=$$QTOPIA_DEPOT_PATH/src/$$pr/$$tail($$pr).pro
        !exists($$files):next()
        path=/src/$$pr
        eval($${target}.files=\$$files)
        eval($${target}.path=\$$path)
        eval($${target}.hint=sdk)
        INSTALLS+=$$target
    }

    # Qt files
    dqtbins.files=\
        $$DQTDIR/bin/qmake\
        $$DQTDIR/bin/moc\
        $$DQTDIR/bin/uic\
        $$DQTDIR/bin/rcc\
        $$DQTDIR/bin/assistant\
        $$DQTDIR/bin/designer\
        $$DQTDIR/bin/linguist\
        $$DQTDIR/bin/lrelease\
        $$DQTDIR/bin/lupdate
    dqtbins.path=/qtopiacore/host/bin
    dqtbins.hint=sdk
    INSTALLS+=dqtbins

    qtebins.files=\
        $$QTEDIR/bin/qmake\
        $$QTEDIR/bin/moc\
        $$QTEDIR/bin/uic\
        $$QTEDIR/bin/rcc
    qtebins.path=/qtopiacore/target/bin
    qtebins.hint=sdk
    INSTALLS+=qtebins

    dqtbinsyms.commands=$$COMMAND_HEADER\
        # Symlink these binaries somewhere useful ($QPEDIR/bin)
        for file in designer linguist lrelease lupdate; do\
            ln -sf $$QTOPIA_SDKROOT/qtopiacore/host/bin/\$$file $(INSTALL_ROOT)/bin/\$$file;\
        done
    dqtbinsyms.path=/bin
    dqtbinsyms.hint=sdk
    INSTALLS+=dqtbinsyms

    mkspecs.files=$$QT_DEPOT_PATH/mkspecs
    mkspecs.path=/qtopiacore/qt
    mkspecs.hint=sdk
    INSTALLS+=mkspecs

    mkspecs_host_qconfig.files=$$DQTDIR/mkspecs/qconfig.pri
    mkspecs_host_qconfig.path=/qtopiacore/host/mkspecs
    mkspecs_host_qconfig.hint=sdk
    INSTALLS+=mkspecs_host_qconfig

    mkspecs_target_qconfig.files=$$QTEDIR/mkspecs/qconfig.pri
    mkspecs_target_qconfig.path=/qtopiacore/target/mkspecs
    mkspecs_target_qconfig.hint=sdk
    INSTALLS+=mkspecs_target_qconfig

    mkspecs_symlinks.commands=$$COMMAND_HEADER\
        ln -sf $$QTOPIA_SDKROOT/qtopiacore/qt/mkspecs/* $(INSTALL_ROOT)/qtopiacore/host/mkspecs $$LINE_SEP\
        ln -sf $$QTOPIA_SDKROOT/qtopiacore/qt/mkspecs/* $(INSTALL_ROOT)/qtopiacore/target/mkspecs
    !isEmpty(PLATFORM_SDK):mkspecs_symlinks.commands+=$$LINE_SEP\
        ln -sf $$QTOPIA_SDKROOT$$PLATFORM_SDK $(INSTALL_ROOT)/qtopiacore/host/mkspecs/default
    !isEmpty(XPLATFORM_SDK):mkspecs_symlinks.commands+=$$LINE_SEP\
        ln -sf $$QTOPIA_SDKROOT$$XPLATFORM_SDK $(INSTALL_ROOT)/qtopiacore/target/mkspecs/default
    mkspecs_symlinks.CONFIG=no_path
    mkspecs_symlinks.hint=sdk
    mkspecs_symlinks.depends+=install_mkspecs install_mkspecs_host_qconfig install_mkspecs_target_qconfig
    INSTALLS+=mkspecs_symlinks

    qvfbskins.path=/src/tools/qt/qvfb
    qvfbskins.commands=$$COMMAND_HEADER\
        cp -RL $$QPEDIR/src/tools/qt/qvfb/*.skin $(INSTALL_ROOT)$$qvfbskins.path
    qvfbskins.hint=sdk
    INSTALLS+=qvfbskins

    dqt_qmakecache.files=$$DQTDIR/.qmake.cache
    dqt_qmakecache.path=/qtopiacore/host
    dqt_qmakecache.hint=sdk
    INSTALLS+=dqt_qmakecache

    qte_qmakecache.files=$$QTEDIR/.qmake.cache
    qte_qmakecache.path=/qtopiacore/target
    qte_qmakecache.hint=sdk
    INSTALLS+=qte_qmakecache

    qtdocs.path=/qtopiacore/qt/doc
    qtdocs.commands=$$COMMAND_HEADER\
        cp -Rf $$QT_DEPOT_PATH/doc/html $(INSTALL_ROOT)/$$qtdocs.path
    qtdocs.hint=sdk
    qtdocs.depends=install_qtopiadocs
    INSTALLS+=qtdocs

    qconfig.files=$$QTOPIA_DEPOT_PATH/qtopiacore/qconfig-qpe.h
    qconfig.path=/qtopiacore
    qconfig.hint=sdk
    INSTALLS+=qconfig

    qtdocsyms.commands=$$COMMAND_HEADER\
        mkdir -p $(INSTALL_ROOT)/qtopiacore/target $$LINE_SEP\
        ln -sf $$QTOPIA_SDKROOT/qtopiacore/qt/doc $(INSTALL_ROOT)/qtopiacore/target/doc $$LINE_SEP\
        mkdir -p $(INSTALL_ROOT)/qtopiacore/host $$LINE_SEP\
        ln -sf $$QTOPIA_SDKROOT/qtopiacore/qt/doc $(INSTALL_ROOT)/qtopiacore/host/doc
    qtdocsyms.CONFIG=no_path
    qtdocsyms.hint=sdk
    INSTALLS+=qtdocsyms

    co=sdk
    phone:co+=phone
    else:pda:co+=pda
    configureoptions.commands=$$COMMAND_HEADER\
        echo $$co >$(INSTALL_ROOT)/.configureoptions
    configureoptions.CONFIG=no_path
    configureoptions.hint=sdk
    INSTALLS+=configureoptions

    # default device directory
    device.files=$$QTOPIA_DEPOT_PATH/devices/default
    device.path=/devices
    device.hint=sdk
    INSTALLS+=device

    # current device directory
    !isEmpty(DEVICE_CONFIG_PATH):device.files+=$$DEVICE_CONFIG_PATH

    # fix up the config.cache file and then generate a new config.pri
    configpri.commands=$$COMMAND_HEADER\
        echo Fixing paths $$LINE_SEP\
        $$QPEDIR/src/build/bin/sdkcache $$QTOPIA_SDKROOT $(INSTALL_ROOT) $$LINE_SEP\
        $(INSTALL_ROOT)/src/build/bin/write_config_pri -sdk $(INSTALL_ROOT)
    configpri.path=/src
    configpri.hint=sdk
    configpri.depends=
    # Go last or we clobber the real config.pri!
    for(i,INSTALLS) {
        configpri.depends+=install_$$i
    }
    INSTALLS+=configpri
}

listcomponents.commands=$$COMMAND_HEADER\
    echo PROJECTS = $$PROJECTS $$LINE_SEP\
    echo THEMES = $$THEMES
QMAKE_EXTRA_TARGETS+=listcomponents

enable_dbusipc {
    convert_services.commands=$$QPEDIR/bin/convert_services_to_dbus $(INSTALL_ROOT)
    convert_services.CONFIG=no_path
    INSTALLS+=convert_services
}
