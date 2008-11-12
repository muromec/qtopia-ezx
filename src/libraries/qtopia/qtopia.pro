qtopia_project(qtopia core lib)
TARGET=qtopia

VERSION=4.0.0
UI_HEADERS_DIR=$$QPEDIR/include/qtopia/private

DEFINES+=QTOPIA_PAGE_SIZE=$$LITERAL_SQUOTE$$QTOPIA_PAGE_SIZE$$LITERAL_SQUOTE
DEFINES+=QTOPIA_PAGE_MASK=$$LITERAL_SQUOTE$$QTOPIA_PAGE_MASK$$LITERAL_SQUOTE
QTOPIA_FORMS+=passwordbase_p.ui

#DEFINES+=EZX_A780

APPS_PATH=../../applications

QTOPIA_HEADERS+=\
    qmimetype.h\
    qdocumentselector.h\
    qtopiaapplication.h\
    qterminationhandler.h\
    qtimezoneselector.h\
    qworldmap.h\
    qpassworddialog.h\
    qtimestring.h\
    qcategorymanager.h\
    qcategoryselector.h\
    qwindowdecorationinterface.h\
    mediarecorderplugininterface.h\
    inputmethodinterface.h\
    qapplicationplugin.h\
    qpluginmanager.h\
    qwaitwidget.h\
    qtimezone.h\
    qthumbnail.h\
    qimagedocumentselector.h\
    qtopiasql.h\
    qiconselector.h\
    inheritedshareddata.h\
    qtopiaserviceselector.h\
    qcolorselector.h\
    qdocumentproperties.h\
    qdevicebutton.h\
    qdevicebuttonmanager.h\
    qdl.h\
    qdlbrowserclient.h\
    qdlclient.h\
    qdleditclient.h\
    qdllink.h\
    qdsaction.h\
    qdsservices.h\
    qdsserviceinfo.h\
    qdsactionrequest.h\
    qdsdata.h\
    qtopianetwork.h\
    qtopianetworkinterface.h\
    qtopiastyle.h\
    qphonestyle.h\
    qsoftmenubar.h\
    qspeeddial.h\
    qstoragedeviceselector.h\
    qconstcstring.h\
    qtopiafeatures.h\
    qdeviceindicators.h\
    themedview.h\
    qphoneprofile.h\
    qhardwareinterface.h\
    qhardwaremanager.h\
    qpowersource.h\
    qbootsourceaccessory.h\
    qvibrateaccessory.h\
    qkeypadlightaccessory.h\
    qtopiasendvia.h\
    qsignalsource.h \
    qanalogclock.h \
    qformlayout.h \
    qanalogclock.h \
    qcontentsortcriteria.h \
    qsoftkeylabelhelper.h \
    qtextentryproxy.h \
    qtopiadocumentserver.h \
    qaudiosourceselector.h \
    qimagesourceselector.h \
    qcontent.h \
    qcontentset.h \
    qdrmcontent.h \
    qcontentplugin.h \
    qcontentfilter.h \
    qdrmcontentplugin.h \
    qdrmrights.h \
    qcontentfiltermodel.h \
    qcontentfilterselector.h\
    qperformancelog.h \
    qdocumentselectorservice.h \
    qtopiaitemdelegate.h

QTOPIA_PRIVATE_HEADERS+=\
    ezxphonestyle_p.h\
    qimagedocumentselector_p.h\
    thumbnailview_p.h\
    singleview_p.h\
    qtopiaresource_p.h\
    qdl_p.h\
    qdlsourceselector_p.h\
    qdlwidgetclient_p.h\
    qds_p.h\
    qdsserviceinfo_p.h\
    qdsactionrequest_p.h\
    themedviewinterface_p.h\
    qdsaction_p.h\
    qdsdata_p.h\
    contextkeymanager_p.h\
    qterminationhandler_p.h \
    qcontent_p.h\
    qhardwareinterface_p.h\
    qworldmap_sun_p.h\
    qworldmap_stylusnorm_p.h \
    qtopiasql_p.h \
    qcontentengine_p.h \
    qcontentstore_p.h \
    qcontentsetengine_p.h \
    qfscontentengine_p.h \
    qsqlcontentsetengine_p.h \
    qdrmcontentengine_p.h \
    qcategorystore_p.h \
    qsqlcategorystore_p.h \
    qmimetypedata_p.h \
    qdocumentserverchannel_p.h \
    qdocumentservercategorystore_p.h \
    qcategorystoreserver_p.h \
    qdocumentservercontentstore_p.h \
    qdocumentservercontentsetengine_p.h \
    qcontentstoreserver_p.h \
    qdocumentservercontentengine_p.h \
    qsqlcontentstore_p.h \
    qsoftkeylabelhelper_p.h \
    qalarmserver_p.h \
    resourcesourceselector_p.h \
    contentserverinterface_p.h \
    drmcontent_p.h \
    contentpluginmanager_p.h \
    qdocumentselectorsocketserver_p.h \
    qcontentfilterselector_p.h \
    qsparselist_p.h \
    qperformancelog_p.h \
    motionpath_p.h \
    qthumbstyle_p.h \
    testslaveinterface_p.h \
    qtagmap_p.h \
    qtopiasqlmigrateplugin_p.h

