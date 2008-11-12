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

// Local includes
#include "homescreenwidgets.h"

// Qt includes
#include <QHBoxLayout>
#include <QButtonGroup>
#include <QToolButton>
#include <QUniqueId>
#include <QDate>
#include <QTimer>
#include <QPainter>
#include <QStyle>

// Qtopia includes
#include <QSoftMenuBar>
#include <QContentSet>
#include <qtopialog.h>
#include <QTimeString>
#include <QtopiaServiceRequest>

// ============================================================================
//
// HomeScreenWidgets
//
// ============================================================================

/*!
    \class HomeScreenWidgets
    \brief The HomeScreenWidgets class is a factory for homescreen widgets.

    The HomescreenWidgets class is used as a factory for creating homescreen
    widgets that have registered. Homescreen widgets are registered with the
    HSWIDGET macro, which takes the name of the widget and the method used
    to create the widget. Typically this will be the name of the widget
    class and the constructor of the class, for example:

    \code
    HSWIDGET(WorldmapHSWidget, WorldmapHSWidget)
    \endcode

    This class is part of the Qtopia server and cannot be used by other Qtopia applications.
    \sa AnalogClockHSWidget, AppointmentsHSWidget, LauncherHSWidget, WorldmapHSWidget
*/


/*!
    \typedef HomeScreenWidgets::CreateHSWidgetFunc

    \internal
*/

QMap<QString, HomeScreenWidgets::CreateHSWidgetFunc> HomeScreenWidgets::mWidgets;

/*!
    Creates the homescreen widget with \a name (case insensitive) and attaches
    it to \a parent. If no widget has been registered with \a name then 0 is
    returned.
*/
QWidget* HomeScreenWidgets::create( const QString& name, QWidget* parent )
{
    QWidget* ret = 0;

    QMap<QString, CreateHSWidgetFunc>::iterator it
        = mWidgets.find( name.toLower() );

    if ( it != mWidgets.end() ) {
        ret = (*it)( 0 );
        ret->setParent( parent );
    }

    return ret;
}

/*!
    \internal

    The HSWIDGET macro uses this static method to add the homescreen widget
    with \a name (case insensitive) and creating \a function.
*/
void HomeScreenWidgets::add( const char* name, CreateHSWidgetFunc function )
{
    QString cname( name );
    mWidgets[cname.toLower()] = function;
}

// ============================================================================
//
// LauncherIcon
//
// ============================================================================

/*!
    \internal
*/
LauncherIcon::LauncherIcon( const QContent& app, QWidget* parent )
:   QWidget( parent ),
    mApp( app )
{
    setFocusPolicy( Qt::StrongFocus );
}

/*!
    \internal
*/
void LauncherIcon::paintEvent( QPaintEvent* /*event*/ )
{
    QStyle *style = QWidget::style();
    QPainter painter(this);

    QRect cr = contentsRect();
    int maxSize = qMin( cr.width(), cr.height() );

    QPixmap pix;
    if ( hasFocus() ) {
        pix = mApp.icon().pixmap( maxSize );
    } else {
        pix = mApp.icon().pixmap( ( 2 * maxSize ) / 3 );
    }

    style->drawItemPixmap( &painter, cr, Qt::AlignCenter, pix );
}

/*!
    \internal
*/
void LauncherIcon::keyPressEvent( QKeyEvent* event )
{
    switch ( event->key() ) {
        case Qt::Key_Select:
        case Qt::Key_Space:
        {
            event->accept();
            mApp.execute();
            break;
        }
        case Qt::Key_Left:
        {
            event->accept();
            if ( QApplication::layoutDirection() == Qt::LeftToRight )
                focusPreviousChild();
            else
                focusNextChild();
            break;
        }
        case Qt::Key_Right:
        {
            event->accept();
            if ( QApplication::layoutDirection() == Qt::LeftToRight )
                focusNextChild();
            else
                focusPreviousChild();
            break;
        }
        default:
        {
            QWidget::keyPressEvent( event );
            break;
        }
    }
}

/*!
    \internal
*/
void LauncherIcon::launch()
{
    mApp.execute();
}

// ============================================================================
//
// LauncherHSWidget
//
// ============================================================================

