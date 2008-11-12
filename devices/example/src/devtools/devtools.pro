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

f_dir.files=$$QTOPIA_DEPOT_PATH/devices/example/src/devtools/.directory
f_dir.path=/apps/Devtools
f_dir.trtarget=Devtools
f_dir.hint=desktop nct
INSTALLS+=f_dir

startup.files=$$QTOPIA_DEPOT_PATH/devices/example/src/devtools/startup/qpe.sh \
              $$QTOPIA_DEPOT_PATH/devices/example/src/devtools/startup/qpe.env
startup.path=/
startup.hint=script

script.files=$$QTOPIA_DEPOT_PATH/devices/example/src/devtools/scripts/*
script.path=/bin
script.hint=script

desktop.files=$$QTOPIA_DEPOT_PATH/devices/example/src/devtools/desktop/docapi-rescan.desktop
desktop.files+=$$QTOPIA_DEPOT_PATH/devices/example/src/devtools/desktop/network-services-start.desktop
desktop.files+=$$QTOPIA_DEPOT_PATH/devices/example/src/devtools/desktop/network-services-stop.desktop
desktop.path=/apps/Devtools
desktop.depends+=install_docapi_f_dir
desktop.hint=desktop

pics.files=$$QTOPIA_DEPOT_PATH/devices/example/src/devtools/*.png\
           $$QTOPIA_DEPOT_PATH/devices/example/src/devtools/*.svg
pics.path=/pics/devtools
pics.hint=pics

help.source=$$DEVICE_CONFIG_PATH/help
help.files=qpe-devtools*
help.hint=help

INSTALLS+=desktop pics script startup help

