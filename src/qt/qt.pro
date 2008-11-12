qtopia_project(stub)

# Fonts...
fonts.files=$$QTOPIA_FONTS
fonts.path=/lib/fonts
INSTALLS+=fonts

fontdir.commands=$$COMMAND_HEADER\
    :>$(INSTALL_ROOT)/lib/fonts/fontdir
fontdir.path=/lib/fonts
INSTALLS+=fontdir

pkg.name=qt-embedded
pkg.desc=Qt/Embedded
pkg.version=$$QTE_VERSION
pkg.domain=trusted

!disable_qt_lupdate {
    #
    # We want to maintain our own translations of Qt.
    #
    # This could be done more simply with a find command
    # but that would not be cross-platform.
    #

    # Only do the libs we have
    QCLIBS=
    for(p,PROJECTS) {
        containstext($$p,libraries/qtopiacore/):QCLIBS+=$$tail($$p)
    }
    QTLIBS=
    for(p,PROJECTS) {
        # Don't do libQt3Support because we don't use it
        containstext($$p,libraries/qt/):!containstext($$p,qt3support):QTLIBS+=$$tail($$p)
    }

    debug=$$(QMAKE_DEBUG_ON)
    exists($$QPEDIR/src/build/debug_on):debug=1
    equals(debug,1) {
        message(Qtopia Core projects:)
        for(f,QCLIBS):message(- $$f)
        message(Qt projects:)
        for(f,QTLIBS):message(- $$f)
    }

    # We need to recurse directories so a function is used
    defineTest(getAllTranslatables) {
        for(cwd,TO_PROCESS) {
            contains(PROCESSED_DIRS,$$cwd):next()
            echo(Searching $$cwd)
            PROCESSED_DIRS+=$$cwd

            FILES=$$files($$cwd/*)
            for(f,FILES) {
                containsre($$f,\.(cpp|h|ui)$):TRANSLATABLES*=$$fixpath($$f)
                # Check for directories (ie. something with more files in it)
                subfiles=$$files($$f/*)
                !isEmpty(subfiles) {
                    TO_PROCESS*=$$fixpath($$f)
                }
            }
        }

        export(PROCESSED_DIRS)
        export(TO_PROCESS)
        export(TRANSLATABLES)
    }

    # Since QTE_DEPOT_PATH and DQT_DEPOT_PATH should be the same use *= so we only do each dir once.
    # If QTE_DEPOT_PATH and DQT_DEPOT_PATH are different, then two sets of translations will be
    # stored in the one .ts file.
    PROCESSED_DIRS=
    TO_PROCESS=
    for(dir,QCLIBS) {
        TO_PROCESS*=$$fixpath($$QTE_DEPOT_PATH/src/$$dir)
    }
    for(dir,QTLIBS) {
        TO_PROCESS*=$$fixpath($$DQT_DEPOT_PATH/src/$$dir)
    }

    TRANSLATABLES=
    getAllTranslatables()
    for(l,forever) {
        equals(PROCESSED_DIRS,$$TO_PROCESS):break()
        getAllTranslatables()
    }

    equals(debug,1) {
        message(Translatables:)
        for(f,TRANSLATABLES):message(- $$f)
    }

    # We have TRANSLATABLES now so we can enable the i18n feature (it does lupdate/lrelease for us)
    !isEmpty(TRANSLATIONS):CONFIG*=i18n
    CONFIG-=no_tr
    TRTARGET=qt
}

