#the next line prevents non-shadowbuilds from including the same directory twice
#otherwise, the build command would be too long for some win32 shells.
!exists($$QT_BUILD_TREE/tools/designer/src/components/propertyeditor/propertyeditor.h):INCLUDEPATH += $$QT_BUILD_TREE/tools/designer/src/components/propertyeditor

INCLUDEPATH += $$PWD

FORMS += $$PWD/paletteeditor.ui \
    $$PWD/stringlisteditor.ui \
    $$PWD/previewwidget.ui \
    $$PWD/newdynamicpropertydialog.ui \
    $$PWD/qtgradientstopseditor.ui \
    $$PWD/qtgradienteditor.ui \
    $$PWD/qtgradientdialog.ui \
    $$PWD/qtbrusheditor.ui \
    $$PWD/qtbrushdialog.ui \
    $$PWD/qtbrushpatterneditor.ui \
    $$PWD/qtbrushpatterndialog.ui

HEADERS += $$PWD/propertyeditor.h \
    $$PWD/paletteeditor.h \
    $$PWD/paletteeditorbutton.h \
    $$PWD/stringlisteditor.h \
    $$PWD/stringlisteditorbutton.h \
    $$PWD/previewwidget.h \
    $$PWD/previewframe.h \
    $$PWD/newdynamicpropertydialog.h \
    $$PWD/styledbutton.h \
    $$PWD/qtgradientstopsmodel.h\
    $$PWD/qtgradientstopswidget.h\
    $$PWD/qtgradientstopseditor.h\
    $$PWD/qtgradientwidget.h\
    $$PWD/qtgradienteditor.h\
    $$PWD/qtgradientdialog.h\
    $$PWD/qtbrushwidget.h\
    $$PWD/qtbrusheditor.h\
    $$PWD/qtbrushdialog.h\
    $$PWD/qtbrushbutton.h\
    $$PWD/qtbrushpatterneditor.h\
    $$PWD/qtbrushpatterndialog.h\
    $$PWD/qtspinbox.h\
    $$PWD/qtcolorbutton.h\
    $$PWD/qtcolorline.h \
    $$PWD/graphicspropertyeditor.h

SOURCES += $$PWD/propertyeditor.cpp \
    $$PWD/paletteeditor.cpp \
    $$PWD/paletteeditorbutton.cpp \
    $$PWD/stringlisteditor.cpp \
    $$PWD/stringlisteditorbutton.cpp \
    $$PWD/previewwidget.cpp \
    $$PWD/previewframe.cpp \
    $$PWD/newdynamicpropertydialog.cpp \
    $$PWD/styledbutton.cpp \
    $$PWD/qtgradientstopsmodel.cpp\
    $$PWD/qtgradientstopswidget.cpp\
    $$PWD/qtgradientstopseditor.cpp\
    $$PWD/qtgradientwidget.cpp\
    $$PWD/qtgradienteditor.cpp\
    $$PWD/qtgradientdialog.cpp\
    $$PWD/qtbrushwidget.cpp\
    $$PWD/qtbrusheditor.cpp\
    $$PWD/qtbrushdialog.cpp\
    $$PWD/qtbrushbutton.cpp\
    $$PWD/qtbrushpatterneditor.cpp\
    $$PWD/qtbrushpatterndialog.cpp\
    $$PWD/qtspinbox.cpp\
    $$PWD/qtcolorbutton.cpp\
    $$PWD/qtcolorline.cpp \
    $$PWD/graphicspropertyeditor.cpp

HEADERS += $$PWD/qpropertyeditor.h \
    $$PWD/qpropertyeditor_items_p.h \
    $$PWD/qpropertyeditor_model_p.h \
    $$PWD/qpropertyeditor_delegate_p.h \
    $$PWD/propertyeditor_global.h \
    $$PWD/flagbox_p.h \
    $$PWD/flagbox_model_p.h \
    $$PWD/defs.h \
    $$PWD/qlonglongvalidator.h

SOURCES += $$PWD/qpropertyeditor.cpp \
    $$PWD/qpropertyeditor_items.cpp \
    $$PWD/qpropertyeditor_model.cpp \
    $$PWD/qpropertyeditor_delegate.cpp \
    $$PWD/flagbox.cpp \
    $$PWD/flagbox_model.cpp \
    $$PWD/defs.cpp \
    $$PWD/qlonglongvalidator.cpp

RESOURCES += $$PWD/qtgradienteditor.qrc