QTOPIA_SOURCES+=\
    qmimetype.cpp\
    qdocumentselector.cpp\
    qterminationhandler.cpp\
    qtopiaapplication.cpp\
    qtimezoneselector.cpp\
    qworldmap.cpp\
    qworldmap_sun.cpp\
    qalarmserver.cpp\
    qpassworddialog.cpp\
    qtimestring.cpp\
    qcategoryselector.cpp\
    qwindowdecorationinterface.cpp\
    mediarecorderplugininterface.cpp\
    qapplicationplugin.cpp\
    inputmethodinterface.cpp\
    qpluginmanager.cpp\
    qcategorymanager.cpp\
    qwaitwidget.cpp\
    qtimezone.cpp\
    qthumbnail.cpp\
    qimagedocumentselector.cpp\
    qimagedocumentselector_p.cpp\
    thumbnailview_p.cpp\
    singleview_p.cpp\
    qtopiasql.cpp\
    qiconselector.cpp\
    qtopiaresource.cpp\
    qtopiaserviceselector.cpp\
    qcolorselector.cpp\
    qdocumentproperties.cpp\
    qstoragedeviceselector.cpp\
    qdevicebutton.cpp\
    qdevicebuttonmanager.cpp\
    qtopiastyle.cpp\
    qdl.cpp\
    qdlbrowserclient.cpp\
    qdlclient.cpp\
    qdleditclient.cpp\
    qdllink.cpp\
    qdlsourceselector.cpp\
    qdlwidgetclient.cpp\
    qdsaction.cpp\
    qdsservices.cpp\
    qdsserviceinfo.cpp\
    qdsactionrequest.cpp\
    qdsdata.cpp\
    qtopianetwork.cpp\
    qtopianetworkinterfaceimpl.cpp\
    qtopiafeatures.cpp\
    qdeviceindicators.cpp\
    qphoneprofile.cpp\
    qcontent_p.cpp\
    qhardwareinterface.cpp\
    qhardwaremanager.cpp\
    qpowersource.cpp\
    qbootsourceaccessory.cpp\
    qvibrateaccessory.cpp\
    qkeypadlightaccessory.cpp\
    qtopiasendvia.cpp\
    qanalogclock.cpp\
    qsignalsource.cpp \
    qtopiasql_p.cpp\
    qformlayout.cpp \
    qtopiasql_p.cpp \
    qcontentengine.cpp \
    qcontentstore.cpp \
    qsqlcontentstore.cpp \
    qcontentsetengine.cpp \
    qsqlcontentsetengine.cpp \
    qdrmcontentengine.cpp \
    qcategorystore.cpp \
    qsqlcategorystore.cpp \
    qmimetypedata.cpp \
    qdocumentserverchannel.cpp \
    qdocumentservercategorystore.cpp \
    qcategorystoreserver.cpp \
    qdocumentservercontentstore.cpp \
    qdocumentservercontentsetengine.cpp \
    qcontentstoreserver.cpp \
    qdocumentservercontentengine.cpp \
    qcontentsortcriteria.cpp \
    qfscontentengine.cpp \
    qsoftkeylabelhelper.cpp \
    qtextentryproxy.cpp \
    qtopiadocumentserver.cpp \
    qaudiosourceselector.cpp \
    qimagesourceselector.cpp \
    resourcesourceselector.cpp \
    themedview.cpp \
    themedviewinterface.cpp \
    qcontent.cpp \
    qcontentset.cpp \
    contentserverinterface_p.cpp \
    qdrmcontent.cpp \
    drmcontent_p.cpp \
    qcontentplugin.cpp \
    contentpluginmanager_p.cpp \
    qcontentfilter.cpp \
    qdrmcontentplugin.cpp \
    qdrmrights.cpp \
    qcontentfiltermodel.cpp \
    qcontentfilterselector.cpp \
    contextkeymanager.cpp \
    qsoftmenubar.cpp \
    qphonestyle.cpp\
    ezxphonestyle.cpp\
    qspeeddial.cpp\
    qperformancelog.cpp \
    qdocumentselectorservice.cpp \
    qdocumentselectorsocketserver.cpp \
    qtopiaitemdelegate.cpp \
    motionpath_p.cpp \
    qthumbstyle_p.cpp \
    qtopiasqlmigrateplugin.cpp

    QTOPIA_HEADERS+=\
        qexportedbackground.h

!x11 {
    QTOPIA_PRIVATE_HEADERS+=\
        qpedecoration_p.h\
        phonedecoration_p.h
    QTOPIA_SOURCES+=\
        qpedecoration_qws.cpp\
        qexportedbackground.cpp\
        phonedecoration.cpp
} else {
    QTOPIA_SOURCES+=\
        qexportedbackground_x11.cpp
}

depends(3rdparty/libraries/zlib)
depends(3rdparty/libraries/md5)
depends(3rdparty/libraries/sqlite)

RESOURCES=qtopia.qrc

# for clock_gettime
LIBS+=-lrt
idep(LIBS+=-lrt)

