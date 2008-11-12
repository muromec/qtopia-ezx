qtopia_project(qtopia app)
TARGET=qpe

CONFIG+=enable_phone_ui

!enable_singleexec {
    # This allows plugins to access symbols in the server.
    # qtopiatest uses this.
    QMAKE_LFLAGS*=-Wl,--export-dynamic
}
enable_singleexec:qtopiatest {
    CONFIG += qtestlib
}
# CONFIG += enable_tests

# DEFINES += QTOPIA_PHONEUI

#this ensures that include path remain the same
INCLUDEPATH+= core_server \
              memory/base \
              infrastructure/camera \
              infrastructure/softmenubar \
              processctrl/appmonitor \
              processctrl/startup \
              processctrl/taskmanagerentry \
              processctrl/terminationhandler \
              ui/abstractinterfaces/stdmessagebox \
              ui/components/delayedwaitdialog \
              ui/components/inputmethods \
              ui/components/touchscreenlockdlg\
              ui/launcherviews/base \
              ui/launcherviews/contentsetview \
              ui/launcherviews/documentview \
              ui/launcherviews/hierarchdocumentview \
              ui/launcherviews/taskmanagerview \
              ui/taskmanager \
              ui/shutdown \
              ui/volumedlg \
              media/alertservice \
              media/volumemanagement \
              comm/session \
              bluetooth/audiovolumemgr \
              bluetooth/serial \
              bluetooth/scomisc \
              pim/savetocontacts \
              pim/servercontactmodel \
              phone/browserscreen/abstract \
              phone/browserscreen/gridbrowser \
              phone/callhistory/default \
              phone/callscreen/themed \
              phone/contextlabel/base \
              phone/dialer/abstract \
              phone/header/themed \
              phone/secondarydisplay/abstract \
              phone/telephony/callpolicymanager/abstract \
              phone/telephony/callpolicymanager/cell \
              phone/telephony/callpolicymanager/voip \
              phone/telephony/cell/cellbroadcast \
              phone/telephony/dialercontrol \
              phone/telephony/dialfilter/gsm  \
              phone/telephony/dialproxy \
              phone/telephony/msgcontrol \
              phone/telephony/phoneserver/dummyservice \
              phone/telephony/ringcontrol \
              phone/telephony/videoringtone \
              phone/themecontrol \
              phone/ui/callcontactmodelview \
              phone 



SERVER_FORMS+=\
    ui/shutdown/shutdown.ui \
    ui/volumedlg/volume.ui

SERVER_HEADERS+=\
    core_server/qabstractserverinterface.h \
    core_server/systemsuspend.h \
    ui/shutdown/shutdownimpl.h \
    ui/volumedlg/volumeimpl.h \
    ui/launcherviews/base/launcherview.h\
    core_server/qcoprouter.h\
    dbusrouter.h\
    media/alertservice/alertservicetask.h\
    comm/session/qabstractdevicemanager.h\
    processctrl/appmonitor/applicationmonitor.h\
    infrastructure/storagemonitor/storagemonitor.h \
    core_server/qtopiaserverapplication.h \
    core_server/environmentsetuptask.h \
    memory/base/memorymonitor.h \
    ui/standarddialogs.h \
    ui/components/delayedwaitdialog/delayedwaitdialog.h \
    memory/testmonitor/testmemorymonitor.h \
    core_server/pressholdgate.h \
    memory/monitor/genericmemorymonitor.h \
    core_server/applicationlauncher.h \
    memory/base/oommanager.h \
    processctrl/terminationhandler/qterminationhandlerprovider.h \
    core_server/qcopfile.h \
    net/netserver/networkserver.h \
    processctrl/startup/startupapps.h \
    core_server/timemonitor.h \
    core_server/timeupdateservice.h \
    core_server/qdeviceindicatorsprovider.h \
    ui/shutdownsplash/shutdownsplash.h \
    ui/launcherviews/contentsetview/contentsetlauncherview.h \
    infrastructure/stabmonitor/stabmonitortask.h \
    infrastructure/signalstrength/defaultsignal.h \
    core_server/defaultbattery.h\
    infrastructure/apm/apmbattery.h\
    obexservicemanager.h \
    media/keyclick/keyclick.h \
    core_server/devicebuttontask.h \
    ui/components/inputmethods/inputmethods.h \
    infrastructure/inputdevice/inputdevicesettings.h \
    memory/base/lowmemorytask.h \
    core_server/windowmanagement.h \
    core_server/virtualkeyboardservice.h \
    core_server/qtopiapowermanager.h\
    core_server/qtopiapowermanagerservice.h \
    standarddevicefeatures.h \
    core_server/qtopiainputevents.h \
    ui/waitindicator/waitindicator.h \
    processctrl/appshutdown/applicationshutdowntask.h

SERVER_SOURCES+=\
    main/main.cpp \
    core_server/qabstractserverinterface.cpp \
    core_server/systemsuspend.cpp \
    infrastructure/suspendtasks/systemsuspendtasks.cpp \
    ui/shutdown/shutdownimpl.cpp \
    ui/volumedlg/volumeimpl.cpp \
    ui/launcherviews/base/launcherview.cpp\
    core_server/qcoprouter.cpp\
    dbusrouter.cpp\
    media/alertservice/alertservicetask.cpp \
    comm/session/qabstractdevicemanager.cpp \
    processctrl/appmonitor/applicationmonitor.cpp \
    infrastructure/storagemonitor/storagemonitor.cpp \
    core_server/qtopiaserverapplication.cpp \
    core_server/environmentsetuptask.cpp \
    memory/base/memorymonitor.cpp \
    ui/standarddialogs.cpp \
    ui/components/delayedwaitdialog/delayedwaitdialog.cpp \
    memory/testmonitor/testmemorymonitor.cpp \
    core_server/pressholdgate.cpp \
    memory/monitor/genericmemorymonitor.cpp \
    core_server/applicationlauncher.cpp \
    memory/base/oommanager.cpp \
    processctrl/terminationhandler/qterminationhandlerprovider.cpp \
    core_server/qcopfile.cpp \
    net/netserver/networkserver.cpp \
    ui/shutdownsplash/shutdownsplash.cpp \
    processctrl/startup/startupapps.cpp \
    core_server/timemonitor.cpp \
    core_server/timeupdateservice.cpp \
    core_server/qdeviceindicatorsprovider.cpp \
    simplebuiltins.cpp \
    ui/launcherviews/contentsetview/contentsetlauncherview.cpp \
    core_server/defaultbattery.cpp\
    infrastructure/apm/apmbattery.cpp\
    processctrl/qdsync/qdsynctask.cpp\
    obexservicemanager.cpp\
    infrastructure/stabmonitor/stabmonitortask.cpp \
    processctrl/appshutdown/applicationshutdowntask.cpp \
    infrastructure/signalstrength/defaultsignal.cpp \
    media/keyclick/keyclick.cpp \
    core_server/devicebuttontask.cpp \
    ui/components/inputmethods/inputmethods.cpp \
    infrastructure/inputdevice/inputdevicesettings.cpp \
    memory/base/lowmemorytask.cpp \
    core_server/virtualkeyboardservice.cpp \
    core_server/qtopiapowermanager.cpp\
    core_server/qtopiapowermanagerservice.cpp \
    standarddevicefeatures.cpp \
    ui/waitindicator/waitindicator.cpp

