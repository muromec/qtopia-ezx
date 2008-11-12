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

#include "playerwidget.h"

#include "playlist.h"
#include "elidedlabel.h"
#include "visualization.h"
#include "keyfilter.h"
#include "keyhold.h"
#include "mediaplayer.h"

#include <media.h>
#include <qmediatools.h>
#include <qmediawidgets.h>
#ifndef NO_HELIX
#include <qmediahelixsettingscontrol.h>
#endif
#include <qmediavideocontrol.h>

#ifdef QTOPIA_KEYPAD_NAVIGATION
#include <qsoftmenubar.h>
#include <custom.h>
#endif

#include <qtranslatablesettings.h>
#include <qtopiaapplication.h>
#include <qthumbnail.h>

#ifndef NO_HELIX
class HelixLogo : public QWidget
{
public:
    HelixLogo( QWidget* parent = 0 );

    // QWidget
    QSize sizeHint() const { return QSize( m_helixlogo.width(), m_helixlogo.height() ); }

protected:
    // QWidget
    void resizeEvent( QResizeEvent* e );
    void paintEvent( QPaintEvent* e );

private:
    QImage m_helixlogo;
    QPoint m_helixlogopos;
};

HelixLogo::HelixLogo( QWidget* parent )
    : QWidget( parent )
{
    static const QString HELIX_LOGO_PATH = ":image/mediaplayer/helixlogo";

    m_helixlogo = QImage( HELIX_LOGO_PATH );
}

void HelixLogo::resizeEvent( QResizeEvent* )
{
    m_helixlogopos = QPoint( (width() - m_helixlogo.width()) / 2,
        (height() - m_helixlogo.height()) / 2 );
}

void HelixLogo::paintEvent( QPaintEvent* )
{
    QPainter painter( this );

    painter.drawImage( m_helixlogopos, m_helixlogo );
}
#endif

#ifndef NO_HELIX
MediaPlayerSettingsDialog::MediaPlayerSettingsDialog( QWidget* parent )
    : QDialog( parent )
{
    static const int TIMEOUT_MIN = 5;
    static const int TIMEOUT_MAX = 99;

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin( 4 );

    QGroupBox *group = new QGroupBox( tr( "Network Settings" ), this );

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addWidget( new QLabel( tr( "Connection Speed" ) ) );

    m_speedcombo = new QComboBox;
    vbox->addWidget( m_speedcombo );

    QFrame *hr = new QFrame;
    hr->setFrameStyle( QFrame::HLine | QFrame::Plain );
    vbox->addWidget( hr );

    QGridLayout *grid = new QGridLayout;
    grid->addWidget( new QLabel( tr( "Timeouts:" )), 0, 0, 1, 2, Qt::AlignCenter );
    grid->addWidget( new QLabel( tr( "Connection" ) ), 1, 0, Qt::AlignLeft );

    m_validator = new QIntValidator( TIMEOUT_MIN, TIMEOUT_MAX, this );

    m_connecttimeout = new QLineEdit;
    m_connecttimeout->setAlignment( Qt::AlignRight );
    m_connecttimeout->setValidator( m_validator );
    grid->addWidget( m_connecttimeout, 1, 1, Qt::AlignRight );

    grid->addWidget( new QLabel( tr( "Server" ) ), 2, 0, Qt::AlignLeft );

    m_servertimeout = new QLineEdit;
    m_servertimeout->setAlignment( Qt::AlignRight );
    m_servertimeout->setValidator( m_validator );
    grid->addWidget( m_servertimeout, 2, 1, Qt::AlignRight );

    vbox->addLayout( grid );
    vbox->addStretch();

    group->setLayout( vbox );

    layout->addWidget( group );
    setLayout( layout );

    // Initialize settings
    readConfig();

    applySettings();
}

void MediaPlayerSettingsDialog::accept()
{
    writeConfig();
    applySettings();

    QDialog::accept();
}

void MediaPlayerSettingsDialog::readConfig()
{
    QTranslatableSettings config( "Trolltech", "MediaPlayer" );
    config.beginGroup( "Network" );

    int size = config.beginReadArray( "ConnectionSpeed" );
    for( int i = 0; i < size; ++i ) {
        config.setArrayIndex( i );
        m_speedcombo->addItem( config.value( "Type" ).toString(), config.value( "Speed" ) );
    }
    config.endArray();

    if( !m_speedcombo->count() ) {
        m_speedcombo->addItem( tr("GPRS (32 kbps)"), 32000 );
        m_speedcombo->addItem( tr("EGPRS (128 kbps)"), 128000 );
    }

    QVariant value = config.value( "ConnectionSpeedIndex" );
    if( value.isNull() || value.toInt() > m_speedcombo->count() ) {
        value = 0;
    }
    m_speedcombo->setCurrentIndex( value.toInt() );

    value = config.value( "ConnectionTimeout" );
    if( value.isNull() ) {
        value = "5";
    }
    m_connecttimeout->setText( value.toString() );

    value = config.value( "ServerTimeout" );
    if( value.isNull() ) {
        value = "5";
    }
    m_servertimeout->setText( value.toString() );
}

void MediaPlayerSettingsDialog::writeConfig()
{
    QTranslatableSettings config( "Trolltech", "MediaPlayer" );
    config.beginGroup( "Network" );

    config.beginWriteArray( "ConnectionSpeed" );
    for( int i = 0; i < m_speedcombo->count(); ++i ) {
        config.setArrayIndex( i );
        config.setValue( "Type", m_speedcombo->itemText( i ) );
        config.setValue( "Speed", m_speedcombo->itemData( i ) );
    }
    config.endArray();

    config.setValue( "ConnectionSpeedIndex", m_speedcombo->currentIndex() );

    config.setValue( "ConnectionTimeout", m_connecttimeout->text() );
    config.setValue( "ServerTimeout", m_servertimeout->text() );
}

void MediaPlayerSettingsDialog::applySettings()
{
    QMediaHelixSettingsControl settings;

    settings.setOption( "Bandwidth",
        m_speedcombo->itemData( m_speedcombo->currentIndex() ) );

    settings.setOption( "ConnectionTimeOut", m_connecttimeout->text() );
    settings.setOption( "ServerTimeOut", m_servertimeout->text() );
}
#endif

class ThumbnailWidget : public QWidget
{
public:
    ThumbnailWidget( QWidget* parent = 0 )
        : QWidget( parent )
    { }

    void setFile( const QString& file );

protected:
    void resizeEvent( QResizeEvent* ) { m_thumb = QPixmap(); }

    // Load thumbnail and draw onto widget
    void paintEvent( QPaintEvent* );

private:
    QString m_file;

    QPixmap m_thumb;
    QPoint m_thumbpos;
};

void ThumbnailWidget::setFile( const QString& file )
{
    m_file = file;
    m_thumb = QPixmap();

    update();
}

void ThumbnailWidget::paintEvent( QPaintEvent* )
{
    if( m_thumb.isNull() && !m_file.isNull() ) {
        m_thumb = QThumbnail( m_file ).pixmap( size() );
        m_thumbpos = QPoint( (width() - m_thumb.width())/2, (height() - m_thumb.height())/2 );
    }

    QPainter painter( this );
    painter.drawPixmap( m_thumbpos, m_thumb );
}

static QImage load_scaled_image( const QString& filename, const QSize& size, Qt::AspectRatioMode mode = Qt::IgnoreAspectRatio )
{
    QImageReader reader( filename );

    QSize scaled;
    if( mode == Qt::IgnoreAspectRatio ) {
        scaled = size;
    } else {
        scaled = reader.size();
        scaled.scale( size, mode );
    }

    reader.setScaledSize( scaled );
    return reader.read();
}

static QImage invert( const QImage& image )
{
    QImage ret;

    switch( image.format() )
    {
    case QImage::Format_RGB32:
    case QImage::Format_ARGB32:
        ret = image;
        break;
    case QImage::Format_ARGB32_Premultiplied:
        ret = image.convertToFormat( QImage::Format_ARGB32 );
        break;
    default:
        // Unsupported
        return image;
    }

    quint32 *p = (quint32*)ret.bits();
    quint32 *end = p + ret.numBytes()/4;
    while( p != end ) {
        *p++ ^= 0x00ffffff;
    }

    return ret;
}

static QImage contrast( const QColor& color, const QImage& image )
{
    static const int INVERT_THRESHOLD = 128;

    int x, v;
    color.getHsv( &x, &x, &v );
    if( v > INVERT_THRESHOLD ) {
        return invert( image );
    }

    return image;
}

