TEMPLATE = app
LANGUAGE = C++
QT += xml network

CONFIG        += qt warn_on assistant uitools
build_all:!build_pass {
    CONFIG -= build_all
    CONFIG += release
}

include(previewtool/previewtool.pri)

SOURCES        += finddialog.cpp \
	translatedialog.cpp \
        batchtranslationdialog.cpp \
        translationsettingsdialog.cpp \
	statistics.cpp \
	messagemodel.cpp \
	phrasemodel.cpp \
	msgedit.cpp \
        main.cpp \
        phrase.cpp \
        phrasebookbox.cpp \
        printout.cpp \
        trwindow.cpp \
        messagestreeview.cpp \
        ../shared/xliff.cpp \
        ../shared/simtexth.cpp \
	../shared/metatranslator.cpp \
	../shared/translator.cpp
HEADERS        += finddialog.h \
	translatedialog.h \
        batchtranslationdialog.h \
        translationsettingsdialog.h \
	statistics.h \
	messagemodel.h \
	phrasemodel.h \
	msgedit.h \
        phrase.h \
        phrasebookbox.h \
        printout.h \
        trwindow.h \
        messagestreeview.h \
        ../shared/xliff.h \
        ../shared/simtexth.h \
        ../shared/metatranslator.h \
	../shared/translator.h

contains(QT_PRODUCT, OpenSource.*):DEFINES *= QT_OPENSOURCE
DEFINES        += QT_KEYWORDS

DESTDIR                = ../../../bin
TARGET                = linguist

win32:RC_FILE        = linguist.rc

mac {
    static:CONFIG -= global_init_link_order
    ICON = linguist.icns
    TARGET = Linguist
}

PROJECTNAME        = Qt Linguist

target.path=$$[QT_INSTALL_BINS]
INSTALLS        += target

linguisttranslations.files = *.qm
linguisttranslations.path = $$[QT_INSTALL_TRANSLATIONS]
INSTALLS += linguisttranslations

phrasebooks.path=$$[QT_INSTALL_DATA]/phrasebooks
### will this work on windows?
phrasebooks.files = $$QT_SOURCE_TREE/tools/linguist/phrasebooks/*
INSTALLS += phrasebooks
FORMS    += statistics.ui \
            phrasebookbox.ui \
            batchtranslation.ui \
            translatedialog.ui \
            mainwindow.ui \
            translationsettings.ui \
            finddialog.ui
INCLUDEPATH        += ../shared ../../assistant/lib
RESOURCES += linguist.qrc