enable_vpn {
    SERVER_HEADERS+=net/vpn/vpnmanager.h
    SERVER_SOURCES+=net/vpn/vpnmanager.cpp
}

!x11 {
    SERVER_HEADERS+=\
        media/screenclick/screenclick.h \
        infrastructure/keyboardlock/qkeyboardlock.h
    SERVER_SOURCES+=\
        media/screenclick/screenclick.cpp \
        infrastructure/keyboardlock/qkeyboardlock.cpp \
        core_server/windowmanagement.cpp \
        core_server/qtopiainputevents.cpp
} else {
    SERVER_SOURCES+=\
        core_server/windowmanagement_x11.cpp \
        core_server/qtopiainputevents_x11.cpp

    LIBS += -lXtst
}

equals(LAUNCH_METHOD,quicklaunch) {
    SERVER_SOURCES+=processctrl/quickexe/quickexeapplicationlauncher.cpp
    SERVER_HEADERS+=processctrl/quickexe/quickexeapplicationlauncher.h
}

SXE_HEADERS=security/monitor/securitymonitor.h 
SXE_SOURCES=security/monitor/securitymonitor.cpp 

TRANSLATABLES+=$$SXE_HEADERS $$SXE_SOURCES

enable_sxe {
    SERVER_HEADERS+=$$SXE_HEADERS
    SERVER_SOURCES+=$$SXE_SOURCES
}

DOCAPI_HEADERS=core_server/contentserver.h
DOCAPI_SOURCES=core_server/contentserver.cpp
TRANSLATABLES+=$$DOCAPI_HEADERS $$DOCAPI_SOURCES

SERVER_HEADERS+=$$DOCAPI_HEADERS
SERVER_SOURCES+=$$DOCAPI_SOURCES

CALIBRATE_HEADERS=ui/components/calibrate/calibrate.h
CALIBRATE_SOURCES=ui/components/calibrate/calibrate.cpp \
                  ui/components/calibrate/register.cpp

!x11 {
    HEADERS+=$$CALIBRATE_HEADERS
    SOURCES+=$$CALIBRATE_SOURCES
}

VPATH+=$$QTOPIA_DEPOT_PATH/src/applications/simapp
INCLUDEPATH+=$$QTOPIA_DEPOT_PATH/src/applications/simapp
SIMAPP_HEADERS=simapp.h simicons.h simwidgets.h
SIMAPP_SOURCES=simapp.cpp simicons.cpp simwidgets.cpp

enable_cell {
    HEADERS+=$$SIMAPP_HEADERS
    SOURCES+=$$SIMAPP_SOURCES
}

BLUETOOTH_HEADERS=bluetooth/servicemgr/bluetoothservicemanager.h \
		  bluetooth/pinhelper/btpinhelper.h \
                  bluetooth/powermgr/btpowerservice.h \
                  bluetooth/hs/btheadsettask.h \
                  bluetooth/hs/qbluetoothhsagserver_p.h \
                  bluetooth/hs/qbluetoothhsservice_p.h \
                  bluetooth/scomisc/scomisc_p.h \
                  bluetooth/audiovolumemgr/btaudiovolumemanager.h \
                  bluetooth/ftp/btftpservice.h

BLUETOOTH_SOURCES=bluetooth/servicemgr/bluetoothservicemanager.cpp \
                  bluetooth/pinhelper/btpinhelper.cpp \
                  bluetooth/powermgr/btpowerservice.cpp \
                  bluetooth/hs/btheadsettask.cpp \
                  bluetooth/hs/qbluetoothhsagserver.cpp \
                  bluetooth/hs/qbluetoothhsservice.cpp \
                  bluetooth/audiovolumemgr/btaudiovolumemanager.cpp \
                  bluetooth/ftp/btftpservice.cpp

# This is documented in src/build/doc/src/deviceprofiles.qdoc
isEmpty(DEVICE_CONFIG_PATH)|!exists($$DEVICE_CONFIG_PATH/server/scomisc.cpp) {
    BLUETOOTH_SOURCES+=bluetooth/scomisc/scomisc.cpp
}

BLUETOOTH_PHONE_HEADERS+=bluetooth/dun/btdialupservice.h \
                  bluetooth/serial/bluetoothserialportservice.h \
                  bluetooth/hf/bthandsfreetask.h \
                  bluetooth/hf/qbluetoothhfagserver_p.h \
                  bluetooth/hf/qbluetoothhfservice_p.h

BLUETOOTH_PHONE_SOURCES+=bluetooth/dun/btdialupservice.cpp \
                         bluetooth/serial/bluetoothserialportservice.cpp \
                  bluetooth/hf/bthandsfreetask.cpp \
                  bluetooth/hf/qbluetoothhfagserver.cpp \
                  bluetooth/hf/qbluetoothhfservice.cpp

TRANSLATABLES+=$$BLUETOOTH_HEADERS $$BLUETOOTH_SOURCES $$BLUETOOTH_PHONE_HEADERS $$BLUETOOTH_PHONE_SOURCES

INFRARED_HEADERS=infrared/powermgr/irpowerservice.h

INFRARED_SOURCES=infrared/powermgr/irpowerservice.cpp

TRANSLATABLES+=$$INFRARED_HEADERS $$INFRARED_SOURCES

#begin media
MEDIA_HEADERS=\
    media/servercontrol/mediaserver.h \
    media/volumemanagement/audiovolumemanager.h \
    media/volumemanagement/mediakeyservice.h \
    media/volumemanagement/mediaservicestask.h

MEDIA_SOURCES=\
    media/servercontrol/mediaserver.cpp \
    media/volumemanagement/audiovolumemanager.cpp \
    media/volumemanagement/mediakeyservice.cpp \
    media/volumemanagement/mediaservicestask.cpp  

HEADERS+=$$MEDIA_HEADERS
SOURCES+=$$MEDIA_SOURCES
#end media

UNPORTED_HEADERS=firstuse.h
UNPORTED_SOURCES=firstuse.cpp