class IconWidget : public QWidget
{
public:
    IconWidget( QWidget* parent = 0 )
        : QWidget( parent )
    { }

    void setFile( const QString& file );

    // QWidget
    QSize sizeHint() const;

protected:
    void reizeEvent( QResizeEvent* ) { m_buffer = QImage(); }

    void paintEvent( QPaintEvent* );

private:
    QString m_filename;

    QImage m_buffer;
};

void IconWidget::setFile( const QString& file )
{
    m_filename = file;

    m_buffer = QImage();
    update();
}

QSize IconWidget::sizeHint() const
{
    int height = QtopiaApplication::style()->pixelMetric(QStyle::PM_SmallIconSize);

    return QSize( height, height );
}

void IconWidget::paintEvent( QPaintEvent* )
{
    if( m_buffer.isNull() ) {
        m_buffer = contrast( palette().windowText().color(), load_scaled_image( m_filename, size() ) );
    }

    QPainter painter( this );
    painter.drawImage( QPoint( 0, 0 ), m_buffer );
}

class PlaylistLabel : public QWidget
{
    Q_OBJECT
public:
    PlaylistLabel( QWidget* parent = 0 );

    void setPlaylist( QExplicitlySharedDataPointer<Playlist> playlist );

private slots:
    void updateLabel();

private:
    QExplicitlySharedDataPointer< Playlist > m_playlist;

    QLabel *m_label;
    IconWidget *m_myshuffleicon;
};

PlaylistLabel::PlaylistLabel( QWidget* parent )
    : QWidget( parent ), m_myshuffleicon( 0 )
{
    m_label = new QLabel ( tr( "- of -", "song '- of -'") );

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin( 0 );

    layout->addWidget( m_label );

    setLayout( layout );
}

void PlaylistLabel::setPlaylist( QExplicitlySharedDataPointer<Playlist> playlist )
{
    // Disconnect from old playlist
    if( m_playlist.data() != NULL ) {
        m_playlist->disconnect( this );
    }

    m_playlist = playlist;

    if( qobject_cast<PlaylistMyShuffle*>( playlist.data() ) ) {
        m_label->hide();
        if(m_myshuffleicon == NULL)
        {
            m_myshuffleicon = new IconWidget;
            m_myshuffleicon->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
            m_myshuffleicon->setMinimumSize( QtopiaApplication::style()->pixelMetric(QStyle::PM_SmallIconSize), QtopiaApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) );
            m_myshuffleicon->setFile( ":image/mediaplayer/black/shuffle" );
            layout()->addWidget( m_myshuffleicon );
        }
        m_myshuffleicon->show();
    } else {

        // Connect to new playlist
        connect( m_playlist, SIGNAL(playingChanged(QModelIndex)),
            this, SLOT(updateLabel()) );
        connect( m_playlist, SIGNAL(rowsInserted(QModelIndex,int,int)),
            this, SLOT(updateLabel()) );
        connect( m_playlist, SIGNAL(rowsRemoved(QModelIndex,int,int)),
            this, SLOT(updateLabel()) );

        updateLabel();

        m_label->show();
        if(m_myshuffleicon != NULL)
            m_myshuffleicon->hide();
    }
}

void PlaylistLabel::updateLabel()
{
    int count = m_playlist->rowCount();

    if( count ) {
        QModelIndex playing = m_playlist->playing();
        if( playing.isValid() ) {
            m_label->setText( tr( "%1 of %2", " e.g. '4 of 6' as in song 4 of 6" ).arg( playing.row() + 1 ).arg( count ) );
        } else {
            m_label->setText( tr( "- of %2", "- of 8" ).arg( count ) );
        }
    } else {
        m_label->setText( tr( "- of -", "song '- of -'" ) );
    }
}

class RepeatState : public QObject
{
    Q_OBJECT
public:
    RepeatState( QObject* parent = 0 )
        : QObject( parent ), m_state( RepeatNone )
    { }

    enum State { RepeatOne, RepeatAll, RepeatNone };

    State state() const { return m_state; }
    void setState( State state );

signals:
    void stateChanged( RepeatState::State state );

private:
    State m_state;
};

void RepeatState::setState( State state )
{
    m_state = state;

    emit stateChanged( state );
}

class ProgressView : public QWidget
{
    Q_OBJECT
public:
    ProgressView( RepeatState* repeatstate, QWidget* parent = 0 );

    void setPlaylist( QExplicitlySharedDataPointer<Playlist> playlist );

    QWidget* keyEventHandler() const { return m_progress; }

public slots:
    void setMediaContent( QMediaContent* content );

private slots:
    void repeatStateChanged( RepeatState::State state );
    void init();

private:
    QMediaContentContext *m_context;
    QMediaProgressWidget *m_progress;
    PlaylistLabel *m_playlistlabel;
    RepeatState *m_repeatstate;
    IconWidget *m_repeaticon;
};

ProgressView::ProgressView( RepeatState* repeatstate, QWidget* parent )
    : QWidget( parent )
    , m_context( 0 )
    , m_progress( 0 )
    , m_playlistlabel( 0 )
    , m_repeatstate( repeatstate )
    , m_repeaticon( 0 )
{
    QTimer::singleShot(1, this, SLOT(init()));
}

void ProgressView::repeatStateChanged( RepeatState::State state )
{
    if(!m_context)
        init();
    switch( state )
    {
    case RepeatState::RepeatOne:
        m_repeaticon->show();
        m_playlistlabel->hide();
        break;
    case RepeatState::RepeatAll:
        m_repeaticon->show();
        m_playlistlabel->show();
        break;
    case RepeatState::RepeatNone:
        m_repeaticon->hide();
        m_playlistlabel->show();
        break;
    }
}

void ProgressView::setMediaContent( QMediaContent* content )
{
    if(!m_context)
        init();
    m_context->setMediaContent( content );
}

void ProgressView::setPlaylist( QExplicitlySharedDataPointer<Playlist> playlist )
{
    if(!m_context)
        init();
    m_playlistlabel->setPlaylist( playlist );
}

void ProgressView::init()
{
    if(m_context)
        return;
    // Connect to repeat state
    connect( m_repeatstate, SIGNAL(stateChanged(RepeatState::State)),
        this, SLOT(repeatStateChanged(RepeatState::State)) );

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin( 0 );

    m_progress = new QMediaProgressWidget;
    layout->addWidget( m_progress );

    QHBoxLayout *hbox = new QHBoxLayout;

    m_playlistlabel = new PlaylistLabel;
    hbox->addWidget( m_playlistlabel );

    m_repeaticon = new IconWidget;
    m_repeaticon->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );
    m_repeaticon->setMinimumSize( QtopiaApplication::style()->pixelMetric(QStyle::PM_SmallIconSize), QtopiaApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) );
    m_repeaticon->setFile( ":image/mediaplayer/black/repeat" );
    hbox->addWidget( m_repeaticon );

    hbox->addStretch();

    QMediaProgressLabel *progresslabel = new QMediaProgressLabel( QMediaProgressLabel::ElapsedTotalTime );
    hbox->addWidget( progresslabel );

    layout->addLayout( hbox );
    setLayout( layout );

    m_context = new QMediaContentContext( this );
    m_context->addObject( m_progress );
    m_context->addObject( progresslabel );

    // Initialize view
    repeatStateChanged( m_repeatstate->state() );
}

class VolumeView : public QWidget
{
    Q_OBJECT
public:
    VolumeView( QWidget* parent = 0 );

    QWidget* keyEventHandler() const { return m_volume; }

public slots:
    void setMediaContent( QMediaContent* content );

private:
    QMediaVolumeWidget *m_volume;
};

VolumeView::VolumeView( QWidget* parent )
    : QWidget( parent )
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin( 0 );

    m_volume = new QMediaVolumeWidget;
    layout->addWidget( m_volume );

    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->addWidget( new QMediaVolumeLabel( QMediaVolumeLabel::MinimumVolume ) );
    hbox->addStretch();
    hbox->addWidget( new QMediaVolumeLabel( QMediaVolumeLabel::MaximumVolume ) );

    layout->addLayout( hbox );
    setLayout( layout );
}

void VolumeView::setMediaContent( QMediaContent* content )
{
    m_volume->setMediaContent( content );
}

class SeekView : public QWidget
{
    Q_OBJECT
public:
    SeekView( QWidget* parent = 0 );

