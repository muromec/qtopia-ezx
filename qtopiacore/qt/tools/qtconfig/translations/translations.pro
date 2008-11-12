# Include those manually as they do not contain any directory specification

SOURCES        += ../colorbutton.cpp ../main.cpp ../previewframe.cpp ../previewwidget.cpp ../mainwindow.cpp ../paletteeditoradvanced.cpp \
    ../mainwindowbase.cpp ../paletteeditoradvancedbase.cpp ../previewwidgetbase.cpp
HEADERS        += ../colorbutton.h ../previewframe.h ../previewwidget.h ../mainwindow.h ../paletteeditoradvanced.h \
    ../mainwindowbase.h ../paletteeditoradvancedbase.h ../previewwidgetbase.h

FORMS        = ../mainwindowbase.ui ../paletteeditoradvancedbase.ui ../previewwidgetbase.ui

TRANSLATIONS=$$[QT_INSTALL_TRANSLATIONS]/qtconfig_pl.ts \
             $$[QT_INSTALL_TRANSLATIONS]/qtconfig_untranslated.ts \
             $$[QT_INSTALL_TRANSLATIONS]/qtconfig_zh_CN.ts
error("This is a dummy profile to be used for translations ONLY.")