PHONE_HEADERS=\
    phone/contextlabel/base/contextlabel.h \
    phone/themecontrol/themecontrol.h \
    ui/launcherviews/documentview/documentview.h \
    phone/browserscreen/gridbrowser/phonebrowser.h \
    processctrl/taskmanagerentry/taskmanagerentry.h \
    ui/launcherviews/taskmanagerview/taskmanagerlauncherview.h \
    infrastructure/camera/cameramonitor.h \
    core_server/alarmcontrol.h \
    phone/themecontrol/serverthemeview.h \
    phone/homescreencontrol.h \
    phone/qabstracthomescreen.h \
    phone/homescreen/themed/themedhomescreen.h \
    phone/themecontrol/themebackground_p.h \
    phone/profileprovider/qphoneprofileprovider.h \
    ui/abstractinterfaces/stdmessagebox/messagebox.h \
    phone/header/themed/phoneheader.h\
    phone/phonelock.h\
    ui/taskmanager/taskmanagerservice.h\
    core_server/qabstractmessagebox.h\
    phone/browserscreen/abstract/qabstractbrowserscreen.h\
    phone/serverinterface/phonelauncher/phonelauncher.h \
    phone/secondarydisplay/abstract/qabstractsecondarydisplay.h \
    infrastructure/softmenubar/qsoftmenubarprovider.h \
    phone/secondarydisplay/themed/secondarythemeddisplay.h \
    phone/receivewindow.h \
    phone/homescreenwidgets.h \
    phone/powermanager/phonepowermanager.h \
    net/gprsmonitor/gprsmonitor.h \
    phone/themecontrol/qabstractthemewidgetfactory.h \
    ui/components/touchscreenlockdlg/touchscreenlockdlg.h

TELEPHONY_HEADERS=\
    phone/telephony/atemulator/externalaccess.h \
    phone/telephony/msgcontrol/messagecontrol.h \
    phone/ui/callcontactmodelview/callcontactlist.h \
    phone/callscreen/themed/callscreen.h \
    phone/telephony/dialercontrol/dialercontrol.h \
    pim/savetocontacts/savetocontacts.h \
    phone/dialer/keypad/quickdial.h \
    phone/dialer/keypad/numberdisplay.h \
    phone/dialer/touch/dialer.h \
    phone/telephony/dialproxy/dialerservice.h \
    phone/dialer/abstract/qabstractdialerscreen.h\
    phone/telephony/phoneserver/base/phoneserver.h \
    phone/telephony/ringcontrol/ringcontrol.h\
    phone/telephony/ringcontrol/ringtoneservice.h\
    pim/servercontactmodel/servercontactmodel.h \
    phone/telephony/callpolicymanager/abstract/qabstractcallpolicymanager.h \
    phone/telephony/videoringtone/videoringtone.h

PHONE_SOURCES=\
    phone/contextlabel/base/contextlabel.cpp \
    phone/serverinterface/phonelauncher/phonelauncher.cpp \
    phone/themecontrol/themecontrol.cpp \
    ui/launcherviews/documentview/documentview.cpp \
    phone/browserscreen/gridbrowser/phonebrowser.cpp \
    processctrl/taskmanagerentry/taskmanagerentry.cpp \
    ui/launcherviews/taskmanagerview/taskmanagerlauncherview.cpp \
    infrastructure/camera/cameramonitor.cpp \
    core_server/alarmcontrol.cpp \
    phone/themecontrol/serverthemeview.cpp \
    phone/qabstracthomescreen.cpp \
    phone/homescreen/themed/themedhomescreen.cpp \
    phone/themecontrol/themebackground_p.cpp \
    phone/profileprovider/qphoneprofileprovider.cpp \
    ui/abstractinterfaces/stdmessagebox/messagebox.cpp \
    phone/header/themed/phoneheader.cpp\
    phone/phonelock.cpp\
    core_server/qabstractmessagebox.cpp\
    ui/taskmanager/taskmanagerservice.cpp\
    phone/secondarydisplay/abstract/qabstractsecondarydisplay.cpp \
    infrastructure/softmenubar/qsoftmenubarprovider.cpp \
    phone/secondarydisplay/themed/secondarythemeddisplay.cpp \
    phone/powermanager/phonepowermanager.cpp \
    phone/receivewindow.cpp \
    phone/homescreenwidgets.cpp \
    net/gprsmonitor/gprsmonitor.cpp \
    ui/components/touchscreenlockdlg/touchscreenlockdlg.cpp

TELEPHONY_SOURCES=\
    phone/telephony/atemulator/externalaccess.cpp \
    phone/telephony/msgcontrol/messagecontrol.cpp \
    phone/ui/callcontactmodelview/callcontactlist.cpp\
    phone/callscreen/themed/callscreen.cpp \
    phone/telephony/dialercontrol/dialercontrol.cpp \
    pim/savetocontacts/savetocontacts.cpp \
    phone/dialer/keypad/quickdial.cpp \
    phone/dialer/keypad/numberdisplay.cpp \
    phone/dialer/touch/dialer.cpp \
    phone/telephony/dialproxy/dialerservice.cpp \
    phone/telephony/ringcontrol/ringcontrol.cpp\
    phone/telephony/ringcontrol/ringtoneservice.cpp\
    phone/telephony/phoneserver/base/phoneserver.cpp \
    pim/servercontactmodel/servercontactmodel.cpp \
    phone/telephony/callpolicymanager/abstract/qabstractcallpolicymanager.cpp \
    phone/telephony/videoringtone/videoringtone.cpp

enable_telephony {
    TELEPHONY_HEADERS+=phone/callhistory/default/callhistory.h
    TELEPHONY_SOURCES+=phone/callhistory/default/callhistory.cpp
}

enable_cell {
    PHONE_HEADERS+=phone/telephony/callpolicymanager/cell/cellmodemmanager.h\
                   phone/telephony/cell/cellbroadcast/cellbroadcastcontrol.h\
                   phone/telephony/phoneserver/dummyservice/phoneserverdummymodem.h\
                   phone/telephony/dialfilter/gsm/gsmkeyactions.h\
                   phone/telephony/dialfilter/gsm/gsmkeyfilter.h
    PHONE_SOURCES+=phone/telephony/callpolicymanager/cell/cellmodemmanager.cpp\
                   phone/telephony/cell/cellbroadcast/cellbroadcastcontrol.cpp\
                   phone/telephony/phoneserver/dummyservice/phoneserverdummymodem.cpp\
                   phone/telephony/dialfilter/gsm/gsmkeyactions.cpp\
                   phone/telephony/dialfilter/gsm/gsmkeyfilter.cpp
}

enable_voip {
    PHONE_HEADERS+=\
        phone/telephony/callpolicymanager/voip/voipmanager.h\
        phone/telephony/callpolicymanager/asterisk/asteriskmanager.h
    PHONE_SOURCES+=\
        phone/telephony/callpolicymanager/voip/voipmanager.cpp\
        phone/telephony/callpolicymanager/asterisk/asteriskmanager.cpp
}

