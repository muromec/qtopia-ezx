PROJECTS*=\
    plugins/qtopiacore/kbddrivers/example\
    plugins/qtopiacore/mousedrivers/example \
    plugins/qtopiacore/gfxdrivers/example

enable_modem {
    for(p,PHONEVENDORS) {
        exists(plugins/phonevendors/$$p/$$tail($$p).pro):PROJECTS*=plugins/phonevendors/$$p
    }
    for(m,MULTIPLEXERS) {
        exists(plugins/multiplexers/$$m/$$tail($$m).pro):PROJECTS*=plugins/multiplexers/$$m
    }
}

PROJECTS-= plugins/network