    QWidget* keyEventHandler() const { return m_seekwidget; }

public slots:
    void setMediaContent( QMediaContent* content );

private:
    QMediaSeekWidget *m_seekwidget;
};

SeekView::SeekView( QWidget* parent )
    : QWidget( parent )
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin( 0 );

    m_seekwidget = new QMediaSeekWidget;
    layout->addWidget( m_seekwidget );

    QHBoxLayout *hbox = new QHBoxLayout;

    QMediaProgressLabel *elapsed = new QMediaProgressLabel( QMediaProgressLabel::ElapsedTime );
    connect( m_seekwidget, SIGNAL(lengthChanged(quint32)),
        elapsed, SLOT(setTotal(quint32)) );
    connect( m_seekwidget, SIGNAL(positionChanged(quint32)),
        elapsed, SLOT(setElapsed(quint32)) );
    hbox->addWidget( elapsed );
    hbox->addStretch();

    QMediaProgressLabel *remaining = new QMediaProgressLabel( QMediaProgressLabel::RemainingTime );
    connect( m_seekwidget, SIGNAL(lengthChanged(quint32)),
        remaining, SLOT(setTotal(quint32)) );
    connect( m_seekwidget, SIGNAL(positionChanged(quint32)),
        remaining, SLOT(setElapsed(quint32)) );
    hbox->addWidget( remaining );

    layout->addLayout( hbox );
    setLayout( layout );
}

void SeekView::setMediaContent( QMediaContent* content )
{
    m_seekwidget->setMediaContent( content );
}

class WhatsThisLabel : public QLabel
{
public:
    WhatsThisLabel( QWidget* parent = 0 )
        : QLabel( parent )
    { }

    void addWidgetToMonitor( QWidget* widget );

    // QObject
    bool eventFilter( QObject* o, QEvent* e );
};

void WhatsThisLabel::addWidgetToMonitor( QWidget* widget )
{
    widget->installEventFilter( this );
}

bool WhatsThisLabel::eventFilter( QObject* o, QEvent* e )
{
    if( e->type() == QEvent::FocusIn ) {
        QWidget *widget = qobject_cast<QWidget*>(o);
        if( widget ) {
            setText( widget->whatsThis() );
        }
    }

    return false;
}

class PlayerDialog : public QDialog
{
public:
    PlayerDialog( QWidget* parent, Qt::WindowFlags f );
};

PlayerDialog::PlayerDialog( QWidget* parent, Qt::WindowFlags f )
    : QDialog( parent, f )
{
    static const int BACKGROUND_ALPHA = 190;

    // Use custom palette
    QPalette pal = palette();
    QColor button = pal.button().color();
    button.setAlpha( BACKGROUND_ALPHA );
    pal.setColor( QPalette::Window, button );
    setPalette( pal );

    // Remove title bar from dialog
    setWindowFlags( windowFlags() | Qt::FramelessWindowHint );
}

class ToolButtonStyle : public QWindowsStyle
{
public:
    void drawComplexControl( ComplexControl cc, const QStyleOptionComplex *opt,
        QPainter *p, const QWidget *widget ) const;

private:
    mutable QPixmap m_buttonbuffer;
    mutable QPixmap m_focusedbuttonbuffer;
};

void ToolButtonStyle::drawComplexControl( ComplexControl cc, const QStyleOptionComplex *opt,
    QPainter *p, const QWidget *widget ) const
{
    switch( cc )
    {
    // Polished tool button
    case CC_ToolButton:
        if (const QStyleOptionToolButton *toolbutton
            = qstyleoption_cast<const QStyleOptionToolButton *>(opt)) {
            QRect button, menuarea;
            button = subControlRect(cc, toolbutton, SC_ToolButton, widget);

            QPixmap *icon;

            if( toolbutton->state & State_HasFocus ) {
                if( !m_focusedbuttonbuffer.isNull() || m_focusedbuttonbuffer.size() != button.size() ) {
                    m_focusedbuttonbuffer = QIcon( ":icon/mediaplayer/black/vote-button-focused" ).pixmap( button.size() );
                }

                icon = &m_focusedbuttonbuffer;
            } else {
                if( !m_buttonbuffer.isNull() || m_buttonbuffer.size() != button.size() ) {
                    m_buttonbuffer = QIcon( ":icon/mediaplayer/black/vote-button" ).pixmap( button.size() );
                }

                icon = &m_buttonbuffer;
            }

            p->drawPixmap( button, *icon );

            QStyleOptionToolButton label = *toolbutton;
            int fw = pixelMetric(PM_DefaultFrameWidth, opt, widget);
            label.rect = button.adjusted(fw, fw, -fw, -fw);

            drawControl(CE_ToolButtonLabel, &label, p, widget);
        }
        break;
    default:
        QWindowsStyle::drawComplexControl( cc, opt, p, widget );
    }
}

class ToolButtonDialog : public PlayerDialog
{
public:
    ToolButtonDialog( QWidget* parent, Qt::WindowFlags f );
    ~ToolButtonDialog();

protected:
    QToolButton* addToolButton( const QIcon& icon, const QString& whatsthis );
    bool eventFilter( QObject *object, QEvent *event );

private:
    ToolButtonStyle *m_style;
    QHBoxLayout *m_layout;
    WhatsThisLabel *m_label;
    int m_iconsize;
    int m_buttonsize;
};

ToolButtonDialog::ToolButtonDialog( QWidget* parent, Qt::WindowFlags f )
    : PlayerDialog( parent, f )
{
    m_style = new ToolButtonStyle;

    m_iconsize = QtopiaApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) + 4;
    m_buttonsize = m_iconsize + 12;

    m_label = new WhatsThisLabel( this );
    m_label->setAlignment( Qt::AlignHCenter );

    QFont smallfont = font();
    smallfont.setPointSize( smallfont.pointSize() - 2 );
    smallfont.setItalic( true );
    smallfont.setBold( true );

    m_label->setFont( smallfont );

    m_layout = new QHBoxLayout;
    m_layout->setSpacing( 18 );

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setMargin( 6 );
    vbox->addLayout( m_layout );
    vbox->addWidget( m_label );

    setLayout( vbox );
}

QToolButton* ToolButtonDialog::addToolButton( const QIcon& icon, const QString& whatsthis )
{
    QToolButton *button = new QToolButton;
    button->setIconSize( QSize( m_iconsize, m_iconsize ) );
    button->setMinimumSize( m_buttonsize, m_buttonsize );
    button->setStyle( m_style );

    button->setIcon( icon );
    button->setWhatsThis( whatsthis );

    button->installEventFilter( this );

    m_label->addWidgetToMonitor( button );
    m_layout->addWidget( button );

    return button;
}

bool ToolButtonDialog::eventFilter( QObject *object, QEvent *event )
{
    if( event->type() == QEvent::KeyPress && QApplication::isRightToLeft() )
    {
        QKeyEvent *keyEvent = static_cast< QKeyEvent * >( event );

        switch( keyEvent->key() )
        {
        case Qt::Key_Left:
            return object->event( new QKeyEvent( QEvent::KeyPress, Qt::Key_Tab, keyEvent->modifiers() ) );
        case Qt::Key_Right:
            return object->event( new QKeyEvent( QEvent::KeyPress, Qt::Key_Backtab, keyEvent->modifiers() ) );
        }
    }

    return PlayerDialog::eventFilter( object, event );
}

ToolButtonDialog::~ToolButtonDialog()
{
    delete m_style;
}

class VoteDialog : public ToolButtonDialog
{
    Q_OBJECT
public:
    VoteDialog( QWidget* parent = 0, Qt::WindowFlags f = 0 );

signals:
    void favoriteVoted();
    void snoozeVoted();
    void banVoted();

private slots:
    void voteFavorite();
    void voteSnooze();
    void voteBan();
};

VoteDialog::VoteDialog( QWidget* parent, Qt::WindowFlags f )
    : ToolButtonDialog( parent, f )
{
    QToolButton *button;

    button = addToolButton( QIcon( ":icon/mediaplayer/black/favorite" ),
        tr( "Play this one more often" ) );
    connect( button, SIGNAL(clicked()), this, SLOT(voteFavorite()) );

    button = addToolButton( QIcon( ":icon/mediaplayer/black/snooze" ),
        tr( "Getting tired of this one" ) );
    connect( button, SIGNAL(clicked()), this, SLOT(voteSnooze()) );

    button = addToolButton( QIcon( ":icon/mediaplayer/black/ban" ),
        tr( "Never play this one again" ) );
    connect( button, SIGNAL(clicked()), this, SLOT(voteBan()) );
}