enable_hierarchicaldocumentview {
    PHONE_HEADERS+=\
        ui/launcherviews/hierarchdocumentview/hierarchicaldocumentview.h
    PHONE_SOURCES+=\
        ui/launcherviews/hierarchdocumentview/hierarchicaldocumentview.cpp
        
    DEFINES+=ENABLE_HIERARCHICAL_DOCUMENT_VIEW
}

PHONE_UI_SOURCES=\
    phone/browserscreen/gridbrowser/phonelauncherview.cpp \
    phone/browserscreen/gridbrowser/griditem.cpp \
    phone/browserscreen/gridbrowser/selecteditem.cpp \
    phone/browserscreen/gridbrowser/selecteditemconnector.cpp \
    phone/browserscreen/gridbrowser/griditemtable.cpp \
    phone/browserscreen/gridbrowser/oscillator_p.cpp \
    phone/browserscreen/gridbrowser/parabola_p.cpp \
    phone/browserscreen/gridbrowser/animator_p.cpp \
    phone/browserscreen/gridbrowser/animatorfactory_p.cpp \
    phone/browserscreen/gridbrowser/bouncer_p.cpp \
    phone/browserscreen/gridbrowser/zoomer_p.cpp \
    phone/browserscreen/gridbrowser/shearer_p.cpp \
    phone/browserscreen/gridbrowser/rotator_p.cpp \
    phone/browserscreen/gridbrowser/radialbackground_p.cpp \
    phone/browserscreen/gridbrowser/renderer.cpp


PHONE_UI_HEADERS=\
    phone/browserscreen/gridbrowser/phonelauncherview.h \
    phone/browserscreen/gridbrowser/griditem.h \
    phone/browserscreen/gridbrowser/selecteditem.h \
    phone/browserscreen/gridbrowser/selecteditemconnector.h \
    phone/browserscreen/gridbrowser/griditemtable.h \
    phone/browserscreen/gridbrowser/oscillator_p.h \
    phone/browserscreen/gridbrowser/parabola_p.h \
    phone/browserscreen/gridbrowser/animator_p.h \
    phone/browserscreen/gridbrowser/animatorfactory_p.cpp \
    phone/browserscreen/gridbrowser/bouncer_p.h \
    phone/browserscreen/gridbrowser/zoomer_p.h \
    phone/browserscreen/gridbrowser/shearer_p.h \
    phone/browserscreen/gridbrowser/rotator_p.h \
    phone/browserscreen/gridbrowser/radialbackground_p.h \
    phone/browserscreen/gridbrowser/renderer.h

enable_phone_ui {
    HEADERS+=$$PHONE_UI_HEADERS
    SOURCES+=$$PHONE_UI_SOURCES
}

SAMPLES_SOURCES=\
    ui/abstractinterfaces/slideinmessagebox/slideinmessagebox.cpp \
    phone/browserscreen/wheelbrowser/wheelbrowser.cpp \
    phone/browserscreen/wheelbrowser/qpixmapwheel.cpp

SAMPLES_HEADERS=\
    ui/abstractinterfaces/slideinmessagebox/slideinmessagebox.h \
    phone/browserscreen/wheelbrowser/wheelbrowser.h \
    phone/browserscreen/wheelbrowser/qpixmapwheel.h

enable_modem:enable_cell {
    SAMPLES_SOURCES+=\
        phone/samples/e1/e1_bar.cpp\
        phone/samples/e1/e1_battery.cpp\
        phone/samples/e1/e1_header.cpp\
        phone/samples/e1/e1_launcher.cpp\
        phone/samples/e1/e1_phonebrowser.cpp\
        phone/samples/e1/colortint.cpp\
        phone/samples/e1/e1_dialer.cpp\
        phone/samples/e1/e1_error.cpp\
        phone/samples/e1/e1_incoming.cpp\
        phone/samples/e1/e1_dialog.cpp\
        phone/samples/e1/e1_callhistory.cpp\
        phone/samples/e1/e1_popup.cpp\
        phone/samples/e1/e1_callscreen.cpp\
        phone/samples/e1/e1_telephony.cpp\
        phone/samples/e2/e2_header.cpp\
        phone/samples/e2/e2_launcher.cpp\
        phone/samples/e2/e2_frames.cpp\
        phone/samples/e2/e2_bar.cpp\
        phone/samples/e2/e2_taskmanager.cpp\
        phone/samples/e2/e2_telephony.cpp\
        phone/samples/e2/e2_callscreen.cpp\
        phone/samples/e2/e2_dialer.cpp\
        phone/samples/e2/e2_browser.cpp\
        phone/samples/e2/e2_telephonybar.cpp\
        phone/samples/e3/e3_phonebrowser.cpp\
        phone/samples/e3/e3_launcher.cpp\
        phone/samples/e3/e3_today.cpp\
        phone/samples/e3/e3_navipane.cpp\
        phone/samples/e3/e3_clock.cpp

    SAMPLES_HEADERS+=\
        phone/samples/e1/e1_bar.h\
        phone/samples/e1/e1_battery.h\
        phone/samples/e1/e1_header.h\
        phone/samples/e1/e1_launcher.h\
        phone/samples/e1/e1_dialog.h\
        phone/samples/e1/e1_dialer.h\
        phone/samples/e1/e1_error.h\
        phone/samples/e1/e1_callhistory.h\
        phone/samples/e1/e1_phonebrowser.h\
        phone/samples/e1/e1_popup.h\
        phone/samples/e1/e1_callscreen.h\
        phone/samples/e1/colortint.h\
        phone/samples/e1/e1_incoming.h\
        phone/samples/e1/e1_telephony.h\
        phone/samples/e2/e2_header.h\
        phone/samples/e2/e2_launcher.h\
        phone/samples/e2/e2_frames.h\
        phone/samples/e2/e2_bar.h\
        phone/samples/e2/e2_telephony.h\
        phone/samples/e2/e2_callscreen.h\
        phone/samples/e2/e2_dialer.h\
        phone/samples/e2/e2_browser.h\
        phone/samples/e2/e2_taskmanager.h\
        phone/samples/e2/e2_telephonybar.h\
        phone/samples/e2/e2_colors.h\
        phone/samples/e3/e3_phonebrowser.h\
        phone/samples/e3/e3_launcher.h\
        phone/samples/e3/e3_today.h\
        phone/samples/e3/e3_navipane.h\
        phone/samples/e3/e3_clock.h

samples_settings.files=\
    $$QTOPIA_DEPOT_PATH/etc/default/Trolltech/E3.conf
samples_settings.path=/etc/default/Trolltech
INSTALLS+=samples_settings
}