/*!
    \class LauncherHSWidget
    \brief The LauncherHSWidget widget launches applications.

    The LauncherHSWidget is a homescreen widget that adds application icons to
    the homescreen so that applications can be launched directly. The
    LauncherHSWidget is added to the homescreen theme including
    the widget theme element in the theme XML file, for example:

    \code
    <widget name="LauncherHSWidget" rect="0,0,0x30" />
    \endcode

    By default LauncherHSWidget will display icons for Contacts, Calendar, Tasks
    and Worldtime applications. This can be modified in
    \c{<Qtopia Runtime Prefix>/Settings/Trolltech/HomeScreenWidgets.conf} file,
    for example:

    \code
    ...
    [LauncherHSWidget]
    num=4
    applications="addressbook;datebook;todolist;worldtime"
    ...
    \endcode

    LauncherHSWidget will select the first \c num valid applications from
    \c applications.

    \sa HomeScreenWidgets
*/

HSWIDGET(LauncherHSWidget, LauncherHSWidget)

/*!
    \internal
*/
LauncherHSWidget::LauncherHSWidget( QWidget* parent )
:   QWidget( parent ),
    mApps()
{
    // Setup focus related stuff
    QSoftMenuBar::setLabel( this, Qt::Key_Select, QSoftMenuBar::Select );
    if ( Qtopia::mousePreferred() )
        setFocusPolicy( Qt::NoFocus );

    // Create the buttons, add them to the group and lay them out
    QHBoxLayout* layout = new QHBoxLayout( this );

    QSettings config( "Trolltech", "HomeScreenWidgets" );
    int numApps = 0;
    QStringList appList;
    if ( !config.childGroups().contains( "LauncherHSWidget" ) ) {
        numApps = 4;
        appList.append( "addressbook" );
        appList.append( "datebook" );
        appList.append( "todolist" );
        appList.append( "worldtime" );
    } else {
        config.beginGroup( "LauncherHSWidget" );
        numApps = config.value( "num", 4 ).toInt();
        QString apps = config.value( "applications" ).toString();
        config.endGroup();

        appList = apps.split( ";" );
    }

    QStringList::const_iterator appCit = appList.begin();
    QContentFilter filter( QContent::Application );
    QContentSet set( filter );
    for ( int i=0; i<numApps; ) {
        if ( appCit == appList.end() )
            break;

        QContent app = set.findExecutable( *appCit );
        if ( !app.isValid() ) {
            qLog(UI) << "LauncherHSWidget can't add invalid application:"
                    << *appCit;
            ++appCit;
            continue;
        }

        ++appCit;
        LauncherIcon* icon = new LauncherIcon( app, this );
        layout->addWidget( icon );

        mApps.append( app );
        mIcons.append( icon );

        ++i;
    }

    setLayout( layout );
}

/*!
    \internal
*/
void LauncherHSWidget::launch()
{
    foreach( LauncherIcon* icon, mIcons ) {
        QRect rs = icon->geometry();
        if ( rs.contains( mapFromGlobal( QCursor::pos() ) ) )
            icon->launch();
    }
}

// ============================================================================
//
// AppointmentsHSWidget
//
// ============================================================================

/*!
    \class AppointmentsHSWidget
    \brief The AppointmentsHSWidget widget provides information about the next or current appointment.

    The AppointmentsHSWidget is a homescreen widget that provides information
    about the next of current appointment entered in the Calendar application.
    This informtation is loaded into the valuespace so that the theme can
    present the information in a style consistent with the theme. The
    AppointmentsHSWidget is added to the homescreen theme including the widget
    theme element in the theme XML file as well as theme items for displaying
    the information, for example:

    \code
    <rect name="appointment" rect="0,0,0x60" bold="yes" color="Text" size="12" brush="None" transient="yes" active="yes" interactive="yes">
        <widget name="AppointmentsHSWidget"/>
        <layout name="appointmentsInfo" orientation="vertical" spacing="0">
            <text name="appointmenttitle" rect="0,0,0x30" bold="yes" color="Text" size="12" align="left,vcenter">
                expr: " " . @/PIM/Appointments/Next/title
            </text>
            <text name="appointmentlocandtime" rect="0,0,0x30" bold="yes" color="Text" size="12" align="left,vcenter">
                expr: " " . @/PIM/Appointments/Next/location . " " . @/PIM/Appointments/Next/time
            </text>
        </layout>
    </rect>
    \endcode

    \sa HomeScreenWidgets
*/

HSWIDGET(AppointmentsHSWidget, AppointmentsHSWidget)