void VoteDialog::voteFavorite()
{
    PlaylistMyShuffle *myshuffle = qobject_cast<PlaylistMyShuffle*>(MediaPlayer::instance()->playlist().data());
    if( myshuffle ) {
        myshuffle->setProbability( MediaPlayer::instance()->playlist()->playing(), PlaylistMyShuffle::Frequent );

        emit favoriteVoted();
    }

    accept();
}

void VoteDialog::voteSnooze()
{
    PlaylistMyShuffle *myshuffle = qobject_cast<PlaylistMyShuffle*>(MediaPlayer::instance()->playlist().data());
    if( myshuffle ) {
        myshuffle->setProbability( MediaPlayer::instance()->playlist()->playing(), PlaylistMyShuffle::Infrequent );

        emit snoozeVoted();
    }

    accept();
}

void VoteDialog::voteBan()
{
    PlaylistMyShuffle *myshuffle = qobject_cast<PlaylistMyShuffle*>(MediaPlayer::instance()->playlist().data());
    if( myshuffle ) {
        myshuffle->setProbability( MediaPlayer::instance()->playlist()->playing(), PlaylistMyShuffle::Never );

        emit banVoted();
    }

    accept();
}

class RepeatDialog : public ToolButtonDialog
{
    Q_OBJECT
public:
    RepeatDialog( RepeatState* repeatstate, QWidget* parent = 0, Qt::WindowFlags f = 0 );

private slots:
    void repeatOne();
    void repeatAll();
    void repeatNone();

protected:
    // QWidget
    void keyPressEvent( QKeyEvent* e );

private:
    RepeatState *m_repeatstate;
};

RepeatDialog::RepeatDialog( RepeatState* repeatstate, QWidget* parent, Qt::WindowFlags f )
    : ToolButtonDialog( parent, f ), m_repeatstate( repeatstate )
{
    QToolButton *button;

    button = addToolButton( QIcon( ":icon/mediaplayer/black/repeat-one" ),
        tr( "Repeat this one only","Repeat this track only" ) );
    connect( button, SIGNAL(clicked()), this, SLOT(repeatOne()) );

    button = addToolButton( QIcon( ":icon/mediaplayer/black/repeat-all" ),
        tr( "Repeat entire playlist" ) );
    connect( button, SIGNAL(clicked()), this, SLOT(repeatAll()) );

    button = addToolButton( QIcon( ":icon/mediaplayer/black/repeat-none" ),
        tr( "Don't repeat anything" ) );
    connect( button, SIGNAL(clicked()), this, SLOT(repeatNone()) );
}

void RepeatDialog::keyPressEvent( QKeyEvent* e )
{
    switch( e->key() )
    {
    case Qt::Key_1:
        e->accept();
        repeatOne();
        break;
    case Qt::Key_Asterisk:
        e->accept();
        repeatAll();
        break;
    case Qt::Key_0:
        e->accept();
        repeatNone();
        break;
    default:
        ToolButtonDialog::keyPressEvent( e );
        break;
    }
}

void RepeatDialog::repeatOne()
{
    m_repeatstate->setState( RepeatState::RepeatOne );
    accept();
}

void RepeatDialog::repeatAll()
{
    m_repeatstate->setState( RepeatState::RepeatAll );
    accept();
}

void RepeatDialog::repeatNone()
{
    m_repeatstate->setState( RepeatState::RepeatNone );
    accept();
}

class TrackInfoDialog : public PlayerDialog
{
    Q_OBJECT
public:
    TrackInfoDialog( QWidget* parent = 0, Qt::WindowFlags f = 0 );

    void setPlaylist( QExplicitlySharedDataPointer<Playlist> playlist );

private slots:
    void updateInfo();

private:
    QExplicitlySharedDataPointer< Playlist > m_playlist;

    ElidedLabel *m_track, *m_artist, *m_album;
    ThumbnailWidget *m_cover;
};

TrackInfoDialog::TrackInfoDialog( QWidget* parent, Qt::WindowFlags f )
    : PlayerDialog( parent, f )
{
    static const int STRETCH_MAX = 1;

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin( 6 );
    layout->setSpacing( 6 );

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setMargin( 0 );
    vbox->setSpacing( 4 );

    m_cover = new ThumbnailWidget;
    int side = QtopiaApplication::style()->pixelMetric(QStyle::PM_SmallIconSize)*3 + vbox->spacing()*2;
    m_cover->setMinimumSize( QSize( side, side ) );

    layout->addWidget( m_cover );

    m_track = new ElidedLabel;
    vbox->addWidget( m_track );

    m_album = new ElidedLabel;
    vbox->addWidget( m_album );

    m_artist = new ElidedLabel;
    vbox->addWidget( m_artist );

    layout->addLayout( vbox, STRETCH_MAX );

    setLayout( layout );
}

void TrackInfoDialog::setPlaylist( QExplicitlySharedDataPointer<Playlist> playlist )
{
    // Disconnect from old playlist
    if( m_playlist.data() != NULL ) {
        m_playlist->disconnect( this );
    }

    m_playlist = playlist;

    // Connect to new playlist
    connect( m_playlist, SIGNAL(playingChanged(QModelIndex)),
        this, SLOT(updateInfo()) );

    updateInfo();
}

void TrackInfoDialog::updateInfo()
{
    QModelIndex playing = m_playlist->playing();

    QVariant variant = m_playlist->data( playing, Playlist::Title );
    if( variant.isValid() ) {
        m_track->setText( variant.toString() );
    } else {
        m_track->setText( tr( "Unknown Track" ) );
    }

    variant = m_playlist->data( playing, Playlist::Album );
    if( variant.isValid() ) {
        m_album->setText( variant.toString() );
    } else {
        m_album->setText( tr( "Unknown Album" ) );
    }

    variant = m_playlist->data( playing, Playlist::Artist );
    if( variant.isValid() ) {
        m_artist->setText( variant.toString() );
    } else {
        m_artist->setText( tr( "Unknown Artist" ) );
    }

    variant = m_playlist->data( playing, Playlist::AlbumCover );
    if( variant.isValid() ) {
        m_cover->setFile( variant.toString() );
        m_cover->show();
    } else {
        m_cover->hide();
    }
}

class TrackInfoWidget : public QWidget
{
    Q_OBJECT
public:
    TrackInfoWidget( QWidget* parent = 0 );

    void setPlaylist( QExplicitlySharedDataPointer<Playlist> playlist );

private slots:
    void updateInfo();

private:
    QExplicitlySharedDataPointer<Playlist> m_playlist;

    ElidedLabel *m_label;
};

TrackInfoWidget::TrackInfoWidget( QWidget* parent )
    : QWidget( parent )
{
    m_label = new ElidedLabel;
    m_label->setAlignment( Qt::AlignRight );

    QFont italicfont = font();
    italicfont.setItalic( true );

    m_label->setFont( italicfont );

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin( 0 );

    layout->addWidget( m_label );
    setLayout( layout );
}

void TrackInfoWidget::setPlaylist( QExplicitlySharedDataPointer<Playlist> playlist )
{
    // Disconnect from old playlist
    if( m_playlist.data() != NULL ) {
        m_playlist->disconnect( this );
    }

    m_playlist = playlist;

    // Connect to new playlist
    connect( m_playlist.data(), SIGNAL(playingChanged(QModelIndex)),
        this, SLOT(updateInfo()) );

    updateInfo();
}

void TrackInfoWidget::updateInfo()
{
    QModelIndex playing = m_playlist->playing();
    m_label->setText( m_playlist->data( playing, Playlist::Title ).toString() );
}

class ThrottleWidget : public QWidget
{
    Q_OBJECT
public:
    ThrottleWidget( QWidget* parent = 0 );

    void setOpacity( qreal opacity );

    // Set resolution between 0.0 and 1.0, default 0.1
    void setResolution( qreal resolution ) { m_resolution = resolution; }

    // Return current intensity between -1.0 and 1.0
    qreal intensity() const { return m_intensity; }

    QSize sizeHint() const { return QSize( QtopiaApplication::style()->pixelMetric(QStyle::PM_SmallIconSize), QtopiaApplication::style()->pixelMetric(QStyle::PM_SmallIconSize) ); }

signals:
    void pressed();
    void released();

    void intensityChanged( qreal intensity );

protected:
    void mousePressEvent( QMouseEvent* e );
    void mouseReleaseEvent( QMouseEvent* e );
    void mouseMoveEvent( QMouseEvent* e );

