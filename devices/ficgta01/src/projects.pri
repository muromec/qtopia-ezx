PROJECTS*=\
    plugins/audiohardware/ficgta01\
	devtools 

!x11 {
	PROJECTS*= plugins/qtopiacore/kbddrivers/ficgta01
}
	 
enable_modem {
    for(p,PHONEVENDORS) {
        exists(plugins/phonevendors/$$p/$$tail($$p).pro):PROJECTS*=plugins/phonevendors/$$p
    }
    for(m,MULTIPLEXERS) {
        exists(plugins/multiplexers/$$m/$$tail($$m).pro):PROJECTS*=plugins/multiplexers/$$m
    }
}


