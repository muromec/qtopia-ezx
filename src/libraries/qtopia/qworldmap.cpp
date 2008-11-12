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
#include "qworldmap.h"
#include "qworldmap_sun_p.h"
#include "qworldmap_stylusnorm_p.h"

// Qt4 Headers
#include <QFrame>
#include <QPixmap>
#include <QScrollBar>
#include <QDateTime>
#include <QImage>
#include <QMessageBox>
#include <QPainter>
#include <QTimer>
#include <QToolButton>
#include <QAbstractScrollArea>
#include <QPoint>
#include <QDesktopWidget>
#include <QPixmapCache>

// Qtopia includes
#include <qtimestring.h>
#include <qtimezone.h>
#include <qtopiaapplication.h>
#include <qtopialog.h>
#include <qtopia/qsoftmenubar.h>

// System includes
#include <limits.h>

// Constants
static const char* const QWORLDMAP_IMAGEFILE   = "worldtime/simple_grid_400";

static const int  QWORLDMAP_LABELOFFSET = 15;
static const int  QWORLDMAP_CITYSIZE    = 4;
static const int  QWORLDMAP_CITYOFFSET  = 2;
static const int  QWORLDMAP_ACCELRATE   = 5;
static const int  QWORLDMAP_ZOOM   = 4;
static const int  QWORLDMAP_CITYLABEL_TIMEOUT   = 60000;

// Function declarations
static void darken( QImage *pImage, int start, int stop, int row );
static void dayNight( QImage *pImage );

// ============================================================================
//
// StylusNormalizer
//
// ============================================================================

static const int FLUSHTIME = 100;

_StylusEvent::_StylusEvent( const QPoint& newPt )
    : _pt( newPt ),
      _t( QTime::currentTime() )
{
}

_StylusEvent::~_StylusEvent()
{
}

StylusNormalizer::StylusNormalizer( QWidget *parent )
    : QWidget( parent ),
      _next( 0 ),
      bFirst( true )
{
    // initialize _ptList
    int i;
    for (i = 0; i < SAMPLES; i++ ) {
        _ptList[i].setPoint( -1, -1 );
    }
    _tExpire = new QTimer( this );
    QObject::connect( _tExpire, SIGNAL(timeout()),
                      this, SLOT(slotAveragePoint()));
}

StylusNormalizer::~StylusNormalizer()
{
}

void StylusNormalizer::addEvent( const QPoint& pt )
{
    _ptList[_next].setPoint( pt );
    _ptList[_next++].setTime( QTime::currentTime() );
    if ( _next >= SAMPLES ) {
        _next = 0;
    }
    // make a single mouse click work
    if ( bFirst ) {
        slotAveragePoint();
        bFirst = false;
    }
}

void StylusNormalizer::slotAveragePoint( void )
{
    QPoint pt( 0, 0 );
    QTime tCurr = QTime::currentTime();
    int i,
        size;
    size = 0;
    for ( i = 0; i < SAMPLES; i++ ) {
        if ( ( (_ptList[i]).time().msecsTo( tCurr ) < FLUSHTIME ) &&
             ( _ptList[i].point() != QPoint( -1, -1 ) ) ) {
            pt += _ptList[i].point();
            size++;
        }
    }
    if ( size > 0 )
        emit signalNewPoint( pt /= size );
}

void StylusNormalizer::start( void )
{
    _tExpire->start( FLUSHTIME );
}

void StylusNormalizer::stop( void )
{
    _tExpire->stop();
    bFirst = true;
}

// ============================================================================
//
// CityPos
//
// ============================================================================

struct CityPos
{
    int lat;
    int lon;
    QString id;
};

// ============================================================================
//
// ZoomButton
//
// ============================================================================

class ZoomButton : public QToolButton
{
public:
    ZoomButton( QWidget *parent = 0 );

protected:
    virtual void focusInEvent( QFocusEvent * );
    virtual void focusOutEvent( QFocusEvent * );
};

ZoomButton::ZoomButton( QWidget *parent )
: QToolButton( parent )
{
}

void ZoomButton::focusInEvent(QFocusEvent *e)
{
    if ( QApplication::keypadNavigationEnabled() ) {
        QPalette pal;
        pal.setColor( QPalette::Button, pal.color(QPalette::Highlight) );
        setPalette( pal );
    }
    QToolButton::focusInEvent( e );
}

void ZoomButton::focusOutEvent( QFocusEvent *e )
{
    if ( QApplication::keypadNavigationEnabled() ) {
        QPalette pal;
        setPalette( pal );
    }
    QToolButton::focusOutEvent( e );
}

// ============================================================================
//
// CityLabel
//
// ============================================================================

class CityLabel : public QLabel
{
public:
    CityLabel(const QString& text, QWidget *parent = 0);

protected:
    void mouseMoveEvent(QMouseEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
};

CityLabel::CityLabel(const QString& text, QWidget *parent)
    : QLabel(text, parent)
{
    setMinimumSize(sizeHint());
    setFrameStyle(QFrame::Plain | QFrame::Box);
    setFont(QApplication::font());

    QPalette pal(palette());
    QColor col = pal.color(QPalette::Highlight);
    col.setAlpha(192);
    pal.setColor(QPalette::Background, col);
    pal.setColor(QPalette::WindowText, Qt::black);
    setPalette(pal);
    setAutoFillBackground(true);
    hide();
}

void CityLabel::mouseMoveEvent(QMouseEvent *event)
{
    event->ignore();
}

void CityLabel::mousePressEvent(QMouseEvent *event)
{
    event->ignore();
}

void CityLabel::mouseReleaseEvent(QMouseEvent *event)
{
    event->ignore();
}

// ============================================================================
//
// QWorldmapPrivate
//
// ============================================================================

class QWorldmapPrivate
{
public:

    QWorldmapPrivate();
    ~QWorldmapPrivate();

    // convert between the pixels on the image and the coordinates in the
    // database
    bool zoneToWin( int      zoneX,
                    int      zoneY,
                    int&     winX,
                    int&     winY,
                    QWidget* viewport,
                    bool     adjust = true ) const;
    bool winToZone( int winX, int winY, int &zoneX, int &zoneY ) const;

