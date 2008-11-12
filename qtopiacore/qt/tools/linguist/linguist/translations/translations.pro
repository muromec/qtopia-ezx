# Include those manually as they do not contain any directory specification

include(../previewtool/previewtool.pri)

FORMS    += ../statistics.ui \
            ../phrasebookbox.ui \
            ../batchtranslation.ui \
            ../translatedialog.ui \
            ../mainwindow.ui \
            ../translationsettings.ui \
            ../finddialog.ui

SOURCES        += ../finddialog.cpp \
  ../translatedialog.cpp \
        ../batchtranslationdialog.cpp \
        ../translationsettingsdialog.cpp \
  ../statistics.cpp \
  ../messagemodel.cpp \
  ../phrasemodel.cpp \
  ../msgedit.cpp \
        ../main.cpp \
        ../phrase.cpp \
        ../phrasebookbox.cpp \
        ../printout.cpp \
        ../trwindow.cpp \
        ../messagestreeview.cpp \
        ../../shared/xliff.cpp \
        ../../shared/simtexth.cpp \
  ../../shared/metatranslator.cpp \
  ../../shared/translator.cpp

HEADERS        += ../finddialog.h \
  ../translatedialog.h \
        ../batchtranslationdialog.h \
        ../translationsettingsdialog.h \
  ../statistics.h \
  ../messagemodel.h \
  ../phrasemodel.h \
  ../msgedit.h \
        ../phrase.h \
        ../phrasebookbox.h \
        ../printout.h \
        ../trwindow.h \
        ../messagestreeview.h \
        ../../shared/xliff.h \
        ../../shared/simtexth.h \
        ../../shared/metatranslator.h \
  ../../shared/translator.h

TRANSLATIONS=$$[QT_INSTALL_TRANSLATIONS]/linguist_ja.ts \
             $$[QT_INSTALL_TRANSLATIONS]/linguist_pl.ts \
             $$[QT_INSTALL_TRANSLATIONS]/linguist_untranslated.ts \
             $$[QT_INSTALL_TRANSLATIONS]/linguist_tr_TR.ts \
             $$[QT_INSTALL_TRANSLATIONS]/linguist_zh_CN.ts \
             $$[QT_INSTALL_TRANSLATIONS]/linguist_zh_TW.ts
error("This is a dummy profile to be used for translations ONLY.")
