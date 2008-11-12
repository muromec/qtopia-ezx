qtopia_project(theme)

smartconf.files=$$QTOPIA_DEPOT_PATH/etc/themes/smart.conf
smartconf.path=/etc/themes
smartconf.trtarget=Smart
smartconf.hint=themecfg
smartconf.outdir=$$PWD
INSTALLS+=smartconf
smartdata.files=$$QTOPIA_DEPOT_PATH/etc/themes/smart/*.xml $$QTOPIA_DEPOT_PATH/etc/themes/smart/*rc
smartdata.path=/etc/themes/smart
INSTALLS+=smartdata
smartpics.files=$$QTOPIA_DEPOT_PATH/pics/themes/smart/*
smartpics.path=/pics/themes/smart
smartpics.hint=pics
INSTALLS+=smartpics


smartbgimage.files=$$QTOPIA_DEPOT_PATH/pics/themes/smart/background.png
smartbgimage.path=/pics/themes/smart
smartbgimage.hint=background
# let this install first so we can overwrite the image
smartbgimage.depends=install_smartpics
INSTALLS+=smartbgimage

pkg.name=qpe-theme-smart
pkg.desc=Smart theme
pkg.domain=trusted
