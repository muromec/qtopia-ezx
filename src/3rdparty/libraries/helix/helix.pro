qtopia_project(stub)
CONFIG+=syncqtopia

# The actual helix build tree is located somewhere else so redirect the commands
HELIX_PATH=$$QPEDIR/src/3rdparty/libraries/helix/helixbuild

depends(libraries/qtopia)

CONFIG(release,debug|release) {
    release=-t release
    LOGGING=no
    BUILDPART=splay_nodist_qtopia 
}
else {
    release=-t debug
    LOGGING=yes
    BUILDPART=splay_nodist_qtopia_log
}

enable_rpath:!isEmpty(QTOPIA_RPATH):RPATHVALUE=$$QTOPIA_RPATH$$QTOPIA_PREFIX

# setup the helix bulid tree
# this code is copied into configure
setup_helixbuild.commands=$$COMMAND_HEADER\
    if [ -d $$HELIX_PATH ]; then rm -rf $$HELIX_PATH; fi $$LINE_SEP\
    mkdir -p $$HELIX_PATH $$LINE_SEP\
    cp -aRpf $$PWD/src/* $$HELIX_PATH $$LINE_SEP\
    cp -aRpf $$PWD/trolltech/src/* $$HELIX_PATH $$LINE_SEP\
    # chmod this so that we can modify the files
    chmod -R u+w $$HELIX_PATH $$LINE_SEP\
    $$PWD/helixconf $$LITERAL_QUOTE$$HELIX_CONFIG$$LITERAL_QUOTE\
        $$HELIX_PATH/build/umakepf/helix-client-qtopia-nodist.pf\
        $$LITERAL_QUOTE$$QTEDIR$$LITERAL_QUOTE\
        $$LITERAL_QUOTE$$QPEDIR$$LITERAL_QUOTE\
        $$LITERAL_QUOTE$$RPATHVALUE$$LITERAL_QUOTE\
        $$LITERAL_QUOTE$$HELIX_PATH/buildrc$$LITERAL_QUOTE\
        $$LITERAL_QUOTE$$LOGGING$$LITERAL_QUOTE $$LINE_SEP\
    cd $$HELIX_PATH;\
    if [ -d $$PWD/trolltech/patches ]; then\
        for patch in $$PWD/trolltech/patches/*.patch; do\
            patch -g0 -t -p2 <\$$patch;\
        done;\
    fi
QMAKE_EXTRA_TARGETS+=setup_helixbuild
regenerate.depends+=setup_helixbuild

# setup the helix bulid system
setup_helix.commands=$$COMMAND_HEADER\
    echo $${LITERAL_QUOTE}Setting up the helix build system. To see the output run:$${LITERAL_QUOTE}$$LINE_SEP\
    echo $${LITERAL_QUOTE} make setup_helix HELIX_OUTPUT=1$${LITERAL_QUOTE}$$LINE_SEP\
    if [ ! -d $$HELIX_PATH ]; then\
        $$MAKE setup_helixbuild;\
    fi $$LINE_SEP\
    # remove this so that we actually file if we can't do the setup step
    rm -f $$HELIX_PATH/Makefile $$LINE_SEP\
    # If there are CVS directories, helix tries to fetch stuff from CVS
    find $$HELIX_PATH -type d -name CVS | xargs rm -rf $$LINE_SEP\
    cd $$HELIX_PATH;\
    output=$$LITERAL_QUOTE>/dev/null 2>&1$$LITERAL_QUOTE;\
    helix_output=$(HELIX_OUTPUT);\
    if [ $$LITERAL_QUOTE\$$helix_output$$LITERAL_QUOTE = 1 ]; then\
        output=;\
    fi;\
    eval $$ENV BUILD_ROOT=$$LITERAL_QUOTE$$HELIX_PATH/build$$LITERAL_QUOTE\
          PATH=$$LITERAL_QUOTE\$$PATH:\$$BUILD_ROOT/bin$$LITERAL_QUOTE\
          BUILDRC=$$LITERAL_QUOTE$$HELIX_PATH/buildrc$$LITERAL_QUOTE\
          SYSTEM_ID=$$HELIX_SYSTEM_ID \
    # we have to ignore errors from this script because it always give us an error
    python build/bin/build.py $$release -U -P helix-client-qtopia-nodist $$BUILDPART \$$output || true $$LINE_SEP\
    if [ ! -f $$HELIX_PATH/Makefile ]; then\
        echo $${LITERAL_QUOTE}ERROR: Helix build system failure.$${LITERAL_QUOTE};\
        exit 1;\
    fi;\
    for file in \$$(find $$HELIX_PATH -name Makefile); do\
        mv -f \$$file \$$file.bak;\
        cat \$$file.bak\
            # Don't let the Makefiles set MAKE (it breaks our -j support)
            | grep -v $$LITERAL_SQUOTE^MAKE=$$LITERAL_SQUOTE\
            # Make the directory changes quiet
            | sed $${LITERAL_SQUOTE}s/^$$esc(\t)cd/$$esc(\t)@cd/$$LITERAL_SQUOTE\
                >\$$file;\
        rm -f \$$file.bak;\
    done
QMAKE_EXTRA_TARGETS+=setup_helix

# setup the "make" command
redirect_all.commands=$$COMMAND_HEADER\
    # setup the build system if our Makefile is newer or if their Makefile doesn't exist
    if [ ! -e $$HELIX_PATH/Makefile -o $$OUT_PWD/Makefile -nt $$HELIX_PATH/Makefile ]; then\
        $$MAKE setup_helix;\
    fi $$LINE_SEP\
    cd $$HELIX_PATH;\
    eval $$ENV BUILD_ROOT=$$LITERAL_QUOTE$$HELIX_PATH/build$$LITERAL_QUOTE\
          PATH=$$LITERAL_QUOTE\$$PATH:\$$BUILD_ROOT/bin$$LITERAL_QUOTE\
          BUILDRC=$$LITERAL_QUOTE$$HELIX_PATH/buildrc$$LITERAL_QUOTE\
          SYSTEM_ID=$$HELIX_SYSTEM_ID \
    # we have to ignore errors from this script because it always give us an error
    python build/bin/build.py $$release -P helix-client-qtopia-nodist $$BUILDPART $$LINE_SEP
QMAKE_EXTRA_TARGETS+=redirect_all
ALL_DEPS+=redirect_all

# setup the "make clean" command
redirect_clean.commands=$$COMMAND_HEADER\
    $$MAKE -C $$HELIX_PATH clean
QMAKE_EXTRA_TARGETS+=redirect_clean

# setup the "make distclean" command
redirect_distclean.commands=\
    rm -rf $$HELIX_PATH
QMAKE_EXTRA_TARGETS+=redirect_distclean

CONFIG(release,debug|release):debug=release
else:debug=debug

# install some libs and binaries at "make install" time
install_libs.commands=$$COMMAND_HEADER\
    mkdir -p $(INSTALL_ROOT)/lib/helix $$LINE_SEP\
    mkdir -p $(INSTALL_ROOT)/bin $$LINE_SEP\
    for file in $$HELIX_PATH/$$debug/*.so*; do\
        echo cp -afp \$$file $(INSTALL_ROOT)/lib/helix;\
        cp -afp \$$file $(INSTALL_ROOT)/lib/helix;
CONFIG(release,debug|release):!isEmpty(QMAKE_STRIP):install_libs.commands+=\
        echo $$QMAKE_STRIP $$QMAKE_STRIPFLAGS_LIB $(INSTALL_ROOT)/lib/helix/\$$(basename \$$file);\
        $$QMAKE_STRIP $$QMAKE_STRIPFLAGS_LIB $(INSTALL_ROOT)/lib/helix/\$$(basename \$$file);
install_libs.commands+=\
    done $$LINE_SEP_VERBOSE\
    cp -R $$HELIX_PATH/$$debug/splay $(INSTALL_ROOT)/bin
CONFIG(release,debug|release):!isEmpty(QMAKE_STRIP):install_libs.commands+=\
    $$LINE_SEP_VERBOSE $$QMAKE_STRIP $(INSTALL_ROOT)/bin/splay
install_libs.CONFIG=no_path
INSTALLS+=install_libs

idep(INCLUDEPATH+=\
    $$HELIX_PATH/common/include\
    $$HELIX_PATH/common/dbgtool/pub\
    $$HELIX_PATH/common/runtime/pub\
    $$HELIX_PATH/common/util/pub\
    $$HELIX_PATH/client/include\
    $$HELIX_PATH/video/include\
    $$HELIX_PATH/client/videosvc/pub\
    $$HELIX_PATH/video/colconverter/pub)

