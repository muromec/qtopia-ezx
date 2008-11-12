qtopia_project(qtopia lib)
TARGET=qtopiaphone

HEADERS	=   qphonecall.h \
            qphonecallmanager.h \
            qphonecallprovider.h \
	    qcalllist.h \
            qphonestatus.h \
            qtelephonyservice.h \
            qtelephonynamespace.h \
            qnetworkregistration.h \
            qpresence.h \
            qdialoptions.h \
            qtelephonyconfiguration.h \
            qservicechecker.h \
            qservicenumbers.h \
            qcallvolume.h \
            wap/qwsppdu.h \
            wap/qwbxmlreader.h \
            wap/qotareader.h \
            wap/qcacheoperationreader.h \
            wap/qserviceindicationreader.h \
            wap/qserviceloadingreader.h \
            wap/qwmlreader.h

PRIVATE_HEADERS += \
            qphonecall_p.h \
            qphonecallmanager_p.h

SOURCES	=   qphonecall.cpp \
            qphonecallmanager.cpp \
            qphonecallprovider.cpp \
	    qcalllist.cpp \
            qphonestatus.cpp \
            qtelephonyservice.cpp \
            qtelephonynamespace.cpp \
            qnetworkregistration.cpp \
            qpresence.cpp \
            qdialoptions.cpp \
            qtelephonyconfiguration.cpp \
            qservicechecker.cpp \
            qservicenumbers.cpp \
            qcallvolume.cpp \
            wap/qwsppdu.cpp \
            wap/qwbxmlreader.cpp \
            wap/qotareader.cpp \
            wap/qcacheoperationreader.cpp \
            wap/qserviceindicationreader.cpp \
            wap/qserviceloadingreader.cpp \
            wap/qwmlreader.cpp

enable_cell {

    HEADERS += \
	    qphonebook.h \
	    qsimtoolkit.h \
	    qsimcommand.h \
	    qsimterminalresponse.h \
            qsimenvelope.h \
            qsimcontrolevent.h \
	    qsmsmessage.h \
	    qsmsreader.h \
            qpreferrednetworkoperators.h \
            qcallbarring.h \
            qcallforwarding.h \
            qcallsettings.h \
            qsmssender.h \
            qcbsmessage.h \
            qpinmanager.h \
            qsimfiles.h \
            qsimgenericaccess.h \
            qbinarysimfile.h \
            qrecordbasedsimfile.h \
            qsimiconreader.h \
            qcellbroadcast.h \
            qsupplementaryservices.h \
            qsiminfo.h \
            qadviceofcharge.h \
            qphonerffunctionality.h \
            qbandselection.h \
            qgprsnetworkregistration.h

    PRIVATE_HEADERS += \
	    qsmsmessage_p.h \
            qsimiconreader_p.h

    SOURCES += \
	    qphonebook.cpp \
	    qsimtoolkit.cpp \
	    qsimcommand.cpp \
	    qsimterminalresponse.cpp \
            qsimenvelope.cpp \
            qsimcontrolevent.cpp \
	    qsmsmessage.cpp \
	    qsmsreader.cpp \
            qpreferrednetworkoperators.cpp \
            qcallbarring.cpp \
            qcallforwarding.cpp \
            qcallsettings.cpp \
            qsmssender.cpp \
            qcbsmessage.cpp \
            qpinmanager.cpp \
            qsimfiles.cpp \
            qsimgenericaccess.cpp \
            qbinarysimfile.cpp \
            qrecordbasedsimfile.cpp \
            qsimiconreader.cpp \
            qcellbroadcast.cpp \
            qsupplementaryservices.cpp \
            qsiminfo.cpp \
            qadviceofcharge.cpp \
            qphonerffunctionality.cpp \
            qbandselection.cpp \
            qgprsnetworkregistration.cpp

}

CONFIG += qtopia_visibility
    
depends(libraries/qtopiacomm/serial)

sdk_qtopiaphone_headers.files=$${HEADERS}
sdk_qtopiaphone_headers.path=/include/qtopiaphone
sdk_qtopiaphone_headers.hint=sdk headers
INSTALLS+=sdk_qtopiaphone_headers

sdk_qtopiaphone_private_headers.files=$${PRIVATE_HEADERS}
sdk_qtopiaphone_private_headers.path=/include/qtopiaphone/private
sdk_qtopiaphone_private_headers.hint=sdk headers
INSTALLS+=sdk_qtopiaphone_private_headers


HEADERS+=$$PRIVATE_HEADERS

idep(LIBS+=-l$$TARGET)
qt_inc($$TARGET)
