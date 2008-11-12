# Install alternate network scripts
!isEmpty(variable) {
    exists($$variable):bin.files=$$variable
    else {
        bin.files=$${script}.$$variable
        !exists($$bin.files):error("ERROR: Coule not locate $$variable or $${bin.files}.")
    }
} else {
    bin.files=$$script
}
bin.path=/bin
bin.hint=script
INSTALLS+=bin

!isEmpty(variable) {
    fixbin.commands=$$COMMAND_HEADER\
        rm -f $(INSTALL_ROOT)$$bin.path/$$tail($$script) $$LINE_SEP\
        mv $(INSTALL_ROOT)$$bin.path/$$tail($$bin.files) $(INSTALL_ROOT)$$bin.path/$$tail($$script)
    fixbin.CONFIG=no_path
    fixbin.depends=install_bin
    INSTALLS+=fixbin
}

