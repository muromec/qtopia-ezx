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

#include <QFile>
#include <QRegExp>
#include <QLayout>
#include <QDesktopWidget>
#include <QSettings>
#include <QStringList>
#include <QComboBox>

#include <stdlib.h>

#include "qtimezone.h"
#include "qworldmap.h"
#include "qtimezoneselector.h"
#include "qtopiaapplication.h"
#include <qsoftmenubar.h>

#include <qtopiachannel.h>

// ============================================================================
//
// QTimeZoneComboBox
//
// ============================================================================

class QTimeZoneComboBox : public QComboBox
{
    Q_OBJECT
public:
    explicit QTimeZoneComboBox( QWidget* parent=0 );
    ~QTimeZoneComboBox();

    QString currZone() const;
    QString prevZone() const;
    void setCurrZone( const QString& id );

public slots:
    void setToPreviousIndex();

protected:
    friend class QTimeZoneSelector;
    void updateZones();

private slots:
    void handleSystemChannel(const QString&, const QByteArray&);
    void indexChange( const int index );

private:
    QSettings   cfg;
    QStringList identifiers;
    QStringList extras;
    int         prevIndex1;
    int         prevIndex2;
};

// ============================================================================
//
// QWorldmapDialog
//
// ============================================================================

class QWorldmapDialog : public QDialog
{
    Q_OBJECT;

public:
    QWorldmapDialog( QWidget* parent = 0, Qt::WFlags f = 0 );

    void setZone( const QTimeZone& zone );
    QTimeZone selectedZone() const;

public slots:
    int exec();
    void selected();
    void selected( const QTimeZone& zone );
    void cancelled();

private:
    QWorldmap* mMap;
    QTimeZone mZone;
};

// ============================================================================
//
// QTimeZoneSelectorPrivate
//
// ============================================================================

class QTimeZoneSelectorPrivate : public QObject
{
    Q_OBJECT
public:
    QTimeZoneSelectorPrivate(QTimeZoneSelector *p) : QObject(0), q(p), includeLocal(false) {}
    QTimeZoneSelector *q;
    QTimeZoneComboBox *cmbTz;
    bool includeLocal;

    void showWorldmapDialog();
};


// ============================================================================
//
// QTimeZoneComboBox
//
// ============================================================================

/*!
  \internal
*/
QTimeZoneComboBox::QTimeZoneComboBox( QWidget *p )
:   QComboBox( p ),
    cfg("Trolltech","WorldTime"),
    prevIndex1( 0 ),
    prevIndex2( 0 )
{
    cfg.beginGroup("TimeZones");
    updateZones();
    prevIndex1 = prevIndex2 = currentIndex();

    connect( qobject_cast<QComboBox*>( this ),
             SIGNAL(currentIndexChanged(int)),
             this,
             SLOT(indexChange(int)) );

    // listen on QPE/System
    QtopiaChannel *channel = new QtopiaChannel( "QPE/System", this );
    connect( channel, SIGNAL(received(QString,QByteArray)),
        this, SLOT(handleSystemChannel(QString,QByteArray)) );
}

/*!
  \internal
*/
QTimeZoneComboBox::~QTimeZoneComboBox()
{
}

/*!
  \internal
*/
void QTimeZoneComboBox::updateZones()
{
    QString cur = currentText();
    clear();
    identifiers.clear();
    int curix=0;
    QString tz = getenv("TZ");
    bool tzFound = false; // found the current timezone.
    bool hasCur = !cur.isEmpty();
    int listIndex = 0;
    QStringList comboList;
    if (QTimeZoneSelectorPrivate *tzsp = qobject_cast<QTimeZoneSelectorPrivate*>(parent())) {
        if ( tzsp->includeLocal ) {
            // overide to the 'local' type.
            identifiers.append( "None" ); // No tr
            comboList << tr("None");
            if ( cur == tr("None") || !hasCur )
                curix = 0;
            listIndex++;
        }
    }
    int cfgIndex = 0;
    while (1) {
        QString zn = cfg.value("Zone"+QString::number(cfgIndex), QString()).toString();
        if ( zn.isNull() )
            break;
        if ( zn == tz )
            tzFound = true;

        QString nm = QTimeZone( zn.toLatin1() ).name();
        identifiers.append(zn);
        comboList << nm;
        if ( nm == cur || (!hasCur && zn == tz) )
            curix = listIndex;
        ++cfgIndex;
        ++listIndex;
    }
    for (QStringList::Iterator it=extras.begin(); it!=extras.end(); ++it) {
        QTimeZone z( (*it).toLatin1() );
        comboList << z.name();
        identifiers.append(*it);
        if ( *it == cur  || (!hasCur && *it == tz) )
            curix = listIndex;
        ++listIndex;
    }
    if ( !tzFound && !tz.isEmpty()) {
        identifiers.append(tz);
        QString nm = QTimeZone(tz.toLatin1()).name();
        comboList << nm;
        if ( nm == cur  || !hasCur )
            curix = listIndex;
        ++listIndex;
    }
    comboList << tr("More...");
    addItems(comboList);

    setCurrentIndex(curix);
    emit activated(curix);
}