    void resizeEvent( QResizeEvent* e );
    void paintEvent( QPaintEvent* e );

private:
    qreal calculateIntensity( const QPoint& point );

    qreal m_intensity;
    qreal m_resolution;
    qreal m_opacity;

    QImage m_control;
    QPoint m_controlpos;
};

ThrottleWidget::ThrottleWidget( QWidget* parent )
    : QWidget( parent ), m_intensity( 0.0 ), m_resolution( 0.1 ), m_opacity( 1.0 )
{ }

void ThrottleWidget::setOpacity( qreal opacity )
{
    m_opacity = opacity;

    update();
}

void ThrottleWidget::mousePressEvent( QMouseEvent* e )
{
    m_intensity = calculateIntensity( e->pos() );

    emit pressed();
}

void ThrottleWidget::mouseReleaseEvent( QMouseEvent* )
{
    m_intensity = 0.0;

    emit released();
}

void ThrottleWidget::mouseMoveEvent( QMouseEvent* e )
{
    qreal intensity = calculateIntensity( e->pos() );
    qreal delta = m_intensity - intensity;
    if( delta < 0 ) {
        delta = -delta;
    }

    if( delta >= m_resolution  ) {
        m_intensity = intensity;

        emit intensityChanged( m_intensity );
    }
}

void ThrottleWidget::resizeEvent( QResizeEvent* )
{
    m_control = QImage();
}

void ThrottleWidget::paintEvent( QPaintEvent* )
{
    static const QString THROTTLE_CONTROL = ":image/mediaplayer/black/throttle";

    if( m_control.isNull() ) {
        QImageReader reader( THROTTLE_CONTROL );
        QSize scaled = reader.size();
        scaled.scale( size(), Qt::KeepAspectRatio );
        reader.setScaledSize( scaled );
        m_control = reader.read();
        m_controlpos = QPoint( (width() - m_control.width())/2, (height() - m_control.height())/2 );
    }

    if( m_opacity > 0.01 ) {
        QPainter painter( this );
        painter.setOpacity( m_opacity );

        painter.drawImage( m_controlpos, m_control );
    }
}

qreal ThrottleWidget::calculateIntensity( const QPoint& point )
{
    int center = rect().center().x();

    qreal intensity = (qreal)(point.x() - center) / (qreal)center;

    // Limit intensity between -1..1
    if( intensity < -1.0 ) {
        intensity = -1.0;
    }
    if( intensity > 1.0 ) {
        intensity = 1.0;
    }

    return intensity;
}

class ThrottleControl : public QWidget
{
    Q_OBJECT
public:
    ThrottleControl( QWidget* parent = 0 );

signals:
    void clicked();

    // Intensity changed to either -1, 0 or 1
    void intensityChanged( int intensity );

private slots:
    void processPressed();
    void processReleased();
    void processIntensityChange();
    void processTimeout();

    void activate();
    void deactivate();

    void setOpacity( qreal opacity );

private:
    enum State { Deactivated, PendingActivate, Activated, PendingDeactivate };

    State state() const { return m_state; }
    void setState( State state ) { m_state = state; }

    ThrottleWidget *m_throttle;

    QTimer *m_timer;
    int m_intensity;
    State m_state;
};

ThrottleControl::ThrottleControl( QWidget* parent )
    : QWidget( parent ), m_intensity( 0 ), m_state( Deactivated )
{
    static const int CONTROL_SENSITIVITY = 500; // Sensitivity to user actions in ms

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin( 0 );

    m_throttle = new ThrottleWidget;
    connect( m_throttle, SIGNAL(pressed()),
        this, SLOT(processPressed()) );
    connect( m_throttle, SIGNAL(released()),
        this, SLOT(processReleased()) );
    connect( m_throttle, SIGNAL(intensityChanged(qreal)),
        this, SLOT(processIntensityChange()) );
    m_throttle->setResolution( 0.2 );
    m_throttle->setOpacity( 0.0 );

    layout->addWidget( m_throttle );
    setLayout( layout );

    m_timer = new QTimer( this );
    connect( m_timer, SIGNAL(timeout()), this, SLOT(processTimeout()) );
    m_timer->setInterval( CONTROL_SENSITIVITY );
    m_timer->setSingleShot( true );
}

void ThrottleControl::processPressed()
{
    switch( state() )
    {
    case Deactivated:
        m_timer->start();
        setState( PendingActivate );
        break;
    case PendingDeactivate:
        m_timer->stop();
        setState( Activated );
        processIntensityChange();
        break;
    default:
        // Ignore
        break;
    }
}

void ThrottleControl::processReleased()
{
    switch( state() )
    {
    case PendingActivate:
        m_timer->stop();
        setState( Deactivated );
        emit clicked();
        break;
    case Activated:
        m_timer->start();
        setState( PendingDeactivate );
        processIntensityChange();
        break;
    default:
        // Ignore
        break;
    }
}

void ThrottleControl::processIntensityChange()
{
    static const qreal NEGATIVE_THRESHOLD = -0.35;
    static const qreal POSITIVE_THRESHOLD = 0.35;

    if( state() == Activated || state() == PendingDeactivate ) {
        int intensity = 0;

        qreal value = m_throttle->intensity();
        if( value <= NEGATIVE_THRESHOLD ) {
            intensity = -1;
        } else if( value >= POSITIVE_THRESHOLD ) {
            intensity = 1;
        }

        if( intensity != m_intensity ) {
            m_intensity = intensity;

            emit intensityChanged( m_intensity );
        }
    }
}

void ThrottleControl::processTimeout()
{
    switch( state() )
    {
    case PendingActivate:
        setState( Activated );
        activate();
        break;
    case PendingDeactivate:
        setState( Deactivated );
        deactivate();
    default:
        // Ignore
        break;
    }
}

void ThrottleControl::activate()
{
    static const int FADEIN_DURATION = 500;

    processIntensityChange();

    // Animate opacity
    QTimeLine *animation = new QTimeLine( FADEIN_DURATION, this );
    connect( animation, SIGNAL(valueChanged(qreal)),
        this, SLOT(setOpacity(qreal)) );
    connect( animation, SIGNAL(finished()),
        animation, SLOT(deleteLater()) );
    animation->start();
}

void ThrottleControl::deactivate()
{
    static const int FADEOUT_DURATION = 500;

    if( m_intensity != 0 ) {
        m_intensity = 0;
        emit intensityChanged( m_intensity );
    }

    // Animate opacity
    QTimeLine *animation = new QTimeLine( FADEOUT_DURATION, this );
    animation->setDirection( QTimeLine::Backward );
    connect( animation, SIGNAL(valueChanged(qreal)),
        this, SLOT(setOpacity(qreal)) );
    connect( animation, SIGNAL(finished()),
        animation, SLOT(deleteLater()) );
    animation->start();
}

void ThrottleControl::setOpacity( qreal opacity )
{
    static const qreal FULL_OPACITY = 0.65;

    m_throttle->setOpacity( opacity * FULL_OPACITY );
}

static const int KEY_LEFT_HOLD = Qt::Key_unknown + Qt::Key_Left;
static const int KEY_RIGHT_HOLD = Qt::Key_unknown + Qt::Key_Right;

class ThrottleKeyMapper : public QObject
{
    Q_OBJECT
public:
    ThrottleKeyMapper( ThrottleControl* control, QObject* parent );

    enum Mapping { LeftRight, UpDown };

    void setMapping( Mapping mapping ) { m_mapping = mapping; }

private slots:
    void processIntensityChange( int intensity );

private:
    Mapping m_mapping;
    int m_lastpressed;
};

ThrottleKeyMapper::ThrottleKeyMapper( ThrottleControl* control, QObject* parent )
    : QObject( parent ), m_mapping( LeftRight )
{
    connect( control, SIGNAL(intensityChanged(int)),
        this, SLOT(processIntensityChange(int)) );
}

