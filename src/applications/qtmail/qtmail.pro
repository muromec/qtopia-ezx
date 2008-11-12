qtopia_project(qtopia app)
TARGET=qtmail
CONFIG+=qtopia_main

enable_cell:contains(PROJECTS,libraries/qtopiasmil):CONFIG+=enable_mms
else:DEFINES+=QTOPIA_NO_SMS QTOPIA_NO_MMS

FORMS_MMS = mmseditaccountbase.ui
FORMS = editaccountbasephone.ui searchviewbasephone.ui

enable_mms {
    FORMS += $$FORMS_MMS
    qtopia_depot:DEFINES+=ENABLE_UNCONDITIONAL_MMS_SEND
}

HEADERS+=\
    account.h\
    accountlist.h\
    accountsettings.h\
    actionlistview.h\
    addresslist.h\
    client.h\
    detailspage.h\
    editaccount.h\
    emailclient.h\
    emailfolderlist.h\
    emailhandler.h\
    emaillistitem.h\
    emailpropertysetter.h\
    folder.h\
    folderlistview.h\
    imapclient.h\
    imapprotocol.h\
    maillist.h\
    maillistview.h\
    mailtransport.h\
    popclient.h\
    qtmailwindow.h\
    readmail.h\
    search.h\
    searchview.h\
    selectfolder.h\
    smsclient.h\
    smsdecoder.h\
    smtpclient.h\
    statusdisplay.h\
    viewatt.h\
    writemail.h

SOURCES+=\
    account.cpp\
    accountlist.cpp\
    accountsettings.cpp\
    actionlistview.cpp\
    addresslist.cpp\
    client.cpp\
    detailspage.cpp\
    editaccount.cpp\
    emailclient.cpp\
    emailfolderlist.cpp\
    emailhandler.cpp\
    emaillistitem.cpp\
    emailpropertysetter.cpp\
    folder.cpp\
    folderlistview.cpp\
    imapclient.cpp\
    imapprotocol.cpp\
    maillist.cpp\
    maillistview.cpp\
    mailtransport.cpp\
    main.cpp\
    popclient.cpp\
    qtmailwindow.cpp\
    readmail.cpp\
    search.cpp\
    searchview.cpp\
    selectfolder.cpp\
    smsclient.cpp\
    smsdecoder.cpp\
    smtpclient.cpp\
    statusdisplay.cpp\
    viewatt.cpp\
    writemail.cpp

MMS_SOURCES=\
    mmsclient.cpp\
    mmscomms.cpp\
    mmseditaccount.cpp\
    mmsmessage.cpp

MMS_HEADERS=\
    mmsclient.h\
    mmscomms.h\
    mmseditaccount.h\
    mmsmessage.h

enable_mms:HEADERS+=$$MMS_HEADERS
enable_mms:SOURCES+=$$MMS_SOURCES

enable_mms {
# To enable HTTP MMS comms:
    MMSCOMMS_HEADERS=mmscomms_http.cpp
    MMSCOMMS_SOURCES=mmscomms_http.h
    SOURCES+=$$MMSCOMMS_HEADERS
    HEADERS+=$$MMSCOMMS_SOURCES

    DEFINES+=MMSCOMMS_HTTP
}

TRANSLATABLES +=    $$HEADERS\
                    $$SOURCES\
                    $$MMS_HEADERS\
                    $$MMS_SOURCES\
                    $$FORMS\
                    $$FORMS_MMS\
                    $$MMSCOMMS_HEADERS\
                    $$MMSCOMMS_SOURCES

depends(libraries/qtopiamail)
depends(libraries/qtopiapim)

pics.files=$$QTOPIA_DEPOT_PATH/pics/qtmail/*
pics.path=/pics/qtmail
pics.hint=pics
INSTALLS+=pics
desktop.files=$$QTOPIA_DEPOT_PATH/apps/Applications/qtmail.desktop
desktop.path=/apps/Applications
desktop.hint=desktop
INSTALLS+=desktop
emailservice.files=$$QTOPIA_DEPOT_PATH/services/Email/qtmail
emailservice.path=/services/Email
INSTALLS+=emailservice
qdsemailservice.files=$$QTOPIA_DEPOT_PATH/etc/qds/Email
qdsemailservice.path=/etc/qds
INSTALLS+=qdsemailservice
help.source=$$QTOPIA_DEPOT_PATH/help
help.files=qtmail*
help.hint=help
INSTALLS+=help

smsservice.files=$$QTOPIA_DEPOT_PATH/services/SMS/qtmail
smsservice.path=/services/SMS
enable_cell:INSTALLS+=smsservice
messageservice.files=$$QTOPIA_DEPOT_PATH/services/Messages/qtmail
messageservice.path=/services/Messages
INSTALLS+=messageservice
qdssmsservice.files=$$QTOPIA_DEPOT_PATH/etc/qds/SMS
qdssmsservice.path=/etc/qds
enable_cell:INSTALLS+=qdssmsservice

pkg.name=qpe-mail
pkg.desc=Messaging application for Qtopia.
pkg.domain=trusted
