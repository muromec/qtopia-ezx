qtopia_project(qtopia lib)
TARGET=qtopiaprinting

FORMS += printdialogbase.ui

HEADERS += qprinterinterface.h

SOURCES += qprinterinterface.cpp
           
unix:!x11 {
    PRIVATE_HEADERS += qprintdialogcreator_p.h
    SOURCES += qprintdialogcreator.cpp
}

TRANSLATABLES*=$$FORMS $$HEADERS $$SOURCES

sdk_qtopiaprinting_headers.files=$${HEADERS}
sdk_qtopiaprinting_headers.path=/include/qtopia/printing
sdk_qtopiaprinting_headers.hint=sdk headers
INSTALLS+=sdk_qtopiaprinting_headers

unix:!x11 {
    sdk_qtopiaprinting_private_headers.files=$${PRIVATE_HEADERS}
    sdk_qtopiaprinting_private_headers.path=/include/qtopia/printing/private
    sdk_qtopiaprinting_private_headers.hint=sdk headers
    INSTALLS+=sdk_qtopiaprinting_private_headers
}

HEADERS += $$PRIVATE_HEADERS

service.files=$$QTOPIA_DEPOT_PATH/services/Printing/qpe
service.path=/services/Printing
INSTALLS+=service


idep(LIBS+=-l$$TARGET)
qt_inc($$TARGET)
