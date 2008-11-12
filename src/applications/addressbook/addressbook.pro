qtopia_project(qtopia app)
TARGET=addressbook
CONFIG+=qtopia_main

HEADERS+=\
    abeditor.h\
    contactdetails.h\
    contactsource.h\
    contactdocument.h\
    contactbrowser.h\
    contactmessagehistorylist.h\
    contactoverview.h\
    contactlistpane.h\
    addressbook.h\
    groupview.h\
    emaildialogphone.h

SOURCES+=\
    abeditor.cpp\
    contactdetails.cpp\
    addressbook.cpp\
    contactsource.cpp\
    groupview.cpp\
    contactdocument.cpp\
    contactmessagehistorylist.cpp\
    contactbrowser.cpp\
    contactoverview.cpp\
    contactlistpane.cpp\
    emaildialogphone.cpp\
    main.cpp\

FORMS += actiondialog.ui

depends(libraries/qtopiaphone)

enable_telephony {
    !enable_singleexec {
        SOURCES += ../../settings/ringprofile/ringtoneeditor.cpp
        HEADERS += ../../settings/ringprofile/ringtoneeditor.h
    }
    HEADERS += contactcallhistorylist.h
    SOURCES += contactcallhistorylist.cpp
}

!enable_singleexec {
    SOURCES += ../todo/reminderpicker.cpp ../todo/qdelayedscrollarea.cpp
    HEADERS += ../todo/reminderpicker.h ../todo/qdelayedscrollarea.h
}

TRANSLATABLES += emaildialogphone.cpp \
                    emaildialogphone.h \
                    actiondialog.ui \
                    ../../settings/ringprofile/ringtoneeditor.cpp \
                    ../../settings/ringprofile/ringtoneeditor.h 

depends(libraries/qtopiapim)
depends(libraries/qtopiamail)

service.files=$$QTOPIA_DEPOT_PATH/services/Contacts/addressbook
service.path=/services/Contacts
INSTALLS+=service

receiveservice.files=$$QTOPIA_DEPOT_PATH/services/Receive/text/x-vcard/addressbook
receiveservice.path=/services/Receive/text/x-vcard/
INSTALLS+=receiveservice

desktop.files=$$QTOPIA_DEPOT_PATH/apps/Applications/addressbook.desktop
desktop.path=/apps/Applications
desktop.hint=desktop
INSTALLS+=desktop

help.source=$$QTOPIA_DEPOT_PATH/help
help.files=addressbook*
help.hint=help
INSTALLS+=help

# pics are installed by libqtopiapim since they're shared

im.files=named_addressbook-*.conf
im.path=/etc/im/pkim
INSTALLS+=im

qdlservice.files=$$QTOPIA_DEPOT_PATH/services/QDL/addressbook
qdlservice.path=/services/QDL
qdsservice.files=$$QTOPIA_DEPOT_PATH/etc/qds/Contacts
qdsservice.path=/etc/qds
INSTALLS+=qdsservice

enable_cell {
    phoneservice.files=$$QTOPIA_DEPOT_PATH/services/ContactsPhone/addressbook
    phoneservice.path=/services/ContactsPhone
    INSTALLS+=phoneservice

    qdsphoneservice.files=$$QTOPIA_DEPOT_PATH/etc/qds/ContactsPhone
    qdsphoneservice.path=/etc/qds
    INSTALLS+=qdsphoneservice
}

pkg.desc=Contacts for Qtopia.
pkg.domain=trusted