enable_greenphone_effects {
    INCLUDEPATH += ../../../include
    LIBS += -L../../../lib -lblend
    idep(LIBS+=-lblend)
    QTOPIA_PRIVATE_HEADERS+=\
        qsmoothlist_p.h
    QTOPIA_SOURCES+=\
        qsmoothlist_p.cpp
}

PREFIX=QTOPIA
resolve_include()

CONFIG += qtopia_visibility

enable_sxe:depends(libraries/qtopiasecurity)


sdk_qtopia_headers.files=$${QTOPIA_HEADERS}
sdk_qtopia_headers.path=/include/qtopia
sdk_qtopia_headers.hint=sdk headers

sdk_qtopia_private_headers.files=$${QTOPIA_PRIVATE_HEADERS}
sdk_qtopia_private_headers.path=/include/qtopia/private
sdk_qtopia_private_headers.hint=sdk headers

INSTALLS+=\
    sdk_qtopia_headers sdk_qtopia_private_headers

etc.files =\
    $$QTOPIA_DEPOT_PATH/etc/colors\
    $$QTOPIA_DEPOT_PATH/etc/mime.types
etc.path=/etc
bins.files=\
    $$QPEDIR/bin/qtopia-addmimetype
bins.path=/bin
bins.hint=script
!phone {
    quitdesktop.files=$$QTOPIA_DEPOT_PATH/apps/Settings/quit.desktop
    quitdesktop.path=/apps/Settings
    quitdesktop.hint=desktop
    INSTALLS+=quitdesktop
}

pics.files=$$QTOPIA_DEPOT_PATH/pics/icons
pics.path=/pics
pics.hint=pics
INSTALLS+=pics

drmpics.files=$$QTOPIA_DEPOT_PATH/pics/drm/*
drmpics.path=/pics/drm
drmpics.hint=pics
INSTALLS+=drmpics

# WorldTime conf is used by qtimezoneselector.
defaults.files=$$QTOPIA_DEPOT_PATH/etc/default/Trolltech/presstick.conf\
               $$QTOPIA_DEPOT_PATH/etc/default/Trolltech/SpeedDial.conf\
               $$QTOPIA_DEPOT_PATH/etc/default/Trolltech/WorldTime.conf\
               $$QTOPIA_DEPOT_PATH/etc/default/Trolltech/Log.conf
defaults.path=/etc/default/Trolltech
INSTALLS+=\
    etc bins defaults

!enable_singleexec {
    textcodecs.files=$$QTOPIA_DEPOT_PATH/plugins/textcodecs/.directory
    textcodecs.path=/plugins/textcodecs/
    INSTALLS+=textcodecs

    imagecodecs.files=$$QTOPIA_DEPOT_PATH/plugins/imagecodecs/.directory
    imagecodecs.path=/plugins/imagecodecs/
    INSTALLS+=imagecodecs

    decorations.files=$$QTOPIA_DEPOT_PATH/plugins/decorations/.directory
    decorations.path=/plugins/decorations/
    INSTALLS+=decorations

    styles.files=$$QTOPIA_DEPOT_PATH/plugins/styles/.directory
    styles.path=/plugins/styles/
    INSTALLS+=styles
}

!isEmpty(STORAGE_CONF_FILE) {
    storage_conf.files=$$STORAGE_CONF_FILE
    storage_conf.path=/etc/default/Trolltech
    INSTALLS+=storage_conf
}
zonetab.files=$$QTOPIA_DEPOT_PATH/etc/zoneinfo/zone.tab
zonetab.trtarget=timezone
# don't automatically install this
zonetab.CONFIG=no_path no_default_install
# We're really only interested in the translations for zone.tab
zonetab.hint=nct
INSTALLS+=zonetab

# This is here so that these get installed before any .desktop files
mainapps_category.files=$$QTOPIA_DEPOT_PATH/apps/MainApplications/.directory
mainapps_category.path=/apps/MainApplications
mainapps_category.hint=desktop

app_categories.files=$$QTOPIA_DEPOT_PATH/apps/Applications/.directory
app_categories.path=/apps/Applications
app_categories.hint=desktop
app_categories.depends=install_docapi_mainapps_category

settings_category.files=$$QTOPIA_DEPOT_PATH/apps/Settings/.directory
settings_category.path=/apps/Settings
settings_category.hint=desktop
settings_category.depends=install_docapi_mainapps_category

INSTALLS+=mainapps_category app_categories settings_category

!platform {
    games_category.files=$$QTOPIA_DEPOT_PATH/apps/Games/.directory
    games_category.path=/apps/Games
    games_category.hint=desktop
    games_category.depends=install_docapi_mainapps_category
    INSTALLS+=games_category
}

# FIXME THIS IS EVIL!!!!!
INCLUDEPATH+=$$QTOPIA_DEPOT_PATH/src/server

pkg.desc =Base Qtopia library.
pkg.domain=trusted

idep(LIBS+=-l$$TARGET)
qt_inc($$TARGET)

# Needed for includes from qtextengine_p.h
INCLUDEPATH+=$$QT_DEPOT_PATH/src/3rdparty/harfbuzz/src