    // Data members
    QPixmap*     pixCurr; // image to be drawn on the screen
    CityLabel*   lblCity; // the "tool-tip" that shows up when you pick a city...
    ZoomButton*  cmdZoom;   // our zoom option...
    ZoomButton*  selecButton;
    ZoomButton*  cancelButton;

    QTimer*      tHide;  // the timer to hide the "tool tip"
    QTimer * lblCityTimer; //clock timer for tooltip
    QTimeZone    m_last;   // the last known good city that was found...
    QTimeZone    m_repaint; // save the location to maximize the repaint...

    StylusNormalizer norm;

    //the True width and height of the map...
    int wImg;
    int hImg;
    // the pixel points that correspond to (0, 0);
    int OriginX;
    int OriginY;
    uint minMovement;
    uint maxMovement;

    // the drawable area of the map...
    int drawableW;
    int drawableH;

    bool bZoom; // a flag to indicate that zoom is active
    bool bIllum;    // flag to indicate that illumination is active
    bool readOnly;  // flag to indicate the readonly mode

    QTimeZone m_cursor;
    int m_cursor_x;
    int m_cursor_y;

    int wx; // viewport x, y
    int wy;

    QVector<CityPos> cities;
    bool citiesInit;

    QTimer* cursorTimer;
    int accelHori;
    int accelVert;
};

QWorldmapPrivate::QWorldmapPrivate()
:   pixCurr( 0 ),
    lblCity( 0 ),
    cmdZoom( 0 ),
    tHide( 0 ),
    wImg( 0 ),
    hImg( 0 ),
    OriginX( 0 ),
    OriginY( 0 ),
    minMovement( 1 ),
    drawableW( -1 ),
    drawableH( -1 ),
    bZoom( false ),
    bIllum( true ),
    readOnly( false ),
    m_cursor(),
    m_cursor_x( -1 ),
    m_cursor_y( -1 ),
    wx( 0 ),
    wy( 0 ),
    citiesInit( false ),
    cursorTimer( 0 ),
    accelHori(0),
    accelVert(0)
{
}

QWorldmapPrivate::~QWorldmapPrivate()
{
    delete pixCurr;
}

bool QWorldmapPrivate::zoneToWin(
    int zoneX,
    int zoneY,
    int &winX,
    int &winY,
    QWidget* viewport,
    bool adjust ) const
{
    Q_ASSERT( viewport );

    if(!bZoom)
    {
      winY = OriginY - ( ( hImg * zoneY ) / 648000 );  // 180 degrees in secs
      winX = OriginX + ( ( wImg * zoneX ) / 1296000 ); // 360 degrees in secs
    }
    else
    {
      winY = OriginY - ( ( hImg/2 * zoneY ) / 648000 );  // 180 degrees in secs
      winX = OriginX + ( ( wImg/2 * zoneX ) / 1296000 ); // 360 degrees in secs
      winY= 2 * winY - wy;
      winX= 2 * winX - wx;
    }

    // whoa, some things aren't in the best spots..
    bool ret = true;
    if ( adjust ) {
        if ( winX > viewport->width() ) {
            winX = viewport->width() - QWORLDMAP_CITYOFFSET;
            ret = false;
        } else if ( winX <= 0 ) {
            winX = QWORLDMAP_CITYOFFSET;
            ret = false;
        }

        if ( winY >= viewport->height() ) {
            winY = viewport->height() - QWORLDMAP_CITYOFFSET;
            ret = false;
        } else if ( winY <= 0 ) {
            winY = QWORLDMAP_CITYOFFSET;
            ret = false;
        }
    }
    return ret;
}

bool QWorldmapPrivate::winToZone(
    int winX,
    int winY,
    int &zoneX,
    int &zoneY ) const
{
    if(!bZoom)
    {
      zoneY = ( 648000 * ( OriginY - winY ) ) / hImg;
      zoneX = ( 1296000 * ( winX - OriginX ) ) / wImg;
    }
    else
    {
      winX= ( winX + wx ) / 2;
      winY= ( winY + wy ) / 2;
      zoneY = ( 648000 * ( OriginY - winY ) )*2 / hImg;
      zoneX = ( 1296000 * ( winX - OriginX ) )*2 / wImg;
    }
    // perhaps in the future there will be some real error checking
    // for now just return true...
    return true;
}

// ============================================================================
//
// QWorldmap
//
// ============================================================================

/*!
    \class QWorldmap
    \mainclass
    \brief The QWorldmap widget displays a worldmap for time zone selection

    The QWorldmap widget displays a worldmap for time zone selection.
    selectNewZone() is used to select one of the available time zones from
    the map.

    \ingroup time
*/


/*!
    \fn void QWorldmap::selecting();

    Signal that is emitted when the widget enters zone selection mode
*/

/*!
    \fn void QWorldmap::newZone( const QTimeZone& zone );

    Signal that is emitted when the new \a zone is selected from the map
*/

/*!
    \fn void QWorldmap::selectZoneCanceled();

    Signal that is emitted when zone selection is canceled.
*/


/*!
    \fn void QWorldmap::buttonSelected();

    Signal that is emitted when the user selects a new time zone via the slect button used in touchscreen mode.
*/

/*!

  Creates a world map widget and attaches it to \a parent.
*/
QWorldmap::QWorldmap( QWidget *parent )
:   QAbstractScrollArea( parent ),
    d( 0 )
{

    setProperty("updateOnEditFocus", true); // We want an update when we gain/lose edit focus.
    d = new QWorldmapPrivate();

    d->cursorTimer = new QTimer(this);
    d->lblCityTimer = new QTimer(this);

    if( !Qtopia::mousePreferred())  {
        connect( d->cursorTimer, SIGNAL(timeout()),
                 this, SLOT(cursorTimeout()));
    } else {
              setFocusPolicy( Qt::NoFocus );
    }
       connect( d->lblCityTimer, SIGNAL(timeout()),
                 this, SLOT(cityLabelTimeout()));

    setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

    // get the map loaded
    // just set the current image to point
    d->pixCurr = new QPixmap();

    QIcon pixZoom( QPixmap(":image/worldtime/mag"));

    float dpi = QApplication::desktop()->screen()->logicalDpiY();

    d->cmdZoom = new ZoomButton( this );
    d->cmdZoom->setIcon( pixZoom );
    d->cmdZoom->setDown( false );

    int buttonSizeW;
    int buttonSizeH;

    if(dpi < 200 ) {
        buttonSizeW = 25;
        buttonSizeH = 25;
    } else {
        buttonSizeW = d->cmdZoom->sizeHint().width();
        buttonSizeH = d->cmdZoom->sizeHint().height();
    }

    d->cmdZoom->resize( buttonSizeW, buttonSizeH );

    QIcon pixSelect( QPixmap(":icon/select"));
    d->selecButton = new ZoomButton( this );
    d->selecButton->setIcon( pixSelect );
    d->selecButton->setDown( false );

    if( !Qtopia::mousePreferred())  {
        // not needed on keypad
        d->selecButton->hide();
    } else {
        d->selecButton->resize( buttonSizeW, buttonSizeH);

    }

    QIcon pixCancel( QPixmap(":icon/cancel"));
    d->cancelButton = new ZoomButton( this );
    d->cancelButton->setIcon( pixCancel );
    d->cancelButton->setDown( false );

    if( !Qtopia::mousePreferred())  {
        // not needed on keypad
        d->cancelButton->hide();
    } else {
        d->cancelButton->resize(  buttonSizeW, buttonSizeH);
    }

    d->lblCity = new CityLabel(tr("CITY"), this);

    // A timer to make sure the label gets hidden
    d->tHide = new QTimer( this );
    QObject::connect( d->tHide, SIGNAL(timeout()),
                      d->lblCity, SLOT(hide()));
    QObject::connect( d->tHide, SIGNAL(timeout()),
                      this, SLOT(redraw()));
    QTimer *tUpdate = new QTimer( this );
    QObject::connect( tUpdate, SIGNAL(timeout()),
                      this, SLOT(update()));
    QObject::connect( qApp, SIGNAL(timeChanged()),
                      this, SLOT(update()));
    QObject::connect( d->cmdZoom, SIGNAL(pressed()),
                      this, SLOT(toggleZoom()));

    if( Qtopia::mousePreferred()) {
        QObject::connect( d->selecButton, SIGNAL(pressed()),
                      this, SLOT(select()));
        if( Qtopia::mousePreferred())
            QObject::connect( d->cancelButton, SIGNAL(pressed()),
                              this, SLOT(selectCanceled()));
    }

    QObject::connect( &d->norm, SIGNAL(signalNewPoint(QPoint)),
                      this, SLOT(setZone(QPoint)) );
    // update the sun's movement every 5 minutes
    tUpdate->start( 5 * 60 * 1000 );
    // May as well read in the timezone information too...

    d->cities.clear();

     if( !Qtopia::mousePreferred())
         QSoftMenuBar::setLabel( this,
                            Qt::Key_Back,
                            QSoftMenuBar::Cancel,
                            QSoftMenuBar::EditFocus );

    selectionMode = true;
    QTimer::singleShot( 0, this, SLOT(initCities()));
}

/*!
    Destroys this world map.
*/
QWorldmap::~QWorldmap()
{
    delete d;
}

/*!
    \internal
*/


void QWorldmap::mousePressEvent( QMouseEvent* event )
{
    if ( d->readOnly ) {
        QAbstractScrollArea::mousePressEvent( event );
        return;
    }

    // add the mouse event into the normalizer, and get the average,
    // pass it along

    redraw();
    d->norm.start();
    d->norm.addEvent( event->pos() );
}

/*!
    \internal
*/
void QWorldmap::mouseMoveEvent( QMouseEvent* event )
{
    if ( d->readOnly )
        return;

    if ((event->x() >= 0 && event->x() < viewport()->width()) &&
            (event->y() >= 0 && event->y() < viewport()->height()))
    {

        redraw();
        d->norm.start();
        d->norm.addEvent( event->pos() );

        if( selectionMode) {
        d->norm.stop();
        QString line = "";
        //     if ( d->m_last.isValid() ) {
            line = d->m_last.id();
            stopSelecting();
            d->m_last = QTimeZone( line.toLatin1() );
            //      }
        d->tHide->start( QWORLDMAP_CITYLABEL_TIMEOUT);
        }
    }
    event->accept();
}

/*!
    \internal
*/
void QWorldmap::mouseReleaseEvent( QMouseEvent* event )
{
    if ( d->readOnly ) {
        QAbstractScrollArea::mouseReleaseEvent( event );
        return;
    }

    QString line = "";

    // get the averaged points in case a timeout hasn't occurred,
    // more for "mouse clicks"
    d->norm.stop();
    if ( d->m_last.isValid() ) {
        line = d->m_last.id();
         stopSelecting();
        emit newZone( QTimeZone( line.toLatin1() ) );
        d->m_last = QTimeZone( line.toLatin1() );
    }

        d->tHide->start( QWORLDMAP_CITYLABEL_TIMEOUT);
}

/*!
    Puts the widget into time zone selection mode.
*/
void QWorldmap::selectNewZone()
{
    if ( d->readOnly ) {
        return;
    }

    if ( Qtopia::mousePreferred() || !hasEditFocus() ) {
        if( !Qtopia::mousePreferred() ) {
            // Generate a key event to put this widget into edit focus
            QKeyEvent keyEvent( QEvent::KeyPress, Qt::Key_Select, Qt::NoModifier );
            keyPressEvent( &keyEvent );
        }
        startSelecting();
    }
}

/*!
    Sets the widget into continous time zone selection mode \a selectMode

    touchscreen only.
*/
void QWorldmap::setContinuousSelect(const bool selectMode)
{
    selectionMode = selectMode;
}

/*!
    \internal
*/
void QWorldmap::startSelecting()
{
    d->tHide->stop();
    if ( d->m_cursor.isValid() )
        showCity( d->m_cursor );
    else
        setZone( QPoint( viewport()->width(), viewport()->height() ) / 2 );

        d->cmdZoom->setFocusPolicy( Qt::NoFocus );


    emit selecting();
}

/*!
    \internal
*/
void QWorldmap::stopSelecting()
{
     if( Qtopia::mousePreferred())
        return;

    d->cmdZoom->setFocusPolicy( Qt::StrongFocus );
}

void QWorldmap::selectCanceled()
{
    emit selectZoneCanceled();
}

/*!
    Determines the height of the widget for a given width \a w.
*/
int QWorldmap::heightForWidth( int w ) const
{
    float scale = .5;
    return int(float(w) * scale);
}

/*!
    \internal
*/
void QWorldmap::keyPressEvent( QKeyEvent *ke )
{
    // On keypad devices, we must ignore all keys except the Select
    // key if we don't have edit focus.  This allows the parent window
    // to process key presses that navigate to other widgets or exit the app.
    if ( !Qtopia::mousePreferred() && !hasEditFocus() ) {
        if ( ke->key() == Qt::Key_Select ) {
            setEditFocus( true );
            startSelecting();
            ke->accept();
        } else
            ke->ignore();
        return;
    }

    // For processing the remaining events, we know that either we have edit
    // focus, or we're running on a touchscreen device, where edit focus
    // doesn't matter.
    switch ( ke->key() )
    {
#ifdef TEST_ACCESS_TO_CITIES
        case Qt::Key_T:
            testAccess();
            break;
#endif

        case Qt::Key_No:
        case Qt::Key_Back:
            // On keypad devices, the Back button should unfocus the map
            // rather than exiting the application.
            if ( !Qtopia::mousePreferred() && hasEditFocus()) {
                emit selectZoneCanceled();
                setEditFocus( false );
                stopSelecting();
                d->tHide->start(250);
                ke->accept();
            }
            else
                ke->ignore();
            break;

        case Qt::Key_Left:
            if (!ke->isAutoRepeat() && d->accelHori == 0) {
                d->accelHori = -1;
                d->cursorTimer->setInterval(500);
                updateCursor();
            } else {
                updateCursor();
            }
            ke->accept();
            break;

        case Qt::Key_Right:
            if (!ke->isAutoRepeat() && d->accelHori == 0) {
                d->accelHori = 1;
                d->cursorTimer->setInterval(500);
                updateCursor();
            } else
                updateCursor();
            ke->accept();
            break;

        case Qt::Key_Up:
            if (!ke->isAutoRepeat() && d->accelVert == 0) {
                d->accelVert = -1;
                d->cursorTimer->setInterval(500);
                updateCursor();
            } else
                updateCursor();
            ke->accept();
            break;

        case Qt::Key_Down:
            if (!ke->isAutoRepeat() && d->accelVert == 0) {
                d->accelVert = 1;
                d->cursorTimer->setInterval(500);
                updateCursor();
            } else
                updateCursor();
            ke->accept();
            break;

        case Qt::Key_Space:
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Select:
            if( !Qtopia::mousePreferred() )
                if ( d->m_cursor.isValid() ) {
                    QString line = d->m_cursor.id();
                    emit newZone( QTimeZone( line.toLatin1() ) );
                    stopSelecting();
                    QTimer::singleShot( 0, this, SLOT(redraw()) );
                    d->lblCity->hide();
                    setEditFocus( false );
                }
            ke->accept();
            break;

        default:
            QAbstractScrollArea::keyPressEvent(ke);
    }
}

/*!
    \internal
*/
void QWorldmap::keyReleaseEvent( QKeyEvent *ke )
{
    switch(ke->key())
    {
        case Qt::Key_Up:
        case Qt::Key_Down:
            if (!ke->isAutoRepeat() && d->accelVert != 0) {
                d->accelVert = 0;
                if (d->accelHori == 0)
                    d->cursorTimer->stop();
            }
            break;

        case Qt::Key_Left:

        case Qt::Key_Right:
            if (!ke->isAutoRepeat() && d->accelHori != 0) {
                d->accelHori = 0;
                if (d->accelVert == 0)
                    d->cursorTimer->stop();
            }
            break;

        default:
            QAbstractScrollArea::keyReleaseEvent(ke);
    }
}

/* accelerating cursor movement */
/*!
    \internal
*/
void QWorldmap::cursorTimeout()
{
    if (d->accelHori < 0 && d->accelHori > -5)
        d->accelHori = -5;
    else if (d->accelHori > 0 && d->accelHori < 5)
        d->accelHori = 5;
    if (d->accelVert < 0 && d->accelVert > -5)
        d->accelVert = -5;
    else if (d->accelVert > 0 && d->accelVert < 5)
        d->accelVert = 5;
    updateCursor();
    d->cursorTimer->setInterval(100);
}

/*!
    \internal
*/
void QWorldmap::cityLabelTimeout()
{
    if(  d->lblCity->isVisible()) {
        setCityLabelText();
    } else {
        d->lblCityTimer->stop();
    }
}


/*!
    \internal
*/
void QWorldmap::setCityLabelText()
{
    QDateTime cityTime = d->m_cursor.fromUtc(QTimeZone::utcDateTime());

    d->lblCity->setText( d->m_cursor.city().replace(QRegExp("_"), " ") +
                         "\n" + QTimeString::localHM( cityTime.time() ) );
    int ms = 1000 - QTime::currentTime().msec();
    ms += (60-QTime::currentTime().second())*1000;
    d->lblCityTimer->setInterval( ms);
}

/*!
    \internal
*/
void QWorldmap::updateCursor()
{
    // accellerate timer after first one..
    uint mx, my;
    uint habs = QABS(d->accelHori);
    uint vabs = QABS(d->accelVert);
    mx = d->minMovement << ( habs / QWORLDMAP_ACCELRATE ); // min movement doubles.
    my = d->minMovement << ( vabs / QWORLDMAP_ACCELRATE );
    if ( mx < d->maxMovement ) {
        if (d->accelHori < 0)
            d->accelHori--;
        else if (d->accelHori > 0)
            d->accelHori++;
    } else
        mx = d->maxMovement;
    if ( my < d->maxMovement ) {
        if (d->accelVert < 0)
            d->accelVert--;
        else if (d->accelVert > 0)
            d->accelVert++;
    } else
        my = d->maxMovement;

    if ( d->m_cursor_x != -1 && d->m_cursor_y != -1 ) {
        int nx = d->m_cursor_x;
        int ny = d->m_cursor_y;

        // horizontal movement
        if (d->accelHori < 0) {
            nx -= mx;
            if (nx < 0) {
                QScrollBar* scrollbar = horizontalScrollBar();
                if ( !d->bZoom ||
                     ( scrollbar->sliderPosition() == scrollbar->minimum() ) )
                {
                    scrollbar->setSliderPosition( scrollbar->maximum() );
                    nx = viewport()->width();
                }
                else
                {
                    scrollbar->setSliderPosition( scrollbar->sliderPosition() -
                                                  scrollbar->pageStep() );
                    nx = scrollbar->pageStep();
                }
            }
        }
        else if (d->accelHori > 0) {
            nx += mx;
            if (nx > viewport()->width()) {
                QScrollBar* scrollbar = horizontalScrollBar();
                if ( !d->bZoom ||
                     ( scrollbar->sliderPosition() == scrollbar->maximum() ) )
                {
                    scrollbar->setSliderPosition( scrollbar->minimum() );
                    nx = 0;
                }
                else
                {
                    scrollbar->setSliderPosition( scrollbar->sliderPosition() +
                                                  scrollbar->pageStep() );
                    nx = viewport()->width() - scrollbar->pageStep();
                }
            }
        }
        // vertical movement
        if (d->accelVert < 0) {
            ny -= my;
            if (ny < 0) {
                QScrollBar* scrollbar = verticalScrollBar();
                if ( !d->bZoom ||
                     ( scrollbar->sliderPosition() == scrollbar->minimum() ) )
                {
                    scrollbar->setSliderPosition( scrollbar->maximum() );
                    ny = viewport()->height();
                }
                else
                {
                    scrollbar->setSliderPosition( scrollbar->sliderPosition() -
                                                  scrollbar->pageStep() );
                    ny = scrollbar->pageStep();
                }
            }
        } else if (d->accelVert > 0) {
            ny += my;
            if (ny > viewport()->height()) {
                QScrollBar* scrollbar = verticalScrollBar();
                if ( !d->bZoom ||
                     ( scrollbar->sliderPosition() == scrollbar->maximum() ) )
                {
                    scrollbar->setSliderPosition( scrollbar->minimum() );
                    ny = 0;
                }
                else
                {
                    scrollbar->setSliderPosition( scrollbar->sliderPosition() +
                                                  scrollbar->pageStep() );
                    ny = viewport()->height() - scrollbar->pageStep();
                }
            }
        }
        setCursorPoint(nx, ny);
        viewport()->update();
    }
}

/*!
    \internal
*/
void QWorldmap::setCursorPoint( int ox, int oy, QString city )
{
    int i;

    // Old Location Window Coords
    int olwx, olwy;
    d->zoneToWin( d->m_cursor.longitude(),
                  d->m_cursor.latitude(),
                  olwx,
                  olwy,
                  viewport() );

    // New Location Window Coords
    int nlwx=0, nlwy=0;
    d->m_cursor_x = ox;
    d->m_cursor_y = oy;

    // zone coords x and y.
    int zx, zy;
    d->winToZone( d->m_cursor_x, d->m_cursor_y, zx, zy );

    long lDistance;
    long lClosest = LONG_MAX;
    QTimeZone closestZone;
    QPoint lPoint;
    CityPos *cp;
    int match=0;
    for ( i = 0; i < d->cities.count(); i++ ) {
        cp = &d->cities[i];
        // use the manhattenLength, a good enough of an appoximation here
        lDistance = QABS(zy - cp->lat) + QABS(zx - cp->lon);
        // Find the closest city (but don't overwrite the specified city)
        if ( lDistance < lClosest ) {
            if ( city.isEmpty() ||
                    ( closestZone != QTimeZone( city.toLatin1() ) ) )
            {
                lClosest = lDistance;
                lPoint = QPoint(cp->lat, cp->lon);
                closestZone = QTimeZone(cp->id.toLatin1());
                match=i;
            }
        } else if ( !city.isEmpty() && ( cp->id == city ) ) {
            lPoint = QPoint(cp->lat, cp->lon);
            closestZone = QTimeZone(cp->id.toLatin1());
            match=i;
        }
    }
    d->m_cursor = closestZone;
    d->zoneToWin( d->m_cursor.longitude(),
                  d->m_cursor.latitude(),
                  nlwx,
                  nlwy,
                  viewport() );

    d->m_last = d->m_cursor;
    setCityLabelText();
    d->lblCityTimer->start();

    d->lblCity->setMinimumSize( d->lblCity->sizeHint() );
    d->lblCity->resize( qMax( d->lblCity->sizeHint().width(), 80 ),
                        qMax( d->lblCity->sizeHint().height(), 40 ) );

    int x,y;
    x=nlwx;
    y=nlwy;

    // Put default position for the city label in the "above left"
    // area.  This avoids obscuring the popup for quite a bit of
    // the map.  Use the "below right" position for the border cases.
    //
    x -= QWORLDMAP_LABELOFFSET + d->lblCity->width();
    if (x < 0) {
        // right
        x += 2*QWORLDMAP_LABELOFFSET + d->lblCity->width();
        // still keep on screen, over red dot if need be.
        if ((x+d->lblCity->width() > viewport()->width()))
            x -= x+d->lblCity->width() - viewport()->width();
    }
    y -= QWORLDMAP_LABELOFFSET + d->lblCity->height();
    if (y < 0) {
        // below
        y += 2*QWORLDMAP_LABELOFFSET + d->lblCity->height();
        // still keep on screen, over red dot if need be.
        if ((y+d->lblCity->height() > viewport()->height()))
            y -= y+d->lblCity->height() - viewport()->height();
    }

    // draw in the city and the label
    if ( d->m_repaint.isValid()) {
        int repx, repy;
        d->zoneToWin( d->m_repaint.longitude(),
                      d->m_repaint.latitude(),
                      repx,
                      repy,
                      viewport() );
    }
    d->m_repaint = d->m_last;

    d->lblCity->move( x, y );
    d->lblCity->show();
    d->tHide->start( 10000 );
    viewport()->update();
}

/*!
    Sets the current time zone to city closest to \a pos.
*/
void QWorldmap::setZone( const QPoint &pos )
{
    initCities();
    setCursorPoint( pos.x(), pos.y() );
}

/*!
    Sets the current time zone to \a zone.
*/
void QWorldmap::setZone( const QTimeZone& zone )
{
    initCities();
    for ( int i = 0; i < d->cities.count(); i++ ) {
        if ( d->cities[i].id == zone.id() ) {
            int olwx, olwy;
            if ( d->bZoom ) {
                if ( !d->zoneToWin( d->cities[i].lon,
                                    d->cities[i].lat,
                                    olwx,
                                    olwy,
                                    viewport() ) )
                {
                    // City must not be on zoomed map, so unzoom it and try again
                    toggleZoom();
                    if ( !d->zoneToWin( d->cities[i].lon,
                                        d->cities[i].lat,
                                        olwx,
                                        olwy,
                                        viewport() ) )
                    {
                        // Bugger, something is really wrong, so just return.
                        return;
                    }
                }
            } else {
                d->zoneToWin( d->cities[i].lon,
                              d->cities[i].lat,
                              olwx,
                              olwy,
                              viewport() );
            }
            setCursorPoint( olwx, olwy, zone.id() );
        }
    }
}

/*!
    Sets the \a readOnly mode for the world map. When \a readOnly is true
    the world map will not accept focus and the zoom button will be invisible.
*/
void QWorldmap::setReadOnly( const bool readOnly )
{
    if( isZoom())
        toggleZoom();
    d->cmdZoom->setVisible( !readOnly );

    if( Qtopia::mousePreferred())  {
        d->selecButton->setVisible( !readOnly );
        d->cancelButton->setVisible( !readOnly );
    }
    d->readOnly = readOnly;
    if ( readOnly ) {
        setFocusPolicy( Qt::NoFocus );
       d->tHide->start( 1);
    } else {
            if( Qtopia::mousePreferred())
        setFocusPolicy( Qt::StrongFocus );
    }
}

/*!
    \internal
*/
void QWorldmap::showCity( const QTimeZone &city )
{
    // use set cursor point to erase old point if need be.
    int mx, my;
    d->zoneToWin( city.longitude(), city.latitude(), mx, my, viewport() );
    setCursorPoint( mx, my, city.id() );
}

/*!
    \internal
*/
void QWorldmap::resizeEvent( QResizeEvent *e )
{
    QSize _size = e->size();

    // if RTL mode is activated the buttons are displayed in a reverse order
    if(qApp->isRightToLeft()) {
        // move the buttons of a scrollbar width when zoomed
        if(d->bZoom){
           d->cmdZoom->move(verticalScrollBar()->width(),
                            _size.height() - d->cmdZoom->height());
           d->selecButton->move(_size.width() - d->selecButton->width()
                                + verticalScrollBar()->width(),
                              _size.height() - d->cmdZoom->height());
           d->cancelButton->move(_size.width() - d->selecButton->width()
                                     - d->cancelButton->width()
                                     + verticalScrollBar()->width(),
                              _size.height() -d->cancelButton->height());
        } else {
            d->cmdZoom->move(0, _size.height() - d->cmdZoom->height());
            d->selecButton->move(_size.width() - d->selecButton->width(),
                                 _size.height() - d->cmdZoom->height());
             d->cancelButton->move(_size.width() - d->selecButton->width()
                                        - d->cancelButton->width(),
                                   _size.height() -d->cancelButton->height());
        }
    } else {
        d->cmdZoom->move( _size.width() - d->cmdZoom->width(),
                      _size.height() - d->cmdZoom->height() );

        d->selecButton->move( 0, _size.height() - d->selecButton->height() );
        d->cancelButton->move( d->selecButton->width(), _size.height() - d->cancelButton->height() );
    }

    d->drawableW = viewport()->width() - 2 * frameWidth();
    d->drawableH = viewport()->height() - 2 * frameWidth();

    if ( !d->bZoom ) {
        d->wx = 0;
        d->wy = 0;
        makeMap( d->drawableW, d->drawableH );
      }
    else
        makeMap( d->wImg, d->hImg );
}

#ifdef DEBUG_QWORLDMAP
/*!
    \internal
*/
void QWorldmap::drawCities( QPainter *p )
{
    int x,y;
    // draw in the cities
    // for testing only as when you put it
    // on the small screen it looks awful and not to mention useless
    p->setPen( Qt::red );
    QTimeZone curZone;
    QList<QString>::iterator it = QTimeZone::ids().begin();
    QString zoneID;
    for (; it != QTimeZone::ids().end(); ++it) {
        zoneID=(QString)*it;
        curZone = QTimeZone( zoneID.toLatin1() );
        d->zoneToWin( curZone.latitude(),
                      curZone.longitude(),
                      x,
                      y,
                      viewport() );
        p->drawRect( x - QWORLDMAP_CITYOFFSET,
                     y - QWORLDMAP_CITYOFFSET,
                     QWORLDMAP_CITYSIZE,
                     QWORLDMAP_CITYSIZE);
    }
}
#endif

/*!
    \internal
*/
static void dayNight(QImage *pImage)
{
    // create a mask that functions from sun.h
    double dJulian,
           dSunRad,
           dSunDecl,
           dSunRadius,
           dSunLong;
    int wImage = pImage->width(),
        hImage = pImage->height(),
        iStart,
        iStop,
        iMid,
        relw,
        i;
    short * wtab = new short [ wImage ];
    time_t tCurrent;
    struct tm *pTm;

    // get the position of the sun based on our current time...
    tCurrent = time( NULL );
    pTm = gmtime( &tCurrent );
    dJulian = jtime( pTm );
    sunpos( dJulian, 0, &dSunRad, &dSunDecl, &dSunRadius, &dSunLong );

    // now get the projected illumination
    projillum( wtab, wImage, hImage, dSunDecl );
    relw = wImage - int( wImage * 0.0275 );

    // draw the map, keeping in mind that we may go too far off the map...
    iMid = ( relw * ( 24*60 - pTm->tm_hour * 60 - pTm->tm_min ) ) / ( 24*60 );

    for ( i = 0; i < hImage; i++ ) {
        if ( wtab[i] > 0 ) {
            iStart = iMid - wtab[i];
            iStop = iMid + wtab[i];
            if ( iStart < 0 ) {
                darken( pImage, iStop, wImage + iStart, i );
            } else if ( iStop > wImage ) {
                darken( pImage, iStop - wImage, iStart, i );
            } else {
                darken( pImage, 0, iStart, i );
                darken( pImage, iStop, wImage, i );
            }
        } else {
            darken( pImage, 0, wImage, i );
        }
    }
    delete [] wtab;
}

/*!
    \internal
*/
static void darken( QImage *pImage, int start, int stop, int row )
{
    // Always clip stop parameter to ensure our preconditions
    if ( stop >= pImage->width() )
        stop =  pImage->width() - 1;

    // Assume that the image is 32bpp as we should have converted to that previously...
    QRgb *p = (QRgb *)pImage->scanLine( row );
    for ( int j = start; j <= stop; j++ ) {
        QRgb rgb = p[j];
        p[j] = qRgb( 2 * qRed( rgb ) / 3, 2 * qGreen( rgb ) / 3, 2 * qBlue( rgb ) / 3 );
    }
}

/*!
    \internal
*/
void QWorldmap::makeMap( int w, int h )
{
    QImage imgOrig;
    QString imgName = QWORLDMAP_IMAGEFILE;
    QPixmap pm;

// if (!QPixmapCache::find("worldmap_big_image", pm)) {

    imgOrig = QImage( ":image/" + imgName );
    if ( imgOrig.isNull() ) {
        QMessageBox::warning( this,
                tr( "Couldn't find Map" ),
                tr( "<p>Couldn't load map: %1, exiting", "%1-map name")
                .arg( QWORLDMAP_IMAGEFILE ) );
        exit(-1);
    }

    // set up the color table for darkening...

    imgOrig = imgOrig.convertToFormat(QImage::Format_RGB32);

    // else go one with making the map...
    if ( d->bIllum ) {
        // do a daylight mask
        dayNight(&imgOrig);
    }
    // redo the width and height
    d->wImg = w;
    d->hImg = h;
    if( !d->bZoom ) {
        d->OriginX = ( d->wImg / 2 ) - int( d->wImg * 0.0275 );
        d->OriginY = d->hImg / 2;
    } else {
        d->OriginX = ( d->wImg / 4 ) - int( d->wImg * 0.0275/2 );
        d->OriginY = d->hImg / 4;
    }

    pm = QPixmap::fromImage(
        imgOrig.scaled( w,
                        h,
                        Qt::IgnoreAspectRatio,
                        Qt::SmoothTransformation ),
        Qt::ThresholdDither );

    //   if( !QPixmapCache::insert("worldmap_big_image", pm) )
    //       qWarning()<<"pixmap insertfailed";

// }

  *d->pixCurr = pm;
    // should also work out max accell, or accell rates for this zoom.
    d->maxMovement = viewport()->width() / 25;
}

/*!
    \internal
*/
void QWorldmap::zoneToZoomedWin( int zoneX, int zoneY, int &winX, int &winY )
{
    d->zoneToWin(zoneX, zoneY, winX, winY, viewport(), !d->bZoom );
    if ( d->bZoom ) {

        // Adjust the zoomed width
        QScrollBar* hscrollbar = horizontalScrollBar();
        QScrollBar* vscrollbar = verticalScrollBar();

        hscrollbar->setSliderPosition( static_cast<int>( winX - (viewport()->width() / 2 ) ));
        vscrollbar->setSliderPosition( static_cast<int>( winY - (viewport()->height() / 2 ) ));
    }
}

/*!
    \internal
*/
void QWorldmap::drawCity( QPainter *p, const QTimeZone &city )
{
    int x, y;
    p->setPen( Qt::red );
    d->zoneToWin( city.longitude(), city.latitude(), x, y,viewport() );

    if (d->m_cursor_x != -1 && d->m_cursor_y != -1) {


        p->drawLine( d->m_cursor_x, d->m_cursor_y, x, y);
        p->drawEllipse( d->m_cursor_x - QWORLDMAP_CITYOFFSET,
                          d->m_cursor_y - QWORLDMAP_CITYOFFSET,
                          QWORLDMAP_CITYSIZE,
                          QWORLDMAP_CITYSIZE );

        //draw cross hairs
        p->setPen( Qt::magenta );
        p->drawLine(x,0, x, viewport()->height());
        p->drawLine(0,y, viewport()->width(),y);

    }
}

/*!
    \internal
*/
void QWorldmap::paintEvent( QPaintEvent * )
{
    QPainter p(viewport());

    d->drawableW = viewport()->width() - 2 * frameWidth();
    d->drawableH = viewport()->height() - 2 * frameWidth();

    int pixmapW = d->pixCurr->width(),
        pixmapH = d->pixCurr->height();
    if ( !d->bZoom &&  ( ( pixmapW != d->drawableW ) ||
                       ( pixmapH != d->drawableH) ) )
    {
        makeMap( d->drawableW, d->drawableH );
    }

    if( d->bZoom ) {
        p.drawPixmap( -horizontalScrollBar()->sliderPosition(), -verticalScrollBar()->sliderPosition(), *d->pixCurr );
    } else {
        p.drawPixmap( 0, 0, *d->pixCurr );
    }

    // Draw that city!
      if ( d->m_last.isValid() )
        drawCity( &p, d->m_last );

    if ( QApplication::keypadNavigationEnabled() && hasFocus() && !hasEditFocus() ) {
        QColor color = palette().color( QPalette::Highlight );
        color.setAlpha( 50 );
        p.fillRect( rect(), color );
    }
}

/*!
    \internal
*/
void QWorldmap::scrollContentsBy( int x, int y )
{
    Q_UNUSED(x);
    Q_UNUSED(y);

    d->wx=horizontalScrollBar()->sliderPosition();
    d->wy=verticalScrollBar()->sliderPosition();
    viewport()->update();
}

/*!
    Returns true if the map is currently zoomed in.

    \sa QWorldmap::toggleZoom()
*/
bool QWorldmap::isZoom() const
{
    return d->bZoom;
}

/*!
    Toggles the map zoom.

    \sa QWorldmap::isZoom()
*/
void QWorldmap::toggleZoom( )
{
    if ( ( d->m_cursor_x == -1 ) || ( d->m_cursor_y == - 1) ) {
        d->m_cursor_x = viewport()->width() / 2;
        d->m_cursor_y = viewport()->height() / 2;
    }

    int cx, cy;
    d->winToZone( d->m_cursor_x, d->m_cursor_y, cx, cy );

    if ( d->bZoom )
        d->bZoom = 0;
    else
        d->bZoom = 1;

    if ( d->bZoom ) {
        setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
        setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOn );
        verticalScrollBar()->setRange( 0,  ( QWORLDMAP_ZOOM - 1) * d->hImg );
        horizontalScrollBar()->setRange( 0, ( QWORLDMAP_ZOOM - 1) * d->wImg );
        makeMap( QWORLDMAP_ZOOM * d->wImg , QWORLDMAP_ZOOM * d->hImg );
        verticalScrollBar()->setSliderPosition(0);
        horizontalScrollBar()->setSliderPosition(0);
    } else {
        setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        makeMap( d->drawableW, d->drawableH );
    }

