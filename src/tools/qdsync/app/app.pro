qtopia_project(qtopia app)
TARGET=qdsync
CONFIG+=no_tr singleexec_main

#CONFIG+=qtopia_main no_tr no_quicklaunch
## Force no quicklaunch (because this is a daemon and it can start slow, unlike other things at startup time)
#CONFIG-=force_quicklaunch

VPATH+=..

SOURCES+=\
    main.cpp\

pkg.desc=Synchronization
pkg.domain=trusted
pkg.multi=\
    tools/qdsync/base\
    tools/qdsync/common\
    tools/qdsync/pim

desktop.files=../qdsync.desktop
desktop.path=/apps/Applications
desktop.hint=desktop
INSTALLS+=desktop

help.source=../help
help.files=qdsync.html
help.hint=help
INSTALLS+=help

pics.files=../pics/*
pics.path=/pics/qdsync
pics.hint=pics
INSTALLS+=pics
