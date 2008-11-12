qtopia_project(theme)

finxiconf.files=$$QTOPIA_DEPOT_PATH/etc/themes/finxi.conf
finxiconf.path=/etc/themes
finxiconf.trtarget=Qtopia
finxiconf.hint=themecfg
finxiconf.outdir=$$PWD
INSTALLS+=finxiconf
finxidata.files=$$QTOPIA_DEPOT_PATH/etc/themes/finxi/*.xml $$QTOPIA_DEPOT_PATH/etc/themes/finxi/*rc
finxidata.path=/etc/themes/finxi
INSTALLS+=finxidata
finxipics.files=$$QTOPIA_DEPOT_PATH/pics/themes/finxi/*
finxipics.path=/pics/themes/finxi
finxipics.hint=pics
INSTALLS+=finxipics


finxibgimage.files=$$QTOPIA_DEPOT_PATH/pics/themes/finxi/background.png
finxibgimage.path=/pics/themes/finxi
finxibgimage.hint=background
# let this install first so we can overwrite the image
finxibgimage.depends=install_finxipics
INSTALLS+=finxibgimage

pkg.name=qpe-theme-finxi
pkg.desc=finxi theme
pkg.domain=trusted