enable_samples {
    HEADERS+=$$SAMPLES_HEADERS
    SOURCES+=$$SAMPLES_SOURCES

    samplespics.files=$$QTOPIA_DEPOT_PATH/pics/samples/*
    samplespics.path=/pics/samples
    samplespics.hint=pics
    INSTALLS+=samplespics

    samplesprofilepics.files=$$QTOPIA_DEPOT_PATH/pics/profiles/*
    samplesprofilepics.path=/pics/profiles
    samplesprofilepics.hint=pics
    INSTALLS+=samplesprofilepics
}

TESTS_SOURCES += test/cellmodemmanagertest.cpp
TESTS_HEADERS += test/cellmodemmanagertest.h

enable_tests {
    HEADERS+=$$TESTS_HEADERS
    SOURCES+=$$TESTS_SOURCES
}

TRANSLATABLES+=$$PHONE_HEADERS $$PHONE_SOURCES

enable_bluetooth {
    PHONE_HEADERS+=$$BLUETOOTH_HEADERS
    PHONE_SOURCES+=$$BLUETOOTH_SOURCES
}
enable_infrared {
    PHONE_HEADERS+=$$INFRARED_HEADERS
    PHONE_SOURCES+=$$INFRARED_SOURCES
}

# This is necessary for Handsfree / Headset to work
equals(QTOPIA_SOUND_SYSTEM,alsa) {
    depends(3rdparty/libraries/alsa)
    DEFINES+=HAVE_ALSA
}

isEmpty(DEVICE_CONFIG_PATH) {
    PHONE_HEADERS+=media/dummyvolumeservice/dummyvolumeservice.h
    PHONE_SOURCES+=media/dummyvolumeservice/dummyvolumeservice.cpp
}

HEADERS+=$$PHONE_HEADERS
SOURCES+=$$PHONE_SOURCES

# phone ui
HEADERS+=$$TELEPHONY_HEADERS
DEFINES+=QTOPIA_PHONEUI
SOURCES+=$$TELEPHONY_SOURCES
enable_bluetooth {
    HEADERS+=$$BLUETOOTH_PHONE_HEADERS
    SOURCES+=$$BLUETOOTH_PHONE_SOURCES
}
depends(libraries/qtopiaphone)
depends(libraries/qtopiapim)
depends(3rdparty/libraries/openobex)
depends(libraries/qtopiacomm)
enable_qtopiamedia {
    depends(libraries/qtopiamedia)
}
depends(libraries/qtopiaaudio)

enable_modem {
    depends(libraries/qtopiaphonemodem)
}

# This is documented in src/build/doc/src/deviceprofiles.qdoc
!isEmpty(DEVICE_CONFIG_PATH) {
    INCLUDEPATH+=$$DEVICE_CONFIG_PATH/server
    SERVER_HEADERS+=$$files($$DEVICE_CONFIG_PATH/server/*.h)
    SERVER_SOURCES+=$$files($$DEVICE_CONFIG_PATH/server/*.cpp)
    SERVER_FORMS+=$$files($$DEVICE_CONFIG_PATH/server/*.ui)
}

enable_sxe:depends(libraries/qtopiasecurity)

drmagent {
    SERVER_HEADERS+=ui/launcherviews/archiveview/archiveviewer.h
    SERVER_SOURCES+=ui/launcherviews/archiveview/archiveviewer.cpp
    archivesdesktop.files=$$QTOPIA_DEPOT_PATH/apps/Applications/archives.desktop
    archivesdesktop.path=/apps/Applications
    archivesdesktop.hint=desktop
    INSTALLS+=archivesdesktop
    archiveshelp.source=$$QTOPIA_DEPOT_PATH/help
    archiveshelp.files=qpe-archives*
    archiveshelp.hint=help
    INSTALLS+=archiveshelp
    archivespics.files=$$QTOPIA_DEPOT_PATH/pics/archives/*
    archivespics.path=/pics/archives
    archivespics.hint=pics
    INSTALLS+=archivespics
}

FORMS+=$$SERVER_FORMS
HEADERS+=$$SERVER_HEADERS
SOURCES+=$$SERVER_SOURCES

sdk_server_headers.files=$$SERVER_HEADERS
sdk_server_headers.path=/src/server
sdk_server_headers.hint=sdk
INSTALLS+=sdk_server_headers

sdk_calibrate_headers.files=$$CALIBRATE_HEADERS
sdk_calibrate_headers.path=/src/settings/calibrate
sdk_calibrate_headers.hint=sdk
INSTALLS+=sdk_calibrate_headers

sdk_edn_headers.files=$$PHONE_HEADERS
sdk_edn_headers.path=/src/server/phone
sdk_edn_headers.hint=sdk
INSTALLS+=sdk_edn_headers

bins.files=\
    $$QTOPIA_DEPOT_PATH/bin/targzip\
    $$QTOPIA_DEPOT_PATH/bin/targunzip
bins.path=/bin
bins.hint=script
INSTALLS+=bins

taskmanagerdesktop.files=$$QTOPIA_DEPOT_PATH/apps/Settings/taskmanager.desktop
taskmanagerdesktop.path=/apps/Settings
taskmanagerdesktop.hint=desktop
INSTALLS+=taskmanagerdesktop

calibratedesktop.files=$$QTOPIA_DEPOT_PATH/apps/Settings/calibrate.desktop
calibratedesktop.path=/apps/Settings
calibratedesktop.hint=desktop
INSTALLS+=calibratedesktop

calibrateservice.files=$$QTOPIA_DEPOT_PATH/services/calibrate/calibrate
calibrateservice.path=/services/calibrate
INSTALLS+=calibrateservice

launcherservice.files=$$QTOPIA_DEPOT_PATH/services/Launcher/qpe
launcherservice.path=/services/Launcher
INSTALLS+=launcherservice

contentsetviewservice.files=$$QTOPIA_DEPOT_PATH/services/ContentSetView/qpe
contentsetviewservice.path=/services/ContentSetView
INSTALLS+=contentsetviewservice

alertservice.files=$$QTOPIA_DEPOT_PATH/services/Alert/qpe
alertservice.path=/services/Alert
INSTALLS+=alertservice

timeupdateservice.files=$$QTOPIA_DEPOT_PATH/services/TimeUpdate/qpe
timeupdateservice.path=/services/TimeUpdate
INSTALLS+=timeupdateservice

qtopiapowermanager.files=$$QTOPIA_DEPOT_PATH/services/QtopiaPowerManager/qpe
qtopiapowermanager.path=/services/QtopiaPowerManager
INSTALLS+=qtopiapowermanager

virtualkeyboardservice.files=$$QTOPIA_DEPOT_PATH/services/VirtualKeyboard/qpe
virtualkeyboardservice.path=/services/VirtualKeyboard
INSTALLS+=virtualkeyboardservice

touchscreendialerservice.files=$$QTOPIA_DEPOT_PATH/services/TouchscreenDialer/qpe
touchscreendialerservice.path=/services/TouchscreenDialer
INSTALLS+=touchscreendialerservice

calibratepics.files=$$QTOPIA_DEPOT_PATH/pics/calibrate/*
calibratepics.path=/pics/calibrate
calibratepics.hint=pics
INSTALLS+=calibratepics

appspics.files=$$QTOPIA_DEPOT_PATH/pics/*.* $$QTOPIA_DEPOT_PATH/pics/icons
appspics.path=/pics
appspics.hint=pics
INSTALLS+=appspics

pics.files=$$QTOPIA_DEPOT_PATH/pics/qpe/*.* $$QTOPIA_DEPOT_PATH/pics/qpe/icons
pics.path=/pics/qpe
pics.hint=pics
INSTALLS+=pics

enable_cell {
    sdk_simapp_headers.files=$$SIMAPP_HEADERS
    sdk_simapp_headers.path=/src/applications/simapp
    sdk_simapp_headers.hint=sdk
    INSTALLS+=sdk_simapp_headers

    simappdesktop.files=$$QTOPIA_DEPOT_PATH/apps/Applications/simapp.desktop
    simappdesktop.path=/apps/Applications
    simappdesktop.hint=desktop
    INSTALLS+=simappdesktop

    simapppics.files=$$QTOPIA_DEPOT_PATH/pics/simapp/*
    simapppics.path=/pics/simapp
    simapppics.hint=pics
    INSTALLS+=simapppics
}

settings.files=\
    $$QTOPIA_DEPOT_PATH/etc/default/Trolltech/locale.conf\
    $$QTOPIA_DEPOT_PATH/etc/default/Trolltech/ServerWidgets.conf\
    $$QTOPIA_DEPOT_PATH/etc/default/Trolltech/Security.conf\
    $$QTOPIA_DEPOT_PATH/etc/default/Trolltech/IniValueSpace.conf\
    $$QTOPIA_DEPOT_PATH/etc/default/Trolltech/BluetoothServices.conf\
    $$QPEDIR/etc/default/Trolltech/IconSizes.conf
!isEmpty(HARDWARE_CONF_FILE) {
    settings.files+=$$HARDWARE_CONF_FILE
}
settings.path=/etc/default/Trolltech
INSTALLS+=settings

# This is documented in src/build/doc/src/deviceprofiles.qdoc
!isEmpty(DEVICE_CONFIG_PATH):exists($$DEVICE_CONFIG_PATH/etc/default/Trolltech/qpe.conf):qpe_conf.files=$$DEVICE_CONFIG_PATH/etc/default/Trolltech/qpe.conf
else:qpe_conf.files=$$QTOPIA_DEPOT_PATH/etc/default/Trolltech/qpe.conf
qpe_conf.path=/etc/default/Trolltech
INSTALLS+=qpe_conf

!enable_singleexec {
    inputmethods.files=$$QTOPIA_DEPOT_PATH/plugins/inputmethods/.directory
    inputmethods.path=/plugins/inputmethods
    INSTALLS+=inputmethods

    obex.files=$$QTOPIA_DEPOT_PATH/plugins/obex/.directory
    obex.path=/plugins/obex
    INSTALLS+=obex

    network.files=$$QTOPIA_DEPOT_PATH/plugins/network/.directory
    network.path=/plugins/network
    INSTALLS+=network
}

speeddialsounds.files=$$QTOPIA_DEPOT_PATH/sounds/speeddial
speeddialsounds.path=/sounds
INSTALLS+=speeddialsounds

servicedefs.files=$$files($$QTOPIA_DEPOT_PATH/services/*.service)
!enable_cell {
    servicedefs.files -= $$QTOPIA_DEPOT_PATH/services/SMS.service
    servicedefs.files -= $$QTOPIA_DEPOT_PATH/services/CallForwarding.service
    servicedefs.files -= $$QTOPIA_DEPOT_PATH/services/CallNetworks.service
    servicedefs.files -= $$QTOPIA_DEPOT_PATH/services/Dialer.service
    servicedefs.files -= $$QTOPIA_DEPOT_PATH/services/SimApp.service
    servicedefs.files -= $$QTOPIA_DEPOT_PATH/services/Ringtone.service
}
!drmagent {
    servicedefs.files -= $$QTOPIA_DEPOT_PATH/services/OmaDrmAgent.service
}
servicedefs.path=/services
INSTALLS+=servicedefs

help.source=$$QTOPIA_DEPOT_PATH/help
help.files=\
    index.html\
    qpe*\
    help-*\
    help.html\
    document*\
    device*\
    appservices.html\
    simapp.html\
    callhistory.html\
    calibrat*\
    dialer*
help.hint=help
INSTALLS+=help

beam.files=$$QTOPIA_DEPOT_PATH/etc/beam
beam.path=/etc
INSTALLS+=beam

# This is documented in src/build/doc/src/deviceprofiles.qdoc
!isEmpty(DEVICE_CONFIG_PATH):exists($$DEVICE_CONFIG_PATH/etc/Tasks.cfg) {
    tasks.files=$$DEVICE_CONFIG_PATH/etc/Tasks.cfg
} else {
    tasks.files=$$QTOPIA_DEPOT_PATH/etc/Tasks.cfg
}
tasks.path=/etc
INSTALLS+=tasks

defaultbuttons {
    defbtn.files=$$QPEDIR/etc/defaultbuttons.conf
    defbtn.path=/etc
    INSTALLS+=defbtn
}

suspendservice.files=$$QTOPIA_DEPOT_PATH/services/Suspend/qpe
suspendservice.path=/services/Suspend
INSTALLS+=suspendservice

enable_infrared {
    irbeamingservice.files=$$QTOPIA_DEPOT_PATH/services/InfraredBeaming/qpe
    irbeamingservice.path=/services/InfraredBeaming
    INSTALLS+=irbeamingservice

    irqdsservice.files=$$QTOPIA_DEPOT_PATH/etc/qds/InfraredBeaming
    irqdsservice.path=/etc/qds
    INSTALLS+=irqdsservice
}

enable_bluetooth {
    btpushservice.files=$$QTOPIA_DEPOT_PATH/services/BluetoothPush/qpe
    btpushservice.path=/services/BluetoothPush
    INSTALLS+=btpushservice

    btqdsservice.files=$$QTOPIA_DEPOT_PATH/etc/qds/BluetoothPush
    btqdsservice.path=/etc/qds
    INSTALLS+=btqdsservice

    btservices.files=$$QTOPIA_DEPOT_PATH/etc/bluetooth/sdp/*.xml
    btservices.path=/etc/bluetooth/sdp
    INSTALLS+=btservices
}

phonepics.files=$$QTOPIA_DEPOT_PATH/pics/qpe/phone/*
phonepics.path=/pics/qpe/phone
phonepics.hint=pics
INSTALLS+=phonepics

globalphonepics.files=$$QTOPIA_DEPOT_PATH/pics/phone/*
globalphonepics.path=/pics/phone
globalphonepics.hint=pics
INSTALLS+=globalphonepics

settings.files+=$$QTOPIA_DEPOT_PATH/etc/default/Trolltech/PhoneProfile.conf

defaultalerts.files=$$QTOPIA_DEPOT_PATH/etc/SystemRingTones/*.wav
defaultalerts.path=/etc/SystemRingTones
defaultalerts.hint=content nct
defaultalerts.categories=SystemRingtones
defaultalerts.trtarget=QtopiaRingTones
INSTALLS+=defaultalerts

enable_telephony {
    callhistorydesktop.files=$$QTOPIA_DEPOT_PATH/apps/Applications/callhistory.desktop
    callhistorydesktop.path=/apps/Applications
    callhistorydesktop.hint=desktop
    INSTALLS+=callhistorydesktop

    callhistorypics.files=$$QTOPIA_DEPOT_PATH/pics/callhistory/*
    callhistorypics.path=/pics/callhistory
    callhistorypics.hint=pics
    INSTALLS+=callhistorypics

    callhistoryservice.files=$$QTOPIA_DEPOT_PATH/services/CallHistory/qpe
    callhistoryservice.path=/services/CallHistory
    INSTALLS+=callhistoryservice
}

taskmanagerservice.files=$$QTOPIA_DEPOT_PATH/services/TaskManager/qpe
taskmanagerservice.path=/services/TaskManager
INSTALLS+=taskmanagerservice

defaulttheme.files=$$QTOPIA_DEPOT_PATH/etc/themes/default/*
defaulttheme.path=/etc/themes/default
INSTALLS+=defaulttheme

defaultpics.files=$$QTOPIA_DEPOT_PATH/pics/themes/default*
defaultpics.path=/pics/themes
defaultpics.hint=pics
INSTALLS+=defaultpics

dialerservice.files=$$QTOPIA_DEPOT_PATH/services/Dialer/qpe
dialerservice.path=/services/Dialer
INSTALLS+=dialerservice

ringtoneservice.files=$$QTOPIA_DEPOT_PATH/services/Ringtone/qpe
ringtoneservice.path=/services/Ringtone
INSTALLS+=ringtoneservice

ANIMFILE=$$QTOPIA_DEPOT_PATH/pics/qpe/splash/$${QTOPIA_DISP_WIDTH}x$${QTOPIA_DISP_HEIGHT}/splash.gif
exists($$ANIMFILE) {
    splash.files=$$ANIMFILE
} else {
    # Fall-back un-animated
    splash.files=$$QTOPIA_DEPOT_PATH/src/server/splash.png
}
splash.path=/pics/qpe
INSTALLS+=splash

enable_sxe {
    security.path=/etc
    security.commands=$$COMMAND_HEADER\
        install -c $$QTOPIA_DEPOT_PATH/etc/sxe.* $(INSTALL_ROOT)/etc $$LINE_SEP\
        chmod 0600 $(INSTALL_ROOT)/etc/sxe.* $$LINE_SEP\
        mkdir -p $(INSTALL_ROOT)/etc/rekey $$LINE_SEP\
        mkdir -p $(INSTALL_ROOT)/etc/sxe_qtopia $$LINE_SEP\
        mkdir -p $(INSTALL_ROOT)/etc/sxe_domains $$LINE_SEP

    SXE_SCRIPTS=sxe_qtopia sxe_sandbox sxe_unsandbox sxe_reloadconf
    for(file,SXE_SCRIPTS) {
        # This is documented in src/build/doc/src/deviceprofiles.qdoc
        !isEmpty(DEVICE_CONFIG_PATH):exists($$DEVICE_CONFIG_PATH/etc/sxe_qtopia/$$file) {
            security.commands+=\
                install -m 0500 -c $$DEVICE_CONFIG_PATH/etc/sxe_qtopia/$$file $(INSTALL_ROOT)/etc/sxe_qtopia $$LINE_SEP
        } else {
            security.commands+=\
                install -m 0500 -c $$QTOPIA_DEPOT_PATH/etc/sxe_qtopia/$$file $(INSTALL_ROOT)/etc/sxe_qtopia $$LINE_SEP
        }
    }

    security.commands+=\
        install -m 0500 -c $$QTOPIA_DEPOT_PATH/etc/sxe_domains/* $(INSTALL_ROOT)/etc/sxe_domains $$LINE_SEP

    # This is documented in src/build/doc/src/deviceprofiles.qdoc
    !isEmpty(DEVICE_CONFIG_PATH):exists($$DEVICE_CONFIG_PATH/etc/sxe_domains) {
        security.commands+=\
            install -m 0500 -c $$DEVICE_CONFIG_PATH/etc/sxe_domains/* $(INSTALL_ROOT)/etc/sxe_domains $$LINE_SEP
    }
            
    # This is documented in src/build/doc/src/deviceprofiles.qdoc
    !isEmpty(DEVICE_CONFIG_PATH):exists($$DEVICE_CONFIG_PATH/etc/default/Trolltech/Sxe.conf) {
        settings.files+=$$DEVICE_CONFIG_PATH/etc/default/Trolltech/Sxe.conf
    } else {
        settings.files+=$$QTOPIA_DEPOT_PATH/etc/default/Trolltech/Sxe.conf
    }

    INSTALLS+=security
}

# We want an en_US .directory file and dictionaries, even if en_US was disabled via -languages
commands=$$COMMAND_HEADER
INST_LANGUAGES=$$LANGUAGES
INST_LANGUAGES*=en_US
for(lang,INST_LANGUAGES) {
    !equals(commands,$$COMMAND_HEADER):commands+=$$LINE_SEP
    commands+=\
        mkdir -p $(INSTALL_ROOT)$$resdir/i18n/$$lang $$LINE_SEP_VERBOSE\
        install -c $$QTOPIA_DEPOT_PATH/i18n/$$lang/.directory $(INSTALL_ROOT)$$resdir/i18n/$$lang
}
langfiles.commands=$$commands
langfiles.CONFIG=no_path
INSTALLS+=langfiles

qdawggen=$$QPEDIR/bin/qdawggen
!equals(QTOPIA_HOST_ENDIAN,$$QTOPIA_TARGET_ENDIAN):qdawggen+=-e
commands=$$COMMAND_HEADER
for(lang,INST_LANGUAGES) {
    exists($$QTOPIA_DEPOT_PATH/etc/dict/$$lang/words) {
        !equals(commands,$$COMMAND_HEADER):commands+=$$LINE_SEP
        commands+=\
            mkdir -p $(INSTALL_ROOT)$$resdir/etc/dict/$$lang $$LINE_SEP\
            find $$QTOPIA_DEPOT_PATH/etc/dict/$$lang -maxdepth 1 -type f | xargs -r $$qdawggen $(INSTALL_ROOT)$$resdir/etc/dict/$$lang
    }
}
dicts.commands=$$commands
dicts.CONFIG=no_path
INSTALLS+=dicts

EXTRA_TS_FILES=\
    QtopiaApplications QtopiaGames QtopiaSettings QtopiaI18N QtopiaServices\
    QtopiaNetworkServices QtopiaBeaming QtopiaColorSchemes QtopiaDefaults\
    QtopiaRingTones QtopiaThemes Categories-Qtopia

ALL_TRANSLATIONS=$$AVAILABLE_LANGUAGES
ALL_TRANSLATIONS-=$$STRING_LANGUAGE

# lupdate for "global" stuff
nct_lupdate.commands=$$COMMAND_HEADER\
    cd $$PWD;\
    find $$QTOPIA_DEPOT_PATH\
        # apps, plugins and i18n
        $${LITERAL_BACKSLASH}( $${LITERAL_BACKSLASH}( -path $${LITERAL_SQUOTE}$$QTOPIA_DEPOT_PATH/apps/*$${LITERAL_SQUOTE} -o\
              -path $${LITERAL_SQUOTE}$$QTOPIA_DEPOT_PATH/plugins/*$${LITERAL_SQUOTE} -o\
              -path $${LITERAL_SQUOTE}$$QTOPIA_DEPOT_PATH/i18n/*$${LITERAL_SQUOTE} $${LITERAL_BACKSLASH}) -a\
           # .directory and .desktop files
           $${LITERAL_BACKSLASH}( -name $${LITERAL_SQUOTE}.directory$${LITERAL_SQUOTE} -o -name $${LITERAL_SQUOTE}*.desktop$${LITERAL_SQUOTE} $${LITERAL_BACKSLASH}) $${LITERAL_BACKSLASH}) -o\
        # etc
        $${LITERAL_BACKSLASH}( $${LITERAL_BACKSLASH}( -path $${LITERAL_SQUOTE}$$QTOPIA_DEPOT_PATH/etc/*$${LITERAL_SQUOTE} $${LITERAL_BACKSLASH}) -a\
           # .conf and .scheme files
           $${LITERAL_BACKSLASH}( -name $${LITERAL_SQUOTE}*.conf$${LITERAL_SQUOTE} -o -name $${LITERAL_SQUOTE}*.scheme$${LITERAL_SQUOTE} -o -name $${LITERAL_SQUOTE}*.desktop$${LITERAL_SQUOTE} $${LITERAL_BACKSLASH}) $${LITERAL_BACKSLASH}) -o\
        # qds
        $${LITERAL_BACKSLASH}( -path $${LITERAL_SQUOTE}$$QTOPIA_DEPOT_PATH/etc/qds/*$${LITERAL_SQUOTE} $${LITERAL_BACKSLASH}) -o\
        # pics
        $${LITERAL_BACKSLASH}( $${LITERAL_BACKSLASH}( -path $${LITERAL_SQUOTE}$$QTOPIA_DEPOT_PATH/pics/*$${LITERAL_SQUOTE} $${LITERAL_BACKSLASH}) -a\
           # config files (media player skins)
           $${LITERAL_BACKSLASH}( -name $${LITERAL_SQUOTE}config$${LITERAL_SQUOTE} $${LITERAL_BACKSLASH}) $${LITERAL_BACKSLASH}) -o\
        # services (all files)
        $${LITERAL_BACKSLASH}( -path $${LITERAL_SQUOTE}$$QTOPIA_DEPOT_PATH/services/*$${LITERAL_SQUOTE} $${LITERAL_BACKSLASH}) |\
    $$QBS_BIN/nct_lupdate\
        -nowarn\
        -depot\
        $$LITERAL_QUOTE$$QTOPIA_DEPOT_PATH$$LITERAL_QUOTE\
        $$LITERAL_QUOTE$$DQTDIR$$LITERAL_QUOTE\
        $$LITERAL_QUOTE$$ALL_TRANSLATIONS$$LITERAL_QUOTE\
        -
lupdate.depends+=nct_lupdate
QMAKE_EXTRA_TARGETS+=nct_lupdate

pkg.name=qpe-taskbar
pkg.desc=Launcher for QPE
pkg.domain=qpe

# FIXME THIS SHOULD NOT BE HERE!!!
dep(INCLUDEPATH+=$$PWD)
dep(pkg.deps+=$$pkg.name)

enable_singleexec {
    # Depend on everything so we can guarantee that this directory is processed last
    for(p,PROJECTS) {
        !equals(p,server):!contains(QTOPIA_DEPENDS,$$p):depends($$p,fake)
    }

    # Final preparation for singleexec build
    # This is hidden from other .pro files because it is very expensive
    equals(QTOPIA_ID,server) {
        # The server can't be built as a .a file
        CONFIG-=enable_singleexec
        # However, we still need this define so singleexec can be detected in code
        DEFINES+=SINGLE_EXEC
        # This is rediculous. Just ignore it.
        CONFIG-=link_prl

        cmds=$$fromfile(singleexec_reader.pri,SINGLEEXEC_READER_CMD)
        #message(singleexec_reader tells us:)
        for(c,cmds) {
            contains(QMAKE_BEHAVIORS,keep_quotes) {
                c~=s/^"//
                c~=s/"$//
            }
            #message($$c)
            runlast($$c)
        }
        # This avoids problems due to incorrect ordering of libs.
        # I think it tells the linker to treat all these seperate files as one giant library.
        # Surely this is slow and ineffienent but it does seem to work :)
        cmd=LIBS=-Wl,-whole-archive \$$LIBS -Wl,-no-whole-archive
        runlast($$cmd)
        #message(Done! $$LIBS)
    }

    # qmake isn't putting in these dependencies so do it outselves
    files=$$files($$QPEDIR/lib/*.a)
    files*=$$files($$QTEDIR/lib/*.a)
    qpe.depends=$$files
    qpe.commands=
    QMAKE_EXTRA_TARGETS+=qpe

    ipatchqt.commands=$$COMMAND_HEADER\
        $$fixpath($$QBS_BIN/patchqt) $$fixpath($(INSTALL_ROOT)/bin/qpe) $$QTOPIA_PREFIX
    ipatchqt.CONFIG=no_path
    ipatchqt.depends=install_target
    INSTALLS+=ipatchqt
}

