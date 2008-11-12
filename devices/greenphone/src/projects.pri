PROJECTS*=\
    devtools\
    devtools/apm\
    devtools/chvol\
    plugins/qtopiacore/kbddrivers/greenphone \
    plugins/audiohardware/greenphone

enable_modem {
    PROJECTS*=\
        devtools/fixbdaddr

    for(p,PHONEVENDORS) {
        exists(plugins/phonevendors/$$p/$$tail($$p).pro):PROJECTS*=plugins/phonevendors/$$p
    }
    for(m,MULTIPLEXERS) {
        exists(plugins/multiplexers/$$m/$$tail($$m).pro):PROJECTS*=plugins/multiplexers/$$m
    }
}

enable_greenphone_effects {
    PROJECTS *= \
        plugins/qtopiacore/gfxdrivers/greenphone
}