/*!
    \internal
*/
AppointmentsHSWidget::AppointmentsHSWidget( QWidget* parent )
:   QWidget( parent ),
    mVsObject( 0 ),
    mVsItem( 0 ),
    mUid(),
    mDate(),
    mModel( 0 )
{
    // Connect system events which cause appointments to update()
    mVsItem = new QValueSpaceItem( "/UI/DisplayTime/Time", this );
    connect( mVsItem,
             SIGNAL(contentsChanged()),
             this,
             SLOT(update()) );

    connect( qApp,
             SIGNAL(timeChanged()),
             this,
             SLOT(update()) );

    connect( qApp,
             SIGNAL(clockChanged(bool)),
             this,
             SLOT(update()) );

    // Create the value space object and set default values
    mVsObject = new QValueSpaceObject( "/PIM/Appointments/Next", this );
    mVsObject->setAttribute( "title", tr( "Loading appointments..." ) );
    mVsObject->setAttribute( "none", true );
    mVsObject->setAttribute( "location", "" );
    mVsObject->setAttribute( "time", "");

    update();
}

/*!
    \internal
*/
void AppointmentsHSWidget::update()
{
    if ( !updateModel() )
        return;

    if ( mModel->count() == 0 ) {
        mVsObject->setAttribute( "title", tr( "No appointments today" ) );
        mVsObject->setAttribute( "none", true );
        mVsObject->setAttribute( "location", "" );
        mVsObject->setAttribute( "time", "");
        return;
    }

    int index = 0;
    const QDateTime currentDateTime = QDateTime::currentDateTime();
    QOccurrence nextOccurrence = mModel->occurrence( index );
    while ( nextOccurrence.isValid() &&
            ( nextOccurrence.end() <= currentDateTime ) ) {
        ++index;
        nextOccurrence = mModel->occurrence( index );
    }

    if ( nextOccurrence.isValid() ) {
        mUid = nextOccurrence.uid();
        mDate = nextOccurrence.date();
        mVsObject->setAttribute( "title", QVariant( nextOccurrence.description() ) );
        mVsObject->setAttribute( "none", false );
        mVsObject->setAttribute( "location", QVariant( nextOccurrence.location() ) );
        if ( nextOccurrence.startInCurrentTZ() <= currentDateTime ) {
            // Appointment is in progress
            int past = nextOccurrence.startInCurrentTZ().secsTo(
                currentDateTime ) / 60;
            mVsObject->setAttribute( "time", appProgress( past ) );
        } else {
            //Appointment is scheduled in the future
            mVsObject->setAttribute( "time", appScheduled( nextOccurrence ) );
        }
    } else {
        // No valid appointment
        mUid = QUniqueId();
        mDate = QDate();
        mVsObject->setAttribute( "title", tr( "No appointments today" ) );
        mVsObject->setAttribute( "none", true );
        mVsObject->setAttribute( "location", "" );
        mVsObject->setAttribute( "time", "");
        return;
    }
}

/*!
    \internal
*/
bool AppointmentsHSWidget::updateModel()
{
    if ( mModel == 0 ) {
        const QDateTime currentDateTime = QDateTime::currentDateTime();
        QDateTime start( currentDateTime );
        QDateTime end( currentDateTime.addDays(1) );
        mModel = new QOccurrenceModel( start, end, this );

        connect( mModel,
                 SIGNAL(rowsInserted(QModelIndex,int,int)),
                 this,
                 SLOT(update()) );

        connect( mModel,
                 SIGNAL(rowsRemoved(QModelIndex,int,int)),
                 this,
                 SLOT(update()) );

        connect( mModel,
                 SIGNAL(fetchCompleted()),
                 this,
                 SLOT(update()) );

        return false;
    } else {
        const QDateTime currentDateTime = QDateTime::currentDateTime();
        if ( ( mModel->rangeStart().addSecs( 5 * 60 ) ) <
             currentDateTime ) {
            mModel->setRange( currentDateTime, currentDateTime.addDays(1) );

            // Need to wait for model to complete the fetch for appointments
            // before we fresh the value space with the next appointment.
            return false;
        }
    }

    return true;
}

/*!
    \internal
*/
QString AppointmentsHSWidget::appProgress( const int minutesPast )
{
    QString started;
    int past = minutesPast;
    if ( past == 0 ) {
        started = tr( "Starting now" );
    } else if ( past < 60 ) {
        if ( past == 1 )
            started = tr( "Started 1 minute ago" );
        else
            started = tr( "Started %1 minutes ago" );
        started = started.arg( past );
    } else if ( past < 60 * 24 ) {
        past /= 60;
        if ( past == 1 )
            started = tr( "Started 1 hour ago" );
        else
            started = tr( "Started %1 hours ago" );
        started = started.arg( past );
    } else {
        past /= 60 * 24;
        if ( past == 1 )
            started = tr( "Started 1 day ago" );
        else
            started = tr( "Started %1 days ago" );
        started = started.arg( past );
    }

    return started;
}