void ThrottleKeyMapper::processIntensityChange( int intensity )
{
    switch( intensity )
    {
    case -1:
        {
        if( m_lastpressed ) {
            // Send release event
            processIntensityChange( 0 );
        }
        m_lastpressed = m_mapping == LeftRight ? KEY_LEFT_HOLD : Qt::Key_Down;
        QKeyEvent event = QKeyEvent( QEvent::KeyPress, m_lastpressed, Qt::NoModifier );
        QCoreApplication::sendEvent( parent(), &event );
        }
        break;
    case 0:
        if( m_lastpressed ) {
            QKeyEvent event = QKeyEvent( QEvent::KeyRelease, m_lastpressed, Qt::NoModifier );
            QCoreApplication::sendEvent( parent(), &event );
            m_lastpressed = 0;
        }
        break;
    case 1:
        {
        if( m_lastpressed ) {
            // Send release event
            processIntensityChange( 0 );
        }
        m_lastpressed = m_mapping == LeftRight ? KEY_RIGHT_HOLD : Qt::Key_Up;
        QKeyEvent event = QKeyEvent( QEvent::KeyPress, m_lastpressed, Qt::NoModifier );
        QCoreApplication::sendEvent( parent(), &event );
        }
        break;
    }
}

class PileLayout : public QLayout
{
public:
    PileLayout( QWidget* parent = 0 )
        : QLayout( parent )
    { }
    ~PileLayout();

    void addLayout( QLayout* layout );
    int count() const { return m_pile.count(); }
    void addItem( QLayoutItem* item );
    QSize sizeHint() const;
    QSize minimumSize() const;
    QLayoutItem *itemAt( int index ) const;
    QLayoutItem *takeAt( int index );
    void setGeometry( const QRect& rect );

private:
    QList<QLayoutItem*> m_pile;
};

PileLayout::~PileLayout()
{
    foreach( QLayoutItem* item, m_pile ) {
        delete item;
    }
}

void PileLayout::addLayout( QLayout* layout )
{
    QWidget* widget = new QWidget;
    widget->setLayout( layout );
    addWidget( widget );
}

void PileLayout::addItem( QLayoutItem* item )
{
    m_pile.append( item );
}

QSize PileLayout::sizeHint() const
{
    QSize hint( 0, 0 );

    foreach( QLayoutItem* item, m_pile ) {
        hint = hint.expandedTo( item->sizeHint() );
    }

    return hint;
}

QSize PileLayout::minimumSize() const
{
    QSize min( 0, 0 );

    foreach( QLayoutItem* item, m_pile ) {
        min = min.expandedTo( item->minimumSize() );
    }

    return min;
}


QLayoutItem* PileLayout::itemAt( int index ) const
{
    return m_pile.value( index );
}

QLayoutItem* PileLayout::takeAt( int index )
{
    if( index >= 0 && index < m_pile.count() ) {
        return m_pile.takeAt( index );
    }

    return 0;
}

void PileLayout::setGeometry( const QRect& rect )
{
    QLayout::setGeometry( rect );

    foreach( QLayoutItem* item, m_pile ) {
        item->setGeometry( rect );
    }
}

static const int KEY_SELECT_HOLD = Qt::Key_unknown + Qt::Key_Select;

PlayerWidget::PlayerWidget( PlayerControl* control, QWidget* parent )
    : QWidget( parent ),
      m_playercontrol( control ),
      m_content( 0 ),
      m_mediacontrol( 0 ),
#ifdef QTOPIA_KEYPAD_NAVIGATION
#ifndef NO_HELIX
      m_settingsaction( 0 ),
#endif
#endif
      m_videowidget( 0 ),
      m_muteaction( 0 ),
      m_muteicon( 0 ),
      m_voteaction( 0 ),
      m_votedialog( 0 ),
      m_repeataction( 0 ),
      m_repeatdialog( 0 )
{
    static const int HOLD_THRESHOLD = 500;
    static const int STRETCH_MAX = 1;

    m_repeatstate = new RepeatState( this );

    QVBoxLayout *background = new QVBoxLayout;
    background->setMargin( 0 );

    m_visualization = new VisualizationWidget;
    background->addWidget( m_visualization );
    setLayout( background );

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin( 6 );

    m_statewidget = new StateWidget( control );

    QHBoxLayout *hbox = new QHBoxLayout;
    hbox->addWidget( m_statewidget );

    m_muteicon = new QMediaVolumeLabel( QMediaVolumeLabel::MuteVolume );
    hbox->addWidget( m_muteicon );

    m_trackinfo = new TrackInfoWidget;
    hbox->addWidget( m_trackinfo, STRETCH_MAX );

    layout->addLayout( hbox );
    layout->addStretch();

    m_videolayout  = new QVBoxLayout;

#ifndef NO_HELIX
    m_helixlogoaudio = new HelixLogo;
    m_videolayout->addWidget( m_helixlogoaudio );
#endif

    m_videolayout->setMargin( 0 );

    layout->addLayout( m_videolayout, STRETCH_MAX );

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->setMargin( 0 );
    vbox->addStretch();

    m_progressview = new ProgressView( m_repeatstate );
    vbox->addWidget( m_progressview );

    m_volumeview = new VolumeView;
    vbox->addWidget( m_volumeview );

    m_seekview = new SeekView;
    vbox->addWidget( m_seekview );

    PileLayout *pile = new PileLayout;
    pile->addLayout( vbox );

    ThrottleControl *throttle = new ThrottleControl;
    connect( throttle, SIGNAL(clicked()), this, SLOT(cycleView()) );
    pile->addWidget( throttle );

    m_mapper = new ThrottleKeyMapper( throttle, this );

    hbox = new QHBoxLayout;
    hbox->setMargin( 0 );
    hbox->addLayout( pile );

#ifndef NO_HELIX
    m_helixlogovideo = new HelixLogo;
    hbox->addWidget( m_helixlogovideo );
#endif

    layout->addLayout( hbox );

    m_visualization->setLayout( layout );

    m_trackinfodialog = new TrackInfoDialog( this );

    mediaplayer::KeyFilter *filter = new mediaplayer::KeyFilter( m_trackinfodialog, this, this );
    filter->addKey( Qt::Key_Left );
    filter->addKey( Qt::Key_Right );

    QTimer::singleShot(1, this, SLOT(delayMenuCreation()));

    // Initialize view
    setView( Progress );

    m_ismute = false;
    m_muteicon->hide();

#ifndef NO_HELIX
    m_helixlogovideo->hide();
#endif

    new KeyHold( Qt::Key_Left, KEY_LEFT_HOLD, HOLD_THRESHOLD, this, this );
    new KeyHold( Qt::Key_Right, KEY_RIGHT_HOLD, HOLD_THRESHOLD, this, this );

    // Activity monitor
    m_monitor = new ActivityMonitor( 4000, this );
    connect( m_monitor, SIGNAL(inactive()), this, SLOT(showProgress()) );

    m_pingtimer = new QTimer( this );
    m_pingtimer->setInterval( 1000 );
    connect( m_pingtimer, SIGNAL(timeout()), this, SLOT(pingMonitor()) );

    m_context = new QMediaContentContext( this );
    m_context->addObject( m_progressview );
    m_context->addObject( m_volumeview );
    m_context->addObject( m_seekview );

    QMediaControlNotifier *notifier = new QMediaControlNotifier( QMediaControl::name(), this );
    connect( notifier, SIGNAL(valid()), this, SLOT(activate()) );
    connect( notifier, SIGNAL(invalid()), this, SLOT(deactivate()) );
    m_context->addObject( notifier );

    notifier = new QMediaControlNotifier( QMediaVideoControl::name(), this );
    connect( notifier, SIGNAL(valid()), this, SLOT(activateVideo()) );
    connect( notifier, SIGNAL(invalid()), this, SLOT(deactivateVideo()) );
    m_context->addObject( notifier );

    setFocusProxy( m_statewidget );

    // Filter application key events for media keys
    qApp->installEventFilter( this );

    // Context sensitive help hint
    setObjectName( "playback" );
}

PlayerWidget::~PlayerWidget()
{
    if( m_videowidget ) {
        delete m_videowidget;
    }
}

void PlayerWidget::setPlaylist( QExplicitlySharedDataPointer<Playlist> playlist )
{
    // Disconnect from old playlist
    if( m_playlist.data() != NULL ) {
        m_playlist->disconnect( this );
    }

    m_playlist = playlist;

    if ( m_playlist == NULL )
        return;

    // Connect to new playlist
    connect( m_playlist, SIGNAL(playingChanged(QModelIndex)),
        this, SLOT(playingChanged(QModelIndex)) );

    if( m_playlist ) {
        setCurrentTrack( m_playlist->playing() );
        if( !m_currenttrack.isValid() ) {
            setCurrentTrack( m_playlist->index( 0 ) );
        }

        if( m_currenttrack.isValid() ) {
            openCurrentTrack();
        }
    } else {
        qLog(Media) << "PlayerWidget::setPlaylist playlist is null";
    }

    // If playlist is a my shuffle playlist, enable voting and disable repeat
    // Otherwise, disable voting and enable repeat
    if( m_voteaction != NULL)
    {
        if( playlist->inherits("PlaylistMyShuffle") ) {
            m_voteaction->setVisible( true );
            m_repeataction->setVisible( false );
        } else {
            m_voteaction->setVisible( false );
            m_repeataction->setVisible( true );
        }
    }

    // Reset repeat state
    m_repeatstate->setState( RepeatState::RepeatNone );

    m_progressview->setPlaylist( m_playlist );
    m_trackinfo->setPlaylist( m_playlist );
    m_trackinfodialog->setPlaylist( m_playlist );
}

