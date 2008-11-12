qtopia_project(stub)

defineReplace(maketarget) {
    target=$$1
    target~=s,/,_,g
    target~=s,\.\._,,g
    return($$target)
}

# Depend on everything so we can guarantee that this directory is processed last
for(p,PROJECTS) {
    depends($$p,fake)
}
QMAKE_STRIP=

startup.files=$$QTOPIA_DEPOT_PATH/devices/ficgta01/src/devtools/startup/qpe.sh \
              $$QTOPIA_DEPOT_PATH/devices/ficgta01/src/devtools/startup/qpe.env
startup.path=/
startup.hint=script
INSTALLS+=startup

script.files=$$QTOPIA_DEPOT_PATH/devices/ficgta01/src/devtools/scripts/*
script.path=/bin
script.hint=script
INSTALLS+=script

f_dir.files=$$QTOPIA_DEPOT_PATH/devices/ficgta01/src/devtools/.directory
f_dir.path=/apps/Devtools
f_dir.trtarget=Devtools
f_dir.hint=desktop nct
INSTALLS+=f_dir

desktop.files+=$$QTOPIA_DEPOT_PATH/devices/ficgta01/src/devtools/desktop/docapi-rescan.desktop
desktop.files+=$$QTOPIA_DEPOT_PATH/devices/ficgta01/src/devtools/desktop/bt-poweron.desktop
desktop.files+=$$QTOPIA_DEPOT_PATH/devices/ficgta01/src/devtools/desktop/bt-poweroff.desktop
desktop.files+=$$QTOPIA_DEPOT_PATH/devices/ficgta01/src/devtools/desktop/get-ssh-key.desktop
desktop.files+=$$QTOPIA_DEPOT_PATH/devices/ficgta01/src/devtools/desktop/fast-charge.desktop
desktop.files+=$$QTOPIA_DEPOT_PATH/devices/ficgta01/src/devtools/desktop/gps-poweron.desktop
desktop.files+=$$QTOPIA_DEPOT_PATH/devices/ficgta01/src/devtools/desktop/gps-poweroff.desktop
desktop.files+=$$QTOPIA_DEPOT_PATH/devices/ficgta01/src/devtools/desktop/gsm-poweron.desktop
desktop.files+=$$QTOPIA_DEPOT_PATH/devices/ficgta01/src/devtools/desktop/gsm-poweroff.desktop

desktop.path=/apps/Devtools
desktop.depends+=install_docapi_f_dir
desktop.hint=desktop
INSTALLS+=desktop

pics.files=$$QTOPIA_DEPOT_PATH/devices/ficgta01/src/devtools/*.png\
           $$QTOPIA_DEPOT_PATH/devices/ficgta01/src/devtools/*.svg
pics.path=/pics/devtools
pics.hint=pics
INSTALLS+=pics

help.source=$$DEVICE_CONFIG_PATH/help
help.files=qpe-devtools*
help.hint=help
INSTALLS+=help

#probably not the best place to put this stuff
conf.files=$$QTOPIA_DEPOT_PATH/devices/ficgta01/etc/default/Trolltech/PredictiveKeyboard.conf 
conf.files+=$$QTOPIA_DEPOT_PATH/devices/ficgta01/etc/default/Trolltech/Bluetooth.conf
conf.path=/etc/default/Trolltech
INSTALLS+=conf
