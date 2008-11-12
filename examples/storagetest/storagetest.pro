qtopia_project(app qtopia)
TARGET=storagetest
CONFIG+=no_tr no_singleexec no_sxe_test
SOURCES=main.cpp

pkg.name=storagetest
pkg.desc=This is a command line tool useful to enumerate the Document paths known to Qtopia
pkg.version=1.0.0-1
pkg.maintainer=Trolltech (www.trolltech.com)
pkg.license=GPL
pkg.domain=trusted
