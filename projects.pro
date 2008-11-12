qtopia_project(subdirs)

CONFIG+=ordered

SUBDIRS=src
build_examples:build_qtopia:!enable_singleexec {
    # We only want make to do examples, not make install!
    defineTest(append_examples) {
        qtopia_all.commands+=$$esc(\n\t)@$$MAKE -C examples
        export(qtopia_all.commands)
    }
    runlast(append_examples())
}
build_qtopiadesktop:SUBDIRS+=src/qtopiadesktop

!isEmpty(EXTRA_SUBDIRS):SUBDIRS+=$$EXTRA_SUBDIRS

enable_singleexec:SUBDIRS+=src/server

!win {
    clean_qtopia.commands=$$COMMAND_HEADER\
        env QPEDIR=$$QPEDIR $$QTOPIA_DEPOT_PATH/scripts/clean_qtopia
    QMAKE_EXTRA_TARGETS+=clean_qtopia
    qtopia_distclean.depends+=clean_qtopia
}

# Since people expect 'make install' to do the right thing and since it
# doesn't always do the right thing (it's additive, not replacing), force
# 'make install' to remove the image, making it work like 'make cleaninstall'
win32:RMRF=rmdir /s /q
else:RMRF=rm -rf
cleanimage.commands=$$COMMAND_HEADER
PREFIXES=$(IMAGE) $(DIMAGE)
for(prefix,PREFIXES) {
    !equals(cleanimage.commands,$$COMMAND_HEADER):cleanimage.commands+=$$LINE_SEP
    win32:cleanimage.commands+=if exist $$prefix
    cleanimage.commands+=$$RMRF $$prefix
}
build_qtopia:cleanimage.commands+=$$LINE_SEP_VERBOSE\
    $$QPEDIR/bin/content_installer -clearlocks $(INSTALL_ROOT)/qtopia_db.sqlite
QMAKE_EXTRA_TARGETS+=cleanimage
qtopia_install.depends+=cleanimage

# I'm sure at least one person will need to force an additive install so
# give them a way to do it
runlast(append_install.commands=\$$qtopia_install.commands)
QMAKE_EXTRA_TARGETS+=append_install

!equals(QTOPIA_SDKROOT,$$QPEDIR) {
    # make install implies make sdk
    sdk_inst.commands=$(MAKE) sdk
    QMAKE_EXTRA_TARGETS+=sdk_inst
    qtopia_install.depends+=sdk_inst
    cleanimage.depends+=sdk_inst

    # make sdk implies make cleansdk
    cleansdk.commands=$$COMMAND_HEADER
    win32:cleansdk.commands+=if exist $$prefix
    cleansdk.commands+=$$RMRF $(SDKSANDBOX)$(SDKROOT)
    QMAKE_EXTRA_TARGETS+=cleansdk
    check_sdk.depends+=cleansdk
}

# Don't let common.prf put in the cleaninstall rule. Just make it call our install rule.
CONFIG+=no_cleaninstall
cleaninstall.commands=
cleaninstall.depends=qtopia_install
QMAKE_EXTRA_TARGETS+=cleaninstall
cinstall.commands=
cinstall.depends=qtopia_install
QMAKE_EXTRA_TARGETS+=cinstall

build_qtopia {
    # Output a nice message when make finishes
    qtopia_all_extra_commands+=$$LINE_SEP\
        echo $$LITERAL_QUOTE$$LITERAL_QUOTE $$LINE_SEP\
        echo $${LITERAL_QUOTE}Qtopia has been built.$$LITERAL_QUOTE $$LINE_SEP\
        echo $$LITERAL_QUOTE$$LITERAL_QUOTE $$LINE_SEP\
        echo $${LITERAL_QUOTE}You must now install Qtopia by running $${LITERAL_SQUOTE}make install$${LITERAL_SQUOTE}.$$LITERAL_QUOTE $$LINE_SEP\
        echo $${LITERAL_QUOTE}This will put the files required to run Qtopia into the image:$$LITERAL_QUOTE $$LINE_SEP\
        echo $${LITERAL_QUOTE}$(INSTALL_ROOT)$$LITERAL_QUOTE $$LINE_SEP\
        echo $$LITERAL_QUOTE$$LITERAL_QUOTE
    runlast(qtopia_all.commands+=\$$qtopia_all_extra_commands)

    # Output a nice message when make install finishes
    qtopia_install_extra_commands+=$$LINE_SEP\
        echo $$LITERAL_QUOTE$$LITERAL_QUOTE $$LINE_SEP\
        echo $${LITERAL_QUOTE}Qtopia has been installed.$$LITERAL_QUOTE $$LINE_SEP\
        echo $$LITERAL_QUOTE$$LITERAL_QUOTE $$LINE_SEP\
        echo $${LITERAL_QUOTE}The files required to run Qtopia are in the image:$$LITERAL_QUOTE $$LINE_SEP\
        echo $${LITERAL_QUOTE}$(INSTALL_ROOT)$$LITERAL_QUOTE $$LINE_SEP\
        echo $$LITERAL_QUOTE$$LITERAL_QUOTE $$LINE_SEP\
        if [ "$(INSTALL_ROOT)" = "$$QTOPIA_PREFIX" ]; then\
            echo $${LITERAL_QUOTE}Please note that Qtopia cannot be moved. It must be run from the image.$$LITERAL_QUOTE;\
        else\
            echo $${LITERAL_QUOTE}Please note that Qtopia cannot be run from the image.$$LITERAL_QUOTE;\
            echo $${LITERAL_QUOTE}You must move Qtopia to the prefix first. The prefix is:$$LITERAL_QUOTE;\
            echo $$LITERAL_QUOTE$$QTOPIA_PREFIX$$LITERAL_QUOTE;\
        fi $$LINE_SEP
    !enable_rpath:qtopia_install_extra_commands+=\
        echo $$LITERAL_QUOTE$$LITERAL_QUOTE $$LINE_SEP\
        echo $${LITERAL_QUOTE}Note that you will need to set LD_LIBRARY_PATH=$$QTOPIA_PREFIX/lib to run Qtopia.$$LITERAL_QUOTE $$LINE_SEP
    qtopia_install_extra_commands+=\
        echo $$LITERAL_QUOTE$$LITERAL_QUOTE
    runlast(qtopia_install.commands+=\$$qtopia_install_extra_commands)
    runlast(append_install.commands+=\$$qtopia_install_extra_commands)
    runlast(install_target.commands+=\$$qtopia_install_extra_commands)


    # Let the message output be tested without actually doing a make/make install
    makedone.commands=$$COMMAND_HEADER$$qtopia_all_extra_commands
    QMAKE_EXTRA_TARGETS+=makedone
    makeinstalldone.commands=$$COMMAND_HEADER$$qtopia_install_extra_commands
    QMAKE_EXTRA_TARGETS+=makeinstalldone
}

