qtopia_project(stub)
license(COMMERCIAL)
requires(exists($$QPEDIR/config.tests/locate_drmagent.pri))
include($$QPEDIR/config.tests/locate_drmagent.pri)
idep(LIBS+=$$DRMAGENT)
install_lib.commands=\
    cp -aRp $$DRMAGENT* $(INSTALL_ROOT)/lib
CONFIG(release,debug|release):!isEmpty(QMAKE_STRIP):install_lib.commands+=\
    $$LINE_SEP_VERBOSE $$QMAKE_STRIP\
        $$QMAKE_STRIPFLAGS_LIB\
        $$LITERAL_QUOTE$(INSTALL_ROOT)$$libdir/$$tail($$DRMAGENT)$$LITERAL_QUOTE
install_lib.path=/lib
INSTALLS+=install_lib
