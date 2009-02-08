define RULE
@+QPEDIR=$(QTOPIA_BUILD);\
if [ -z "$$QPEDIR" ]; then\
    QPEDIR=$(QTOPIA_BUILD_TREE);\
fi;\
if [ -z "$$QPEDIR" ]; then\
    QPEDIR=$(QPEDIR);\
fi;\
if [ -z "$$QPEDIR" ]; then\
    echo "ERROR: You must set QTOPIA_BUILD if you want to use depot hopping.";\
    echo "When this file was created, the Qtopia build tree was:";\
    echo "    /var/build/200902061601";\
    exit 1;\
fi;\
QPEDIR=`cd $$QPEDIR; /bin/pwd | sed 's/\/$$//'`;\
PWD=`/bin/pwd`;\
shadowdir=`echo $$PWD | perl -ne 's,/home/muromec/qtopia-opensource-src-4.3.4-snapshot-20081021,'$$QPEDIR',;\
                                    print'`;\
if [ "$$shadowdir" != "$$PWD" -a -d "$$shadowdir" ]; then\
    cd "$$shadowdir";\
    $(MAKE) $@;\
else\
    echo "ERROR: Can't resolve shadowdir.";\
    echo "       SOURCE tree = '/home/muromec/qtopia-opensource-src-4.3.4-snapshot-20081021'";\
    echo "       BUILD  tree = '$$QPEDIR'";\
    echo "       PWD         = '$$PWD'";\
    echo "       shadowdir   = '$$shadowdir'";\
    exit 1;\
fi
endef

all:
	$(RULE)
install:
	$(RULE)
install_target:
	$(RULE)
packages:
	$(RULE)
lupdate:
	$(RULE)
sdk:
	$(RULE)
cleaninstall:
	$(RULE)
cinstall:
	$(RULE)
syncqtopia:
	$(RULE)
first_syncqtopia:
	$(RULE)
checkp4:
	$(RULE)
remove_target:
	$(RULE)
relink:
	$(RULE)
printdependrules:
	$(RULE)
qmake_check:
	$(RULE)
test:
	$(RULE)
install_tests:
	$(RULE)
listtests:
	$(RULE)
qtopia_first:
	$(RULE)
first:
	$(RULE)
qtopia_all:
	$(RULE)
qtopia_clean:
	$(RULE)
clean:
	$(RULE)
qtopia_distclean:
	$(RULE)
distclean:
	$(RULE)
qtopia_install:
	$(RULE)
dist:
	$(RULE)
link_test:
	$(RULE)
mocclean:
	$(RULE)
mocables:
	$(RULE)
check_sdk:
	$(RULE)
package_qpe-madplugin_dummy:
	$(RULE)
compiler_moc_header_make_all:
	$(RULE)
compiler_moc_header_clean:
	$(RULE)
compiler_rcc_make_all:
	$(RULE)
compiler_rcc_clean:
	$(RULE)
compiler_image_collection_make_all:
	$(RULE)
compiler_image_collection_clean:
	$(RULE)
compiler_moc_source_make_all:
	$(RULE)
compiler_moc_source_clean:
	$(RULE)
compiler_uic_make_all:
	$(RULE)
compiler_uic_clean:
	$(RULE)
compiler_yacc_decl_make_all:
	$(RULE)
compiler_yacc_decl_clean:
	$(RULE)
compiler_yacc_impl_make_all:
	$(RULE)
compiler_yacc_impl_clean:
	$(RULE)
compiler_lex_make_all:
	$(RULE)
compiler_lex_clean:
	$(RULE)
compiler_runcheckp4_make_all:
	$(RULE)
dummy_checkp4:
	$(RULE)
compiler_clean:
	$(RULE)
uninstall_target:
	$(RULE)
qmake:
	$(RULE)
qmake-debug:
	$(RULE)
regenerate:
	$(RULE)
regenerate_all:
	$(RULE)
nodeps:
	$(RULE)
loud:
	$(RULE)
silent:
	$(RULE)
.PHONY: all install install_target packages lupdate sdk cleaninstall cinstall syncqtopia first_syncqtopia checkp4 remove_target relink printdependrules qmake_check test install_tests listtests qtopia_first first qtopia_all qtopia_clean clean qtopia_distclean distclean qtopia_install dist link_test mocclean mocables check_sdk package_qpe-madplugin_dummy compiler_moc_header_make_all compiler_moc_header_clean compiler_rcc_make_all compiler_rcc_clean compiler_image_collection_make_all compiler_image_collection_clean compiler_moc_source_make_all compiler_moc_source_clean compiler_uic_make_all compiler_uic_clean compiler_yacc_decl_make_all compiler_yacc_decl_clean compiler_yacc_impl_make_all compiler_yacc_impl_clean compiler_lex_make_all compiler_lex_clean compiler_runcheckp4_make_all dummy_checkp4 compiler_clean uninstall_target qmake qmake-debug regenerate regenerate_all nodeps loud silent