bool PlayerWidget::eventFilter( QObject* o, QEvent* e )
{
/*
    // Guard against recursion
    static QEvent* d = 0;
    if( o != this && d != e && (e->type() == QEvent::KeyPress || e->type() == QEvent::KeyRelease) ) {
        QKeyEvent *ke = (QKeyEvent*)e;

        if( ke->isAutoRepeat() ) { return false; }

        switch( ke->key() )
        {
        case Qt::Key_VolumeUp:
            {
            QKeyEvent event = QKeyEvent( e->type(), Qt::Key_VolumeUp, Qt::NoModifier );
            QCoreApplication::sendEvent( this, d = &event );
            d = 0;
            }
            return true;
        case Qt::Key_VolumeDown:
            {
            QKeyEvent event = QKeyEvent( e->type(), Qt::Key_VolumeDown, Qt::NoModifier );
            QCoreApplication::sendEvent( this, d = &event );
            d = 0;
            }
            return true;
        default:
            // Ignore
            break;
        }
    }
*/
    Q_UNUSED(o);
    Q_UNUSED(e);

    return false;
}

void PlayerWidget::setMediaContent( QMediaContent* content )
{
    if( m_content ) {
        m_content->disconnect( this );
    }

    m_content = content;

    if( content ) {
        connect( content, SIGNAL(mediaError(QString)),
            this, SLOT(displayErrorMessage(QString)) );
    }

    m_context->setMediaContent( content );
}

void PlayerWidget::activate()
{
    m_mediacontrol = new QMediaControl( m_content );
    connect( m_mediacontrol, SIGNAL(volumeMuted(bool)),
        this, SLOT(setMuteDisplay(bool)) );
    connect( m_mediacontrol, SIGNAL(playerStateChanged(QtopiaMedia::State)),
        this, SLOT(changeState(QtopiaMedia::State)) );
}

void PlayerWidget::deactivate()
{
    delete m_mediacontrol;
    m_mediacontrol = 0;
}

void PlayerWidget::activateVideo()
{
    QMediaVideoControl control( m_content );
    setVideo( control.createVideoWidget( this ) );
}

void PlayerWidget::deactivateVideo()
{
    removeVideo();
}

void PlayerWidget::displayErrorMessage( const QString& message )
{
    QMessageBox::warning( this, tr( "Media Player Error" ),
        QString( "<qt>%1</qt>" ).arg( message ) );
}

void PlayerWidget::changeState( QtopiaMedia::State state )
{
    switch (state) {
    case QtopiaMedia::Playing:
        if (m_videowidget != 0)
            QtopiaApplication::setPowerConstraint( QtopiaApplication::Disable );
        break;

    case QtopiaMedia::Paused:
        if (m_videowidget != 0)
            QtopiaApplication::setPowerConstraint( QtopiaApplication::Enable );
        break;

    case QtopiaMedia::Stopped:
        if( m_playercontrol->state() == PlayerControl::Playing ) {
            continuePlaying();
        }
        break;

    case QtopiaMedia::Error:
        m_playercontrol->setState( PlayerControl::Stopped );
        displayErrorMessage( m_mediacontrol->errorString() );
        break;
    default:
        // Ignore
        break;
    }
}

void PlayerWidget::setMuteDisplay( bool mute )
{
    if( m_ismute == mute ) {
        return;
    }

    m_ismute = mute;

    if( m_ismute ) {
        m_muteaction->setText( tr( "Mute Off" ) );
        m_muteicon->show();
    } else {
        m_muteaction->setText( tr( "Mute On" ) );
        m_muteicon->hide();
    }
}

void PlayerWidget::playingChanged( const QModelIndex& index )
{
    if( index.isValid() ) {
        setCurrentTrack( index );
        openCurrentTrack();
    } else {
        m_playercontrol->setState( PlayerControl::Stopped );
    }

    // If repeat state is repeat one, reset repeat
    if( m_repeatstate->state() == RepeatState::RepeatOne ) {
        m_repeatstate->setState( RepeatState::RepeatNone );
    }
}

void PlayerWidget::pingMonitor()
{
    m_monitor->update();
}

void PlayerWidget::showProgress()
{
    setView( Progress );
}

void PlayerWidget::cycleView()
{
    switch( view() )
    {
    case Progress:
        setView( Volume );
        m_monitor->update();
        break;
    case Seek:
        setView( Volume );
        m_monitor->update();
        break;
    case Volume:
        setView( Progress );
        break;
    }
}

void PlayerWidget::continuePlaying()
{
    // If repeat state is repeat one, play again
    // Otherwise, skip forward one playlist item
    if (m_repeatstate->state() == RepeatState::RepeatOne ||
        (m_repeatstate->state() == RepeatState::RepeatAll && m_playlist->rowCount() == 1)) {
        m_mediacontrol->start();
    } else {
        QModelIndex index = m_currenttrack.sibling( m_currenttrack.row() + 1, m_currenttrack.column() );

        if (index.isValid()) {
            m_playlist->setPlaying(index);
        } else {
            // If repeat state is repeat all, play from begining
            if (m_repeatstate->state() == RepeatState::RepeatAll)
                m_playlist->setPlaying(m_playlist->index(0));
            else {
                m_playercontrol->setState(PlayerControl::Stopped);

                if (m_videowidget != 0)
                    QtopiaApplication::setPowerConstraint( QtopiaApplication::Enable );
            }
        }
    }
}

void PlayerWidget::toggleMute()
{
    if (m_mediacontrol)
        m_mediacontrol->setMuted(!m_mediacontrol->isMuted());
}

void PlayerWidget::execSettings()
{
#ifndef NO_HELIX
    MediaPlayerSettingsDialog settingsdialog( this );
    QtopiaApplication::execDialog( &settingsdialog );
#endif
}

void PlayerWidget::keyPressEvent( QKeyEvent* e )
{
    static const unsigned int REPEAT_THRESHOLD = 3000; // 3 seconds

    if( e->isAutoRepeat() || !m_mediacontrol ) { e->ignore(); return; }

    switch( e->key() )
    {
    case Qt::Key_Up:
//    case Qt::Key_VolumeUp:
        {
            e->accept();
            setView( Volume );

            QKeyEvent event = QKeyEvent( QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier );
            QCoreApplication::sendEvent( m_volumeview->keyEventHandler(), &event );

            m_pingtimer->start();
            m_monitor->update();
        }
        break;
    case Qt::Key_Down:
//    case Qt::Key_VolumeDown:
        {
            e->accept();
            setView( Volume );

            QKeyEvent event = QKeyEvent( QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier );
            QCoreApplication::sendEvent( m_volumeview->keyEventHandler(), &event );

            m_pingtimer->start();
            m_monitor->update();
        }
        break;
    case Qt::Key_Left:
        // If more than the repeat threshold into the track, seek to the beginning
        // Otherwise, skip backward one playlist item
        e->accept();
        if( m_mediacontrol->position() > REPEAT_THRESHOLD ) {
            m_mediacontrol->seek( 0 );
        } else {
            QModelIndex index = m_currenttrack.sibling( m_currenttrack.row() - 1, m_currenttrack.column() );
            if( index.isValid() ) {
                m_playlist->setPlaying( index );
            }
        }
        break;
    case Qt::Key_Right:
        {
            e->accept();
            // Skip forward one playlist item
            QModelIndex index = m_currenttrack.sibling( m_currenttrack.row() + 1, m_currenttrack.column() );
            if( index.isValid() ) {
                m_playlist->setPlaying( index );
            } else {
                // If repeat state is repeat all, skip to beginning
                if( m_repeatstate->state() == RepeatState::RepeatAll ) {
                    m_playlist->setPlaying( m_playlist->index( 0 ) );
                }
            }
        }
        break;
    case KEY_LEFT_HOLD:
        e->accept();
        if( m_playercontrol->state() != PlayerControl::Stopped && m_mediacontrol->length() > 0 ) {
            setView( Seek);

            QKeyEvent event = QKeyEvent( QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier );
            QCoreApplication::sendEvent( m_seekview->keyEventHandler(), &event );

            m_pingtimer->start();
            m_monitor->update();
        }
        break;
    case KEY_RIGHT_HOLD:
        e->accept();
        if( m_playercontrol->state() != PlayerControl::Stopped && m_mediacontrol->length() > 0 ) {
            setView( Seek );

            QKeyEvent event = QKeyEvent( QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier );
            QCoreApplication::sendEvent( m_seekview->keyEventHandler(), &event );

            m_pingtimer->start();
            m_monitor->update();
        }
        break;
    case Qt::Key_1:
        e->accept();
        if( !qobject_cast<MyShufflePlaylist*>(m_playlist) ) {
            m_repeatstate->setState( RepeatState::RepeatOne );
        }
        break;
    case Qt::Key_Asterisk:
        e->accept();
        if( !qobject_cast<MyShufflePlaylist*>(m_playlist) ) {
            m_repeatstate->setState( RepeatState::RepeatAll );
        }
        break;
    case Qt::Key_0:
        e->accept();
        if( !qobject_cast<MyShufflePlaylist*>(m_playlist) ) {
            m_repeatstate->setState( RepeatState::RepeatNone );
        }
        break;
    default:
        // Ignore
        QWidget::keyPressEvent(e);
        break;
    }
}