/*!
  \internal
*/
void QTimeZoneComboBox::indexChange( const int index )
{
    prevIndex2 = prevIndex1;
    if ( index < 0 )
        prevIndex1 = 0;
    else
        prevIndex1 = index;
}

/*!
  \internal
*/
void QTimeZoneComboBox::setToPreviousIndex()
{
    setCurrentIndex( prevIndex2 );
}

/*!
  \internal
*/
QString QTimeZoneComboBox::prevZone() const
{
    if (identifiers.count())
        return identifiers[prevIndex2];
    return QString();
}

/*!
  \internal
*/
QString QTimeZoneComboBox::currZone() const
{
    if (identifiers.count() && identifiers.count() > currentIndex()) // Not "More..."
        return identifiers[currentIndex()];
    return QString();
}

/*!
  \internal
*/
void QTimeZoneComboBox::setCurrZone( const QString& id )
{
    for (int i=0; i<identifiers.count(); i++) {
        if ( identifiers[i] == id ) {
            if ( currentIndex() != i ) {
                setCurrentIndex(i);
                emit activated(i);
            }
            return;
        }
    }

    QString name = QTimeZone(id.toLatin1()).name();
    int index = count() - 1;
    if ( index > 0 ) {
        insertItem( index, name );
        setCurrentIndex( index );
        identifiers.append(id);
        extras.append(id);
        emit activated( index );
    }
}

/*!
  \internal
*/
void QTimeZoneComboBox::handleSystemChannel(const QString&msg, const QByteArray&)
{
    if ( msg == "timeZoneListChange()" ) {
        updateZones();
    }
}

// ============================================================================
//
// QWorldmapDialog
//
// ============================================================================

/*!
    \internal
*/
QWorldmapDialog::QWorldmapDialog( QWidget* parent, Qt::WFlags f )
:   QDialog( parent, f | Qt::FramelessWindowHint ),
    mMap( 0 ),
    mZone()
{
    setWindowTitle( tr( "Select Time Zone" ) );

    QVBoxLayout *bl = new QVBoxLayout(this);
    mMap = new QWorldmap(this);
    QSizePolicy sp = mMap->sizePolicy();
    sp.setHeightForWidth(true);
    mMap->setSizePolicy(sp);
    bl->addWidget( mMap );
    bl->setSpacing( 4 );
    bl->setMargin( 4 );

    setMinimumWidth( qApp->desktop()->width() );
    setMinimumHeight( qApp->desktop()->height() / 2 );
    setMaximumHeight( qApp->desktop()->height() / 2 );

    QSoftMenuBar::setLabel( this, Qt::Key_Back, QSoftMenuBar::Cancel );

    if( !Qtopia::mousePreferred()) {
    connect( mMap,
             SIGNAL(newZone(QTimeZone)),
             this,
             SLOT(selected(QTimeZone)) );
    } else {
        connect( mMap,
                 SIGNAL(buttonSelected()),
                 this,
                 SLOT(selected()) );
    }

    connect( mMap,
             SIGNAL(selectZoneCanceled()),
             this,
             SLOT(cancelled()) );

    if( !Qtopia::mousePreferred()) mMap->setFocus();
}

/*!
    \internal
*/
int QWorldmapDialog::exec()
{
    if ( isHidden() )
        show();

    if ( mZone.isValid() )
        mMap->setZone( mZone );

    mMap->selectNewZone();
    return QDialog::exec();
}

/*!
    \internal
*/
void QWorldmapDialog::setZone( const QTimeZone& zone )
{
    mZone = zone;
}

/*!
    \internal
*/
void QWorldmapDialog::selected( const QTimeZone& zone )
{
    if ( zone.isValid() ) {
        mZone = zone;
        accept();
    } else {
        reject();
    }
}

/*!
\internal
*/
void QWorldmapDialog::selected( )
{
    qLog(Time)<<"QWorldmapDialog::selected( )";
    if ( mMap->zone().isValid() ) {
        mZone = mMap->zone();
        accept();
    } else {
        reject();
    }
}

/*!
    \internal
*/
void QWorldmapDialog::cancelled()
{
    reject();
}

/*!
    \internal
*/
QTimeZone QWorldmapDialog::selectedZone() const
{
    return mZone;
}

// ============================================================================
//
// QTimeZoneSelectorPrivate
//
// ============================================================================

