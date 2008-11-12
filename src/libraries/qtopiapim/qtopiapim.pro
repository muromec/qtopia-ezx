qtopia_project(qtopia lib)
TARGET=qtopiapim
CONFIG+=qtopia_visibility

VERSION         = 4.0.0

RESOURCES = qtopiapim.qrc

enable_cell:depends(libraries/qtopiaphone)
depends(libraries/qtopiacomm)
depends(3rdparty/libraries/inputmatch)
depends(3rdparty/libraries/sqlite)
depends(3rdparty/libraries/vobject)

QTOPIAPIM_HEADERS+=\
    qpimrecord.h\
    qtask.h\
    qappointment.h\
    qcontact.h\
    qpimmodel.h\
    qtaskmodel.h\
    qappointmentmodel.h\
    qcontactmodel.h\
    qappointmentview.h\
    qcontactview.h\
    qtaskview.h\
    qpimsource.h\
    qpimsourcemodel.h\
    qpimsourcedialog.h\
    qpimdelegate.h\
    qphonenumber.h

QTOPIAPIM_PRIVATE_HEADERS+=\
    qannotator_p.h\
    qsqlpimtablemodel_p.h\
    qtaskio_p.h\
    qappointmentio_p.h\
    qappointmentsqlio_p.h\
    qcontactio_p.h\
    qpimsqlio_p.h\
    qpreparedquery_p.h\
    qrecordio_p.h\
    qtasksqlio_p.h\
    qcontactsqlio_p.h\
    qrecordiomerge_p.h\
    qdependentcontexts_p.h\
    qpimdependencylist_p.h

QTOPIAPIM_SOURCES+=\
    qannotator.cpp\
    qsqlpimtablemodel.cpp\
    qpreparedquery.cpp\
    qpimrecord.cpp\
    qtaskio.cpp\
    qcontactio.cpp\
    qappointmentio.cpp\
    qtask.cpp\
    qappointment.cpp\
    qappointmentsqlio.cpp\
    qcontact.cpp\
    qpimmodel.cpp\
    qtaskmodel.cpp\
    qcontactmodel.cpp\
    qappointmentmodel.cpp\
    qappointmentview.cpp\
    qcontactview.cpp\
    qtaskview.cpp\
    qpimsource.cpp\
    qpimsqlio.cpp\
    qtasksqlio.cpp\
    qcontactsqlio.cpp\
    qrecordiomerge.cpp\
    qpimsourcemodel.cpp\
    qpimsourcedialog.cpp\
    qphonenumber.cpp\
    qpimdelegate.cpp\
    qdependentcontexts.cpp\
    qpimdependencylist.cpp



QTOPIAPIM_PRIVATE_HEADERS+=qgooglecontext_p.h
QTOPIAPIM_SOURCES+=qgooglecontext.cpp

#DEFINES+=GOOGLE_CALENDAR_CONTEXT

enable_cell {
    QTOPIAPIM_PRIVATE_HEADERS+=qsimcontext_p.h
    QTOPIAPIM_SOURCES+=qsimcontext.cpp
}

TRANSLATABLES+=\
    qsimcontext_p.h\
    qsimcontext.cpp\
    qgooglecontext_p.h\
    qgooglecontext.cpp

PREFIX=QTOPIAPIM
resolve_include()

sdk_qtopiapim_headers.files=$${QTOPIAPIM_HEADERS}
sdk_qtopiapim_headers.path=/include/qtopia/pim
sdk_qtopiapim_headers.hint=sdk headers
INSTALLS+=sdk_qtopiapim_headers

sdk_qtopiapim_private_headers.files=$${QTOPIAPIM_PRIVATE_HEADERS}
sdk_qtopiapim_private_headers.path=/include/qtopia/pim/private
sdk_qtopiapim_private_headers.hint=sdk headers
INSTALLS+=sdk_qtopiapim_private_headers

pkg_qtopiapim_settings.files=$$QTOPIA_DEPOT_PATH/etc/default/Trolltech/Contacts.conf
pkg_qtopiapim_settings.path=/etc/default/Trolltech
INSTALLS+=pkg_qtopiapim_settings

apics.files=$$QTOPIA_DEPOT_PATH/pics/addressbook/*
apics.path=/pics/addressbook
apics.hint=pics

dpics.files=$$QTOPIA_DEPOT_PATH/pics/datebook/*
dpics.path=/pics/datebook
dpics.hint=pics

tpics.files=$$QTOPIA_DEPOT_PATH/pics/todolist/*
tpics.path=/pics/todolist
tpics.hint=pics

INSTALLS+=apics dpics tpics



pkg.desc=PIM Data access library
pkg.domain=trusted

idep(LIBS+=-l$$TARGET)
qt_inc($$TARGET)