void PlayerWidget::keyReleaseEvent( QKeyEvent* e )
{
    if( e->isAutoRepeat() || !m_mediacontrol ) { e->ignore(); return; }

    switch( e->key() )
    {
    case Qt::Key_Up:
//    case Qt::Key_VolumeUp:
        {
            e->accept();
            QKeyEvent event = QKeyEvent( QEvent::KeyRelease, Qt::Key_Left, Qt::NoModifier );
            QCoreApplication::sendEvent( m_volumeview->keyEventHandler(), &event );

            m_pingtimer->stop();
        }
        break;
    case Qt::Key_Down:
//    case Qt::Key_VolumeDown:
        {
            e->accept();
            QKeyEvent event = QKeyEvent( QEvent::KeyRelease, Qt::Key_Right, Qt::NoModifier );
            QCoreApplication::sendEvent( m_volumeview->keyEventHandler(), &event );

            m_pingtimer->stop();
        }
        break;
    case KEY_LEFT_HOLD:
        if( m_playercontrol->state() != PlayerControl::Stopped && m_currentview == Seek ) {
            e->accept();
            QKeyEvent event = QKeyEvent( QEvent::KeyRelease, Qt::Key_Left, Qt::NoModifier );
            QCoreApplication::sendEvent( m_seekview->keyEventHandler(), &event );

            m_pingtimer->stop();
        }
        break;
    case KEY_RIGHT_HOLD:
        if( m_playercontrol->state() != PlayerControl::Stopped && m_currentview == Seek ) {
            e->accept();
            QKeyEvent event = QKeyEvent( QEvent::KeyRelease, Qt::Key_Right, Qt::NoModifier );
            QCoreApplication::sendEvent( m_seekview->keyEventHandler(), &event );

            m_pingtimer->stop();
        }
        break;
    default:
        // Ignore
        QWidget::keyReleaseEvent(e);
    }
}

void PlayerWidget::showEvent( QShowEvent* )
{
    if( m_videowidget ) {
        QtopiaApplication::setPowerConstraint( QtopiaApplication::Disable );
    }

#ifndef NO_VISUALIZATION
    else {
        m_visualization->setActive( true );
    }
#endif
}

void PlayerWidget::hideEvent( QHideEvent* )
{
    if( m_videowidget ) {
        QtopiaApplication::setPowerConstraint( QtopiaApplication::Enable );
    }

#ifndef NO_VISUALIZATION
    m_visualization->setActive( false );
#endif
}

void PlayerWidget::setView( View view )
{
    m_currentview = view;

    switch( m_currentview )
    {
    case Progress:
        m_volumeview->hide();
        m_seekview->hide();

        m_progressview->show();
        m_mapper->setMapping( ThrottleKeyMapper::LeftRight );
        break;
    case Volume:
        m_progressview->hide();
        m_seekview->hide();

        m_volumeview->show();
        m_mapper->setMapping( ThrottleKeyMapper::UpDown );
        break;
    case Seek:
        m_progressview->hide();
        m_volumeview->hide();

        m_seekview->show();
        m_mapper->setMapping( ThrottleKeyMapper::LeftRight );
        break;
    }
}

void PlayerWidget::setVideo( QWidget* widget )
{
    m_videowidget = widget;
    m_videowidget->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding );
    m_videolayout->addWidget( m_videowidget );

#ifndef NO_VISUALIZATION
    m_visualization->setActive( false );
#endif

#ifndef NO_HELIX
    m_helixlogoaudio->hide();
    m_helixlogovideo->show();
#endif

    QtopiaApplication::setPowerConstraint( QtopiaApplication::Disable );
}

void PlayerWidget::removeVideo()
{
    delete m_videowidget;
    m_videowidget = 0;

#ifndef NO_VISUALIZATION
    m_visualization->setActive( true );
#endif

#ifndef NO_HELIX
    m_helixlogoaudio->show();
    m_helixlogovideo->hide();
#endif

    QtopiaApplication::setPowerConstraint( QtopiaApplication::Enable );
}

void PlayerWidget::setCurrentTrack( const QModelIndex& index )
{
    m_currenttrack = index;
}

void PlayerWidget::openCurrentTrack()
{
    // Open and play current playlist item
    QString url = qvariant_cast<QString>(m_playlist->data( m_currenttrack, Playlist::Url ));

    if( url.contains( "://" ) ) {
        m_playercontrol->open( url );
    } else {
        m_playercontrol->open( QContent( url ) );
    }
}

void PlayerWidget::execTrackInfoDialog()
{
    QtopiaApplication::execDialog( m_trackinfodialog, false );
}

void PlayerWidget::execVoteDialog()
{
    if( m_votedialog == NULL )
    {
        m_votedialog = new VoteDialog( this );
        connect( m_votedialog, SIGNAL(snoozeVoted()), this, SLOT(continuePlaying()) );
        connect( m_votedialog, SIGNAL(banVoted()), this, SLOT(continuePlaying()) );
    }
    QtopiaApplication::execDialog( m_votedialog, false );
}

void PlayerWidget::execRepeatDialog()
{
    if( m_repeatdialog == NULL )
        m_repeatdialog = new RepeatDialog( m_repeatstate, this );

    QtopiaApplication::execDialog( m_repeatdialog, false );
}

void PlayerWidget::delayMenuCreation()
{
    // Construct soft menu bar
#ifdef QTOPIA_KEYPAD_NAVIGATION
    QMenu *menu = QSoftMenuBar::menuFor( this );

    m_muteaction = new QAction( QIcon( ":icon/mute" ), tr( "Mute" ), this );
    connect( m_muteaction, SIGNAL(triggered()), this, SLOT(toggleMute()) );
    menu->addAction( m_muteaction );
    m_muteaction->setText( tr( "Mute On" ) );

    m_voteaction = new QAction( QIcon( ":icon/mediaplayer/black/vote" ), tr( "Vote..." ), this );
    connect( m_voteaction, SIGNAL(triggered()), this, SLOT(execVoteDialog()) );
    menu->addAction( m_voteaction );

    m_repeataction = new QAction( QIcon( ":icon/mediaplayer/black/repeat" ), tr( "Repeat..." ), this );
    connect( m_repeataction, SIGNAL(triggered()), this, SLOT(execRepeatDialog()) );
    menu->addAction( m_repeataction );

    QAction *trackdetails = new QAction( QIcon( ":icon/info" ), tr( "Track Details..." ), this );
    connect( trackdetails, SIGNAL(triggered()), this, SLOT(execTrackInfoDialog()) );
    menu->addAction( trackdetails );

    menu->addSeparator();

#ifndef NO_HELIX
    m_settingsaction = new QAction( QIcon( ":icon/settings" ), tr( "Settings..." ), this );
    connect( m_settingsaction, SIGNAL(triggered()), this, SLOT(execSettings()) );
    menu->addAction( m_settingsaction );
#endif

    m_voteaction->setVisible( false );
    m_repeataction->setVisible( true );
#endif
}

#include "playerwidget.moc"