    int out_cx, out_cy;
    zoneToZoomedWin( cx, cy, out_cx, out_cy );
    setCursorPoint( out_cx, out_cy, d->m_cursor.id() );

    int lx, ly;
    d->zoneToWin( d->m_cursor.longitude(), d->m_cursor.latitude(), lx, ly, viewport() );
    d->lblCity->hide();
    viewport()->update();

}

/*!
    Button select slot - touchscreen only.

*/
void QWorldmap::select( )
{
    emit buttonSelected();
}

/*!
    Returns true if daylight is highlighted on the map

    \sa QWorldmap::setDaylight()
*/
bool QWorldmap::isDaylight() const
{
    return !d->bIllum;
}

/*!
    Returns true if the world map is in read only mode

    \sa QWorldmap::setReadOnly()
*/
bool QWorldmap::isReadOnly() const
{
    return d->readOnly;
}

/*!
    Returns the current selected time zone.

    \sa QWorldmap::setZone(), QWorldmap::newZone()
*/
QTimeZone QWorldmap::zone() const
{
    return d->m_cursor;
}

/*!
    \fn QWorldmap::setDaylight( const bool show );

    Displays the daylight on the map if \a show is true.

    \sa QWorldmap::isDaylight()
*/
void QWorldmap::setDaylight( const bool show )
{
    d->bIllum = !show;
    // make the map...
    makeMap( d->pixCurr->width(), d->pixCurr->height() );
    viewport()->update();
}

