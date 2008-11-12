qtopia_project(qtopia lib)
TARGET=qtopiamail

CONFIG+=qtopia_visibility

enable_cell:contains(PROJECTS,libraries/qtopiasmil):CONFIG+=enable_mms
else:DEFINES+=QTOPIA_NO_SMS QTOPIA_NO_MMS

RESOURCES = qtopiamail.qrc

HEADERS+=\
    qmailaddress.h\
    qmailcodec.h\
    qmailmessage.h\
    qmailtimestamp.h\
    longstring_p.h \
    longstream_p.h\
    qmailfolder.h\
    qmailfolderkey.h\
	qmailfolderkey_p.h\
	qmailfoldersortkey.h\
	qmailfoldersortkey_p.h\
    qmailstore.h\
	qmailstore_p.h\
    qmailmessagekey.h\
	qmailmessagekey_p.h\
	qmailmessagesortkey.h\
	qmailmessagesortkey_p.h\
    qmailcomposer.h\
    qmailcomposerplugin.h\
    qmailviewer.h\
    qmailviewerplugin.h\
    qmailid.h\
	mailbodystore_p.h\
    qprivateimplementation_p.h\
    qprivateimplementationdef_p.h

SOURCES+=\
    qmailaddress.cpp\
    qmailcodec.cpp\
    qmailmessage.cpp\
    qmailtimestamp.cpp\
    longstring.cpp \
    longstream.cpp\
    qmailfolder.cpp\
    qmailfolderkey.cpp\
	qmailfoldersortkey.cpp\
	qmailmessagesortkey.cpp\
    qmailstore.cpp\
	qmailstore_p.cpp\
    qmailmessagekey.cpp\
    qmailcomposer.cpp\
    qmailcomposerplugin.cpp\
    qmailviewer.cpp\
    qmailviewerplugin.cpp\
    qmailid.cpp\
	mailbodystore.cpp\
    qprivateimplementation.cpp

depends(libraries/qtopiapim)

sdk_qtopiamail_headers.files=$${HEADERS}
sdk_qtopiamail_headers.path=/include/qtopia/mail
sdk_qtopiamail_headers.hint=sdk headers
INSTALLS+=sdk_qtopiamail_headers

pkg.desc=Allows access to the Qtopia mailbox files.

idep(LIBS+=-l$$TARGET)
qt_inc($$TARGET)