/*!
    \internal
*/
QString AppointmentsHSWidget::appScheduled( const QOccurrence& occurence )
{
    QString scheduled;
    const QDateTime currentDateTime = QDateTime::currentDateTime();
    QString time = QTimeString::localHM( occurence.startInCurrentTZ().time() );
    if ( occurence.startInCurrentTZ().date() == currentDateTime.date() ) {
        scheduled = tr( "Today" ) + QLatin1String(" ");
        scheduled += time;
    } else if ( occurence.startInCurrentTZ().date() ==
                currentDateTime.date().addDays( 1 ) ) {
        scheduled = tr( "Tomorrow" ) + QLatin1String(" ");
        scheduled += time;
    } else {
        scheduled = occurence.startInCurrentTZ().toString();
    }

    return scheduled;
}

/*!
    \internal
*/
void AppointmentsHSWidget::showNextAppointment()
{
    if ( !mUid.isNull() && mDate.isValid() ) {
        QtopiaServiceRequest request( "Calendar",
                                      "showAppointment(QUniqueId,QDate)" );
        request << mUid;
        request << mDate;
        request.send();
    }
}

// ============================================================================
//
// WorldmapHSWidget
//
// ============================================================================

/*!
    \class WorldmapHSWidget
    \brief The WorldmapHSWidget widget displays a worldmap.

    The WorldmapHSWidget is a homescreen widget that displays a worldmap showing
    the currently selected timezone and a daylight mask. The WorldmapHSWidget is
    added to the homescreen theme including the widget theme element in the
    theme XML file, for example:

    \code
    <widget name="WorldmapHSWidget" rect="0,0,0x90" />
    \endcode

    \sa HomeScreenWidgets
*/

HSWIDGET(WorldmapHSWidget, WorldmapHSWidget)

/*!
    \internal
*/
WorldmapHSWidget::WorldmapHSWidget( QWidget* parent )
:   QWorldmap( parent ),
    mCheck( 0 )
{
    QSizePolicy sp = sizePolicy();
    sp.setHeightForWidth( true );
    setSizePolicy( sp );

    setReadOnly();
}

/*!
    \internal
*/
void WorldmapHSWidget::showCity()
{
    setZone( mapFromGlobal( QCursor::pos() ) );
    ++mCheck;
    QTimer::singleShot( 5000, this, SLOT(showTZ()) );
}

/*!
    \internal
*/
void WorldmapHSWidget::showTZ()
{
    if ( mCheck > 0 )
        --mCheck;
    repaint();
}

/*!
    \internal
*/
void WorldmapHSWidget::paintEvent( QPaintEvent *event )
{
    if ( mCheck == 0 ) {
        QTimeZone tz( getenv( "TZ" ) );
        if ( tz.isValid() && ( tz != zone() ) && isVisible() ) {
            setZone( tz );
        }
    }

    QWorldmap::paintEvent( event );
}

// ============================================================================
//
// AnalogClockHSWidget
//
// ============================================================================

/*!
    \class AnalogClockHSWidget
    \brief The AnalogClockHSWidget widget displays the current time.

    The AnalogClockHSWidget is a homescreen widget that displays the current
    time using an analog clock face. The AnalogClockHSWidget is
    added to the homescreen theme including the widget theme element in the
    theme XML file, for example:

    \code
    <widget name="AnalogClockHSWidget" rect="0,0,0x45" />
    \endcode

    The image used for the analog clock face can be modified in the
    \c{<Qtopia Runtime Prefix>/Settings/Trolltech/HomeScreenWidgets.conf} file,
    for example:

    \code
    ...
    [AnalogClockHSWidget]
    face="myclockface"
    ...
    \endcode

    \sa HomeScreenWidgets
*/

HSWIDGET(AnalogClockHSWidget, AnalogClockHSWidget)

/*!
    \internal
*/
AnalogClockHSWidget::AnalogClockHSWidget( QWidget* parent )
:   QAnalogClock( parent )
{
    QTimer* timer = new QTimer( this );
    connect( timer,
             SIGNAL(timeout()),
             this,
             SLOT(update()) );
    timer->start( 1000 );

    QSettings config( "Trolltech", "HomeScreenWidgets" );
    config.beginGroup( "AnalogClockHSWidget" );
    QString face = config.value( "face", "clock/background" ).toString();
    config.endGroup();

    setFace( QPixmap( ":image/" + face ) );
    display( QTime::currentTime() );
}

/*!
    \internal
*/
void AnalogClockHSWidget::update()
{
    display( QTime::currentTime() );
}

