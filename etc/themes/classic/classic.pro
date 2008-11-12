qtopia_project(theme)

classicconf.files=$$QTOPIA_DEPOT_PATH/etc/themes/classic.conf
classicconf.path=/etc/themes
classicconf.trtarget=Qtopia
classicconf.hint=themecfg
classicconf.outdir=$$PWD
INSTALLS+=classicconf
classicdata.files=$$QTOPIA_DEPOT_PATH/etc/themes/classic/*.xml $$QTOPIA_DEPOT_PATH/etc/themes/classic/*rc
classicdata.path=/etc/themes/classic
INSTALLS+=classicdata
classicpics.files=$$QTOPIA_DEPOT_PATH/pics/themes/classic/*
classicpics.path=/pics/themes/classic
classicpics.hint=pics
INSTALLS+=classicpics


classicbgimage.files=$$QTOPIA_DEPOT_PATH/pics/themes/classic/background.png
classicbgimage.path=/pics/themes/classic
classicbgimage.hint=background
# let this install first so we can overwrite the image
classicbgimage.depends=install_classicpics
INSTALLS+=classicbgimage

pkg.name=qpe-theme-classic
pkg.desc=Qtopia Classic theme
pkg.domain=trusted
