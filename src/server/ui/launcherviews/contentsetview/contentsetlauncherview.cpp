/****************************************************************************
**
** This file is part of the Qtopia Opensource Edition Package.
**
** Copyright (C) 2008 Trolltech ASA.
**
** Contact: Qt Extended Information (info@qtextended.org)
**
** This file may be used under the terms of the GNU General Public License
** versions 2.0 as published by the Free Software Foundation and appearing
** in the file LICENSE.GPL included in the packaging of this file.
**
** Please review the following information to ensure GNU General Public
** Licensing requirements will be met:
**     http://www.fsf.org/licensing/licenses/info/GPLv2.html.
**
**
****************************************************************************/

#include "contentsetlauncherview.h"
#include <qdocumentproperties.h>
#include <qtopiaapplication.h>
#include <QSoftMenuBar>
#include <QtDebug>
#include <QAction>
#include <QMenu>

/*!
    \class ContentSetLauncherView
    \brief Displays a content set received by the "ContentSetView" service.
    \internal
    
    This class is part of the Qtopia server and cannot be used by other Qtopia applications.
*/

/*!
    Constructs a new content set launcher view with the given \a parent and \a flags.
*/
ContentSetLauncherView::ContentSetLauncherView( QWidget* parent, Qt::WFlags flags )
    : LauncherView( parent, flags )
    , propDlg( 0 )
{
    connect( ContentSetViewService::instance(), SIGNAL(showContentSet()),
             this, SLOT(showContentSet()) );

    showContentSet();

    QMenu *softMenu = QSoftMenuBar::menuFor( this );

    propertiesAction = new QAction( QIcon(":icon/info"), tr("Properties..."), this );

    QObject::connect( propertiesAction, SIGNAL(triggered()), this, SLOT(showProperties()) );

    softMenu->addAction( propertiesAction );
}

void ContentSetLauncherView::showContentSet()
{
    setWindowTitle( ContentSetViewService::instance()->title() );
    *contentSet = ContentSetViewService::instance()->contentSet();
}

void ContentSetLauncherView::showProperties()
{
    const QContent &doc = currentItem();
    if (doc.id() != QContent::InvalidId && doc.isValid()) {
        propLnk = doc;
        if (propDlg)
            delete propDlg;
        propDlg = new QDocumentPropertiesDialog(propLnk, this);
        propDlg->setObjectName("document-properties");
        QtopiaApplication::showDialog(propDlg);
        propDlg->setWindowState(propDlg->windowState() | Qt::WindowMaximized);
    }
}

/*!
    \service ContentSetViewService ContentSetView
    \brief Provides the Qtopia ContentSetView service.

    The \i ContentSetView service raises a launcher menu on the homescreen which displays a custom QContentSet.

    This class is part of the Qtopia server and cannot be used by other Qtopia applications.
*/


/*!
    Creates a new content set view service.
*/
ContentSetViewService::ContentSetViewService()
    : QtopiaAbstractService( "ContentSetView" )
{
    publishAll();
}

/*!
    \internal
*/
ContentSetViewService::~ContentSetViewService()
{
}

/*!
    Returns a pointer to a static instance of ContentSetViewService.
    \internal
*/
ContentSetViewService *ContentSetViewService::instance()
{
    static ContentSetViewService *instance = 0;

    if( !instance )
        instance = new ContentSetViewService();

    return instance;
}

/*!
    Returns the title associated with the last content set a show request was received for.
    \internal
*/
QString ContentSetViewService::title() const
{
    return m_title;
}

/*!
    Returns the last content set a show request was received for.
    \internal
*/
QContentSet ContentSetViewService::contentSet() const
{
    return m_contentSet;
}

/*!
    Requests that a content \a set be displayed on the home screen.

    This slot corresponds to the QCop service message \c {ContentSetView::showContentSet(QContentSet)}.
*/
void ContentSetViewService::showContentSet( const QContentSet &set )
{
    m_title = tr( "Documents" );
    m_contentSet = set;

    emit showContentSet();
}

/*!
    Requests that a content \a set be displayed on the home screen with the title \a title.

    This slot corresponds to the QCop service message \c {ContentSetView::showContentSet(QString,QContentSet)}.
 */
void ContentSetViewService::showContentSet( const QString &title, const QContentSet &set )
{
    m_title = title;
    m_contentSet = set;

    emit showContentSet();
}

/*!
    \fn ContentSetViewService::showContentSet()
    \internal

    Signals that a request to display a content set has been received.  The content set
    can be retrieved using \c ContentSetViewService::contentSet() and the title of the set
    using \c ContentSetViewService::title().
*/