/*!
    \internal
*/
void QWorldmap::update()
{
    // recalculate the light, most people will never see this,
    // but it is good to be thorough
    makeMap ( d->pixCurr->width(), d->pixCurr->height() );
    viewport()->update();
}

/*!
    \internal
*/
void QWorldmap::redraw()
{
    // paint over that pesky city...
    int x, y;
    if ( d->m_repaint.isValid() && !d->readOnly ) {
        d->m_last = QTimeZone();
        d->zoneToWin( d->m_repaint.longitude(),
                      d->m_repaint.latitude(),
                      x,
                      y,
                      viewport() );
        viewport()->update();
        d->m_repaint = QTimeZone();
    }
    d->tHide->stop();
}

/*!
    \internal
*/
void QWorldmap::initCities()
{
    // Contructing QTimeZone::QTimeZone( city ) is hideously expensive -
    // preload the position of each city.
    if ( d->citiesInit )
        return;

    QStringList list = QTimeZone::ids();
    QStringList::iterator it = list.begin();

    int count = 0;
    QString zoneID;

    d->cities.resize( list.count() );
    for (; it != list.end(); ++it) {
        zoneID = *it;
        QTimeZone curZone( zoneID.toLatin1() );

/* isValid causes Data to be gotten - which includes gmtime calculation! We only need lon/lat.
        if ( !curZone.isValid() ) {
            qLog(Time) << "initCities()-timezone is invalid!";
            continue;
        }
*/

        CityPos *cp = new CityPos;
        cp->lat = curZone.latitude();
        cp->lon = curZone.longitude();
        cp->id = zoneID;

        d->cities.insert( count++, *cp );
    }

    d->cities.resize(count);
    d->citiesInit = true;

    /* should also set
       min lat
       max lat
       min long
       max long
       and go through zone file for
       min distance between two cities, (halv it for default min movement)
    */
    ulong lDistance;
    ulong lClosest = ULONG_MAX;
    int i,j;

    for ( i = 0; i < d->cities.count(); i++ ) {
        long latfrom = d->cities[i].lat;
        long lonfrom = d->cities[i].lon;
        for ( j = 0; j < d->cities.count(); j++ ) {
            if (i != j) {
                long latto = d->cities[j].lat;
                long lonto = d->cities[j].lon;
                // use the manhattenLength, a good enough of an appoximation here
                lDistance = QABS(latfrom - latto) + QABS(lonfrom - lonto);
                // first to zero wins!
                if ( lDistance < lClosest )
                    lClosest = lDistance;
            }
        }
    }
    viewport()->update();
}

