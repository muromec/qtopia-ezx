PROJECTS*=\
    devtools\
    plugins/qtopiacore/kbddrivers/c3200\
    plugins/qtopiacore/mousedrivers/c3200\
    plugins/audiohardware/c3200

enable_modem {
    for(p,PHONEVENDORS) {
        exists(plugins/phonevendors/$$p/$$tail($$p).pro):PROJECTS*=plugins/phonevendors/$$p
    }
    for(m,MULTIPLEXERS) {
        exists(plugins/multiplexers/$$m/$$tail($$m).pro):PROJECTS*=plugins/multiplexers/$$m
    }
}