/*!
  \internal
*/
void QTimeZoneSelectorPrivate::showWorldmapDialog( void )
{
    QWorldmapDialog* map = new QWorldmapDialog( q );

    if ( cmbTz->prevZone().isEmpty() || ( cmbTz->prevZone() == "None" ) )
        map->setZone( QTimeZone(getenv( "TZ" )) );
    else
        map->setZone( QTimeZone( cmbTz->prevZone().toLatin1() ) );

    map->setModal( true );
    if ( map->exec() == QDialog::Accepted && map->selectedZone().isValid() ) {
        cmbTz->setCurrZone( map->selectedZone().id() );
    } else {
        cmbTz->setToPreviousIndex();
    }
}

// ============================================================================
//
// QTimeZoneSelector
//
// ============================================================================

/*!
  \class QTimeZoneSelector
  \mainclass

  \brief The QTimeZoneSelector class provides a widget for selecting a time zone.
  \ingroup time

  QTimeZoneSelector presents a list of common city names, corresponding to time zones, for the user to choose from.
  A "More..." option allows the user to access all time zones via a world map.

  The cities included in the initial list are determined by the configuration file \c WorldTime.conf.
  This file has the following format:

  \code
  [TimeZones]
  Zone0=Area/Location
  ...
  ZoneN=Area/Location
  \endcode

  where each Area/Location is a \l {http://www.twinsun.com/tz/tz-link.htm}{zoneinfo} identifier. For example:

  \code
  [TimeZones]
  Zone0=America/New_York
  Zone1=America/Los_Angeles
  Zone2=Europe/Oslo
  ...
  \endcode

  The selector list also includes the current time zone, regardless of whether or not that time zone
  is represented by a city in the configuration file.

  \sa QTimeZone
*/

/*!
    Creates a new QTimeZoneSelector with parent \a p.  The selector will be
    populated with the appropriate timezones (see \l {Detailed Description}
    for an explanation of which zones will be included).
*/
QTimeZoneSelector::QTimeZoneSelector(QWidget* p) :
    QWidget(p)
{
    QHBoxLayout *hbl = new QHBoxLayout(this);
    hbl->setMargin(0);
#ifndef QT_NO_TRANSLATION
    static int transLoaded = 0;
    if (!transLoaded) {
        QtopiaApplication::loadTranslations("timezone");
        transLoaded++;
    }
#endif
    d = new QTimeZoneSelectorPrivate(this);

    // build the combobox before we do any updates...
    d->cmbTz = new QTimeZoneComboBox( this );
    d->cmbTz->setObjectName( "timezone combo" );
    hbl->addWidget(d->cmbTz);

    setFocusPolicy(Qt::TabFocus);
    setFocusProxy(d->cmbTz);

    // set up a connection to catch a newly selected item and throw our
    // signal
    QObject::connect( d->cmbTz, SIGNAL(activated(int)),
                      this, SLOT(tzActivated(int)) );
}

/*!
  Destroys the QTimeZoneSelector.
*/
QTimeZoneSelector::~QTimeZoneSelector()
{
    delete d;
}

/*!
  If \a b is true, allow no timezone ("None") as an option.
  If \a b is false then a specific timezone must be selected.
*/
void QTimeZoneSelector::setAllowNoZone(bool b)
{
    d->includeLocal = b;
    d->cmbTz->updateZones();
}

/*!
  \property QTimeZoneSelector::allowNoZone
  \brief true if no timezone is included as an option; false otherwise.
*/

/*!
  Returns true if no timezone ("None") is included as an option; otherwise returns false.
*/
bool QTimeZoneSelector::allowNoZone() const
{
    return d->includeLocal;
}

/*!
  \property QTimeZoneSelector::currentZone
  \brief the currently selected timezone.
*/

/*!
  Returns the currently selected timezone as a string in Area/Location format, for example,
  \code Australia/Brisbane \endcode
*/
QString QTimeZoneSelector::currentZone() const
{
    return d->cmbTz->currZone();
}

/*!
  Sets the current timezone to \a id. The \a id should be
  a \l QString in Area/Location format, for example, \code Australia/Brisbane \endcode
*/
void QTimeZoneSelector::setCurrentZone( const QString& id )
{
    d->cmbTz->setCurrZone( id );
}

/*! \fn void QTimeZoneSelector::zoneChanged( const QString& id )
  This signal is emitted whenever the time zone is changed.
  The \a id
  is a \l QString in Area/Location format, for example, \code Australia/Brisbane \endcode
*/

/*!
  \internal
*/
void QTimeZoneSelector::tzActivated( int idx )
{
    if (idx == d->cmbTz->count()-1) {
        d->showWorldmapDialog();
    } else {
        emit zoneChanged( d->cmbTz->currZone() );
    }
}

#include "qtimezoneselector.moc"