bool QWorldmap::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseButtonPress:
        mousePressEvent(static_cast<QMouseEvent *>(event));
        return true;
    case QEvent::MouseButtonRelease:
        mouseReleaseEvent(static_cast<QMouseEvent *>(event));
        return true;
    case QEvent::MouseMove:
        mouseMoveEvent(static_cast<QMouseEvent *>(event));
        return true;
    default:
        return QAbstractScrollArea::event(event);
    }
}

#ifdef DEBUG_QWORLDMAP
/*!
    \internal
*/
void QWorldmap::testAccess()
{
    initCities();
    for ( unsigned i = 0; i < d->cities.count(); i++ ) {
        CityPos *cp = d->cities[i];
        // check if pixel for this city and or pixels around this city can
        // be accessed.
        int lat, lon, x, y, cx, cy;
        lat = cp->lat;
        lon = cp->lon;
        d->zoneToWin( lon, lat, x, y, viewport() );
        bool found = false;
        QString id = cp->id;
        for (cx = x-1; cx <= x+1; ++cx) {
            for (cy = y-1; cy <= y+1; ++cy) {
                setCursorPoint(cx,cy);
                if (d->m_cursor.id() == id) {
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            for (cx = x-1; cx <= x+1; ++cx) {
                for (cy = y-1; cy <= y+1; ++cy) {
                    setCursorPoint(cx,cy);
                }
            }
        }
    }
}
#endif
