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

#include "mmscomposer.h"

#include <qsoftmenubar.h>
#include <qtopiaapplication.h>
#include <qcolorselector.h>
#include <qtopia/mail/qmailmessage.h>
#include <qmimetype.h>
#include <qaudiosourceselector.h>
#include <qimagesourceselector.h>

#include <QAction>
#include <QBuffer>
#include <QPainter>
#include <QLayout>
#include <QStackedWidget>
#include <QDialog>
#include <QImage>
#include <QImageReader>
#include <QSpinBox>
#include <QFile>
#include <QStringList>
#include <QDir>
#include <QTextStream>
#include <QDataStream>
#include <QBitArray>
#include <QXmlDefaultHandler>
#include <QMouseEvent>
#include <QDesktopWidget>
#include <QMenu>

MMSSlideImage::MMSSlideImage(QWidget *parent)
    : QLabel(parent)
{
    setAlignment( Qt::AlignCenter );
    connect( this, SIGNAL(clicked()), this, SLOT(select()) );
    setImage( QContent() );
    setFocusPolicy( Qt::StrongFocus );
    setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );
    // would like to specify a value relative to parent here but Qt makes it hard..
    setMinimumSize( 0, 30 );
}

QContent& MMSSlideImage::document()
{
    return m_content;
}

void MMSSlideImage::mousePressEvent( QMouseEvent *event )
{
    if( rect().contains( event->pos() ) )
        m_pressed = true;
}

void MMSSlideImage::mouseReleaseEvent( QMouseEvent *event )
{
    if( m_pressed && rect().contains( event->pos() ) )
        emit clicked();
    m_pressed = false;
}

void MMSSlideImage::keyPressEvent( QKeyEvent *event )
{
    if( event->type() == QEvent::KeyPress )
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        switch( keyEvent->key() )
        {
            case Qt::Key_Left:
                emit leftPressed();
                break;
            case Qt::Key_Right:
                emit rightPressed();
                break;
            case Qt::Key_Select:
                emit clicked();
                break;
            default:
                QLabel::keyPressEvent( event );
                break;
        }
    }
}

void MMSSlideImage::paintEvent( QPaintEvent *event )
{
    QLabel::paintEvent( event );
    if( hasFocus() )
    {
        QPainter p( this );
        QPen pen(palette().highlight().color());
        p.setPen( pen );
        p.drawRect( 0, 0, width(), height() );
        p.drawRect( 1, 1, width()-2, height()-2 );
    }
}

void MMSSlideImage::showEvent(QShowEvent* event)
{
    QLabel::showEvent(event);

    if (m_content.isValid() && m_image.isNull())
    {
        // We have deferred loading this image
        QPixmap pixmap(loadImage());
        m_contentSize = pixmap.size();
        setImage( pixmap );
    }

    if (!m_image.isNull())
    {
        m_image = scale( m_image );
        setPixmap( m_image );
    }
}

QRect MMSSlideImage::contentsRect() const
{
    if (isEmpty())
        return QRect();

    QPoint pnt( rect().x() + (width() - m_image.width()) / 2 + 2,
                rect().y() + (height() - m_image.height()) / 2 + 2);
    pnt = mapToParent( pnt );
    return QRect( pnt.x(), pnt.y(), m_image.width(), m_image.height() );
}

QSize MMSSlideImage::sizeHint() const
{
    QWidget *par = 0;
    if( parent() && parent()->isWidgetType() )
        par = static_cast<QWidget *>(parent());
    QRect mwr = QApplication::desktop()->availableGeometry();
    int w = par ? par->width() : mwr.width(),
        h = par ? par->height() : mwr.height();
    return QSize(w/3*2, h/3*2);
}

void MMSSlideImage::select()
{
    QImageSourceSelectorDialog *selector = new QImageSourceSelectorDialog(this);
    selector->setObjectName("slideImageSelector");
    selector->setMaximumImageSize(QSize(80, 96));
    selector->setContent(m_content);
    selector->setModal(true);
    selector->setWindowTitle(tr("Slide photo"));

    int result = QtopiaApplication::execDialog( selector );
    if( result == QDialog::Accepted ) {
        setImage( selector->content() );
    }
    delete selector;
}

void MMSSlideImage::resizeEvent( QResizeEvent *event )
{
    QLabel::resizeEvent( event );

    if (isVisible()) {
        if( !m_image.isNull() ) {
            m_image = scale( m_image );
            setPixmap( m_image );
        }
    }
}

QPixmap MMSSlideImage::scale( const QPixmap &src ) const
{
    if (src.isNull())
        return src;

    if( (src.width() >= width()) || (src.height() >= height()) )
        return src.scaled( size(), Qt::KeepAspectRatio, Qt::SmoothTransformation );

    return src;
}

QPixmap MMSSlideImage::loadImage() const
{
    // Load the image to fit our display
    QImageReader imageReader( m_content.open() );

    if (imageReader.supportsOption(QImageIOHandler::Size)) {
        QSize fileSize(imageReader.size());

        QSize bounds(isVisible() ? size() : QApplication::desktop()->availableGeometry().size());

        // See if the image needs to be scaled during load
        if ((fileSize.width() > bounds.width()) || (fileSize.height() > bounds.height()))
        {
            // And the loaded size should maintain the image aspect ratio
            QSize imageSize(fileSize);
            imageSize.scale(bounds, Qt::KeepAspectRatio);
            imageReader.setScaledSize(imageSize);
        }
    }

    return QPixmap::fromImage( imageReader.read() );
}

void MMSSlideImage::setImage( const QContent& document )
{
    m_content = document;

    QPixmap pixmap;
    if (!m_content.isValid())
    {
        m_contentSize = QSize();
    }
    else if (isVisible())
    {
        pixmap = loadImage();
        m_contentSize = pixmap.size();
    }

    setImage( pixmap );
}

void MMSSlideImage::setImage( const QPixmap& image )
{
    m_image = (isVisible() ? scale( image ) : image);

    if( m_image.isNull() && !m_content.isValid() ) {
        setText( tr("Slide image") );
    } else if (isVisible()) {
        setPixmap( m_image );
    }

    emit changed();
}

QPixmap MMSSlideImage::image() const
{
    if (m_content.isValid()) {
        QImageReader imageReader( m_content.open() );
        return QPixmap::fromImage( imageReader.read() );
    }

    return QPixmap();
}

bool MMSSlideImage::isEmpty() const
{
    return (m_image.isNull() && !m_content.isValid());
}


MMSSlideText::MMSSlideText(QWidget *parent)
    : QTextEdit(parent), defaultText( QObject::tr("Your text here...") ), m_hasFocus( false )
{
    setSizePolicy( QSizePolicy( QSizePolicy::Expanding,
                                            QSizePolicy::MinimumExpanding ) );
    setWordWrapMode(QTextOption::WordWrap);
    setText( QString() );
}

bool MMSSlideText::event( QEvent *e )
{
    bool a = QTextEdit::event( e );
    if( e->type() == QEvent::EnterEditFocus && text().isNull() ) {
        clear();
    } else if( ( ( e->type() == QEvent::LeaveEditFocus ) ||
                 ( e->type() == QEvent::FocusOut && m_hasFocus ) ) && 
               ( text().isEmpty() ) ) {
        // Reset the text back to the placeholder
        setText( QString() );
    }

    return a;
}

void MMSSlideText::mousePressEvent( QMouseEvent * )
{
    if( !m_hasFocus )
    {
        if (text().isNull())
            clear();
        else
            selectAll();
        m_hasFocus = true;
    }
}

void MMSSlideText::keyPressEvent( QKeyEvent *e )
{
    if (!Qtopia::mousePreferred()) {
        if (!hasEditFocus()) {
            if (e->key() == Qt::Key_Left) {
                emit leftPressed();
                e->accept();
                return;
            } else if (e->key() == Qt::Key_Right) {
                emit rightPressed();
                e->accept();
                return;
            }
            //else fall through
        }
    }
    QTextEdit::keyPressEvent( e );
    updateGeometry();
}

QRect MMSSlideText::contentsRect() const
{
    if( text().isNull() )
        return QRect();

    QPoint pnt = rect().topLeft();
    pnt = mapToParent( pnt );
    return QRect( pnt.x(), pnt.y(), rect().width()-2, rect().height() - 2 );
}

void MMSSlideText::setText( const QString &txt )
{
    if( txt.trimmed().isEmpty() ) {
        QTextEdit::setPlainText( defaultText );
        selectAll();
    } else {
        QTextEdit::setPlainText( txt );
    }
    updateGeometry();
}

QString MMSSlideText::text() const
{
    QString t = QTextEdit::toPlainText().simplified();
    if( t == MMSSlideText::defaultText )
        t = QString();
    return t;
}

QSize MMSSlideText::sizeHint() const
{
    QFontMetrics fm( font() );
    return QSize( QTextEdit::sizeHint().width(),
                    qMax( fm.boundingRect( 0, 0, width()-2, 32768,
                        Qt::TextWordWrap | Qt::AlignHCenter, text() ).height()+2,
                        QApplication::globalStrut().height() ) );
}

bool MMSSlideText::isEmpty() const
{
    return text().isEmpty();
}


//---------------------------------------------------------------------------

MMSSlideAudio::MMSSlideAudio(QWidget *parent)
    : QPushButton(parent)
{
    setIcon(QIcon(":icon/sound"));
    connect(this, SIGNAL(clicked()), this, SLOT(select()));
}

QContent& MMSSlideAudio::document()
{
    return audioContent;
}

void MMSSlideAudio::setAudio( const QContent &doc )
{
    audioContent = doc;
    audioData.resize(0);
    audioName = QString();
    audioType = QString();

    setText(audioContent.name());

    emit changed();
}

void MMSSlideAudio::setAudio( const QByteArray &d, const QString &loc )
{
    audioContent = QContent();
    audioData = d;
    audioName = loc;

    setText(loc.toLatin1());
}

QByteArray MMSSlideAudio::audio() const
{
    if (audioContent.isValid())
    {
        QIODevice* io = audioContent.open();
        audioData = io->readAll();
        delete io;
    }

    return audioData;
}

void MMSSlideAudio::select()
{
    QAudioSourceSelectorDialog *selector = new QAudioSourceSelectorDialog(this);
    selector->setObjectName("slideAudioSelector");
    selector->setDefaultAudio("audio/amr", "amr", 8000, 1);
    if (audioContent.isValid())
        selector->setContent(audioContent);
    else
        selector->setContent(QContent(audioName));
    selector->setWindowTitle(tr("Slide audio"));
    selector->setModal(true);

    int result = QtopiaApplication::execDialog( selector );
    if ( result == QDialog::Accepted ) {
        setAudio(selector->content());
    }
    delete selector;
}

QString MMSSlideAudio::mimeType() const
{
    if (audioType.isEmpty()) {
        // guess
        char buf[7];
        memcpy(buf, audioData.data(), qMin(6, audioData.size()));
        buf[6] = '\0';
        QString head(buf);
        if (head == "#!AMR") {
            audioType = "audio/amr";
        } else if (head == "RIFF") {
            audioType = "audio/x-wav";
        }
    }

    return audioType;
}

void MMSSlideAudio::keyPressEvent( QKeyEvent *e )
{
    switch (e->key()) {
        case Qt::Key_Left:
            emit leftPressed();
            break;
        case Qt::Key_Right:
            emit rightPressed();
            break;
        default:
            QPushButton::keyPressEvent(e);
    }
}

bool MMSSlideAudio::isEmpty() const
{
    return !audioContent.isValid();
}


//===========================================================================

MMSSlide::MMSSlide(QWidget *parent)
    : QWidget(parent), m_duration( 5000 )
{
    setFocusPolicy( Qt::NoFocus );
    setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );
    QVBoxLayout *l = new QVBoxLayout( this );
    l->setMargin(0);
    l->setSpacing(0);

    m_imageContent = new MMSSlideImage( this );
    l->addWidget( m_imageContent, 6 );
    connect( m_imageContent, SIGNAL(leftPressed()), this, SIGNAL(leftPressed()) );
    connect( m_imageContent, SIGNAL(rightPressed()), this, SIGNAL(rightPressed()) );

    m_textContent = new MMSSlideText( this );
    l->addWidget( m_textContent, 3 );
    connect( m_textContent, SIGNAL(leftPressed()), this, SIGNAL(leftPressed()) );
    connect( m_textContent, SIGNAL(rightPressed()), this, SIGNAL(rightPressed()) );

    m_audioContent = new MMSSlideAudio( this );
    l->addWidget( m_audioContent, 1 );
    connect( m_audioContent, SIGNAL(leftPressed()), this, SIGNAL(leftPressed()) );
    connect( m_audioContent, SIGNAL(rightPressed()), this, SIGNAL(rightPressed()) );
}

void MMSSlide::setDuration( int t )
{
    if( t != m_duration )
    {
        m_duration = t;
        emit durationChanged( m_duration );
    }
}

int MMSSlide::duration() const
{
    return m_duration;
}

MMSSlideImage *MMSSlide::imageContent() const
{
    return m_imageContent;
}

MMSSlideText *MMSSlide::textContent() const
{
    return m_textContent;
}

MMSSlideAudio *MMSSlide::audioContent() const
{
    return m_audioContent;
}

//==============================================================================

MMSComposer::MMSComposer(QWidget *parent)
    : QWidget(parent), m_curSlide(-1), m_internalUpdate(false)
{
    setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Expanding ) );
    setFocusPolicy( Qt::NoFocus );

    m_durationLabel = new QLabel( this );

    m_slideLabel = new QLabel( this );
    m_slideLabel->setAlignment( Qt::AlignRight | Qt::AlignVCenter );

    m_slideStack = new QStackedWidget( this );
    m_slideStack->setFocusPolicy( Qt::NoFocus );

    m_removeSlide = new QAction(tr("Remove Slide"), this);
    connect( m_removeSlide, SIGNAL(triggered()), this, SLOT(removeSlide()) );

    QHBoxLayout *labelLayout = new QHBoxLayout;
    labelLayout->addWidget( m_durationLabel );
    labelLayout->addWidget( m_slideLabel );

    QVBoxLayout *l = new QVBoxLayout( this );
    l->setMargin(0);
    l->addLayout( labelLayout );
    l->addWidget( m_slideStack, 1 );

    connect( this, SIGNAL(currentChanged(uint)), this, SLOT(updateLabels()) );

    QSoftMenuBar::setLabel(this, Qt::Key_Back, QSoftMenuBar::Cancel);

    addSlide();
}

MMSComposer::~MMSComposer()
{
    qDeleteAll(m_slides);
}

void MMSComposer::addActions(QMenu* menu)
{
    QAction *add = new QAction(tr("Add Slide"), this);
    connect( add, SIGNAL(triggered()), this, SLOT(addSlide()) );

    QAction* options = new QAction(tr("Slide Options..."), this);
    connect( options, SIGNAL(triggered()), this, SLOT(slideOptions()) );

    menu->addAction(add);
    menu->addAction(m_removeSlide);
    menu->addSeparator();
    menu->addAction(options);
}

void MMSComposer::slideOptions()
{
    MMSSlide *cur = slide( currentSlide() );
    if( !cur )
        return;
    QDialog *dlg = new QDialog(this);
    dlg->setModal(true);
    dlg->setWindowTitle( tr("Slide options") );
    QGridLayout *l = new QGridLayout( dlg );
    int rowCount = 0;

    QSpinBox *durBox = new QSpinBox( dlg );
    durBox->setMinimum( 1 );
    durBox->setMaximum( 10 );
    durBox->setValue( cur->duration()/1000 );
    durBox->setSuffix( tr("secs") );
    QLabel *la = new QLabel( tr("Duration", "duration between images in a slide show"), dlg );
    la->setBuddy( durBox );
    l->addWidget( la, rowCount, 0 );
    l->addWidget( durBox, rowCount, 1 );
    ++rowCount;

    QColorButton *bg = new QColorButton( dlg );
    bg->setColor( backgroundColor() );
    la = new QLabel( tr("Slide color"), dlg );
    la->setBuddy( bg );
    l->addWidget( la, rowCount, 0 );
    l->addWidget( bg, rowCount, 1 );

    int r = QtopiaApplication::execDialog( dlg );
    if( r == QDialog::Accepted )
    {
        setBackgroundColor( bg->color() );

        cur->setDuration( durBox->value()*1000 );
    }
}

QRect MMSComposer::contentsRect() const
{
    QRect r = rect();
    r.setHeight( r.height() - qMax( m_slideLabel->height(), m_durationLabel->height() ) );
    return r;
}

void MMSComposer::addSlide()
{
    addSlide( -1 );
}

void MMSComposer::addSlide( int a_slide )
{
    if( a_slide < 0 )
    {
        if( currentSlide() == -1 )
            a_slide = 0;
        else
            a_slide = currentSlide();
    }
    else if( a_slide >= static_cast<int>(slideCount()) )
    {
        a_slide = slideCount() - 1;
    }
    if( slideCount() )
        ++a_slide; // add to the next slide

    MMSSlide *newSlide = new MMSSlide( m_slideStack );
    connect( newSlide, SIGNAL(leftPressed()), this, SLOT(previousSlide()) );
    connect( newSlide, SIGNAL(rightPressed()), this, SLOT(nextSlide()) );
    connect( newSlide, SIGNAL(durationChanged(int)), this, SLOT(updateLabels()) );
    m_slides.insert( a_slide, newSlide );
    m_slideStack->addWidget(newSlide);

    QMenu *thisMenu = QSoftMenuBar::menuFor( this );
    QSoftMenuBar::addMenuTo( newSlide, thisMenu );
    QSoftMenuBar::addMenuTo( newSlide->m_textContent, thisMenu );
    QSoftMenuBar::addMenuTo( newSlide->m_imageContent, thisMenu );

    connect( newSlide->m_textContent, SIGNAL(textChanged()), this, SLOT(elementChanged()) );
    connect( newSlide->m_imageContent, SIGNAL(changed()), this, SLOT(elementChanged()) );
    connect( newSlide->m_audioContent, SIGNAL(changed()), this, SLOT(elementChanged()) );

    m_removeSlide->setVisible(slideCount() > 1);

    m_internalUpdate = true;
    setCurrentSlide( a_slide );
}

void MMSComposer::removeSlide()
{
    removeSlide( -1 );
}

void MMSComposer::removeSlide( int a_slide )
{
    if( slideCount() <= 1 )
        return;
    int s = a_slide;
    if( s == -1 )
        s = currentSlide();
    if( s < 0 || s >= static_cast<int>(slideCount()) )
        return;
    m_slideStack->removeWidget( slide( s ) );
    delete m_slides.takeAt( s );
    if( s >= static_cast<int>(slideCount()) )
        s = slideCount() - 1;
    if( s >= 0 )
        m_internalUpdate = true;
    setCurrentSlide( s );

    m_removeSlide->setVisible(slideCount() > 1);
}

void MMSComposer::setTextColor( const QColor &col )
{
    m_textColor = col;
    QPalette pal = m_slideStack->palette();
    pal.setColor( QPalette::Foreground, m_textColor );
    pal.setColor( QPalette::Text, m_textColor );
    m_slideStack->setPalette( pal );
}

QColor MMSComposer::textColor() const
{
    return m_textColor;
}

QColor MMSComposer::backgroundColor() const
{
    return m_backgroundColor;
}

void MMSComposer::setBackgroundColor( const QColor &col )
{
    m_backgroundColor = col;

    // Set the FG to a contrasting colour
    int r, g, b;
    col.getRgb(&r, &g, &b);
    m_textColor = (((r + g + b) / 3) > 128 ? Qt::black : Qt::white);

    QPalette pal = m_slideStack->palette();
    pal.setColor( QPalette::Background, m_backgroundColor );
    pal.setColor( QPalette::Base, m_backgroundColor );
    pal.setColor( QPalette::Foreground, m_textColor );
    pal.setColor( QPalette::Text, m_textColor );
    m_slideStack->setPalette( pal );
}

void MMSComposer::setCurrentSlide( int a_slide )
{
    if( a_slide >= static_cast<int>(slideCount()) )
        return;
    if( a_slide < 0 )
    {
        m_curSlide = -1;
        return;
    }
    if( m_internalUpdate || a_slide != m_curSlide )
    {
        m_internalUpdate = false;
        m_curSlide = a_slide;
        m_slideStack->setCurrentWidget( slide( m_curSlide ) );
        emit currentChanged( m_curSlide );
    }
}

void MMSComposer::nextSlide()
{
    if( !slideCount() )
        return;
    int cur = currentSlide();
    if( cur == -1 || ++cur >= static_cast<int>(slideCount()) )
        cur = 0;
    setCurrentSlide( cur );
}

void MMSComposer::previousSlide()
{
    if( !slideCount() )
        return;
    int cur = currentSlide();
    --cur;
    if( cur < 0 )
        cur = slideCount() - 1;
    setCurrentSlide( cur );
}

void MMSComposer::updateLabels()
{
    QString baseLabel = tr("Slide %1 of %2");
    m_slideLabel->setText( baseLabel.arg( QString::number( currentSlide()+1 ) )
                               .arg( QString::number( slideCount() ) ) );
    baseLabel = tr("Duration: %1secs", "duration between images in a slide show");
    m_durationLabel->setText(
    baseLabel.arg( QString::number( slide( currentSlide() )->duration()/1000 ) ) );
}

int MMSComposer::currentSlide() const
{
    return m_curSlide;
}

uint MMSComposer::slideCount() const
{
    return m_slides.count();
}

MMSSlide *MMSComposer::slide( uint slide ) const
{
    if( slide >= slideCount() )
        return 0;
    return m_slides.at(slide);
}

void MMSComposer::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Back) {
        e->accept();
        emit finished();
        return;
    }

    QWidget::keyPressEvent(e);
}

void MMSComposer::elementChanged()
{
    QSoftMenuBar::setLabel(this, Qt::Key_Back, (isEmpty() ? QSoftMenuBar::Cancel : QSoftMenuBar::Next));

    emit contentChanged();
}

class SmilHandler : public QXmlDefaultHandler
{
public:
    QList<MMSSmilPart> parts;
    MMSSmil smil;
    SmilHandler() : m_insidePart( false ) {}

    bool startElement( const QString &, const QString &, const QString &qName,
                                                const QXmlAttributes & atts )
    {
        if( qName == "smil" )
        {
            smil.fgColor = QColor();
            smil.bgColor = QColor();
            smil.parts.clear();
        }
        else if( qName == "par" )
        {
            m_insidePart = true;
            MMSSmilPart newPart;
            if( atts.value( "duration" ).length() )
                newPart.duration = atts.value( "duration" ).toInt();
            smil.parts.append( newPart );
        }
        else if( qName == "region" )
        {
            if( atts.value("background-color").length() )
                smil.bgColor.setNamedColor( atts.value("background-color") );
        }
        else if( m_insidePart )
        {
            if( qName == "img" && atts.value( "src" ).length() )
                smil.parts.last().image = atts.value( "src" );

            else if( qName == "text" && atts.value( "src" ).length() )
                smil.parts.last().text = atts.value( "src" );

            else if( qName == "audio" && atts.value( "src" ).length() )
                smil.parts.last().audio = atts.value( "src" );
        }
        return true;
    }

    bool endElement( const QString &, const QString &, const QString &qName )
    {
        if( qName == "par" )
            m_insidePart = false;
        return true;
    }

private:
    bool m_insidePart;
};

MMSSmil MMSComposer::parseSmil( const QString &smil )
{
    QXmlInputSource input;
    input.setData( smil );
    QXmlSimpleReader reader;
    SmilHandler *handler = new SmilHandler;
    reader.setContentHandler( handler );
    if( !reader.parse( input ) )
        qWarning( "MMSComposer unable to parse smil message." );
    MMSSmil s = handler->smil;
    delete handler;
    return s;
}

// This logic is replicated in the SMIL viewer...
static QString smilStartMarker(const QMailMessage& mail)
{
    QMailMessageContentType type(mail.headerField("X-qtmail-internal-original-content-type"));
    if (type.isNull()) {
        type = QMailMessageContentType(mail.headerField("Content-Type"));
    }
    if (!type.isNull()) {
        QString startElement = type.parameter("start");
        if (!startElement.isEmpty())
            return startElement;
    }

    return QString("<presentation-part>");
}

static uint smilStartIndex(const QMailMessage& mail)
{
    QString startMarker(smilStartMarker(mail));

    for (uint i = 0; i < mail.partCount(); ++i)
        if (mail.partAt(i).contentID() == startMarker)
            return i;

    return 0;
}

static bool smilPartMatch(const QString identifier, const QMailMessagePart& part)
{
    // See if the identifer is a Content-ID marker
    QString id(identifier);
    if (id.toLower().startsWith("cid:"))
        id.remove(0, 4);

    return ((part.contentID() == id) || (part.displayName() == id) || (part.contentLocation() == id));
}

void MMSComposer::setMessage( const QMailMessage &mail )
{
    clear();
    MMSSlide *curSlide = slide( currentSlide() );

    if (mail.partCount() > 1) 
    {
        // This message must contain SMIL
        uint smilPartIndex = smilStartIndex(mail);
        MMSSmil smil = parseSmil( mail.partAt( smilPartIndex ).body().data() );

        // For each SMIL slide...
        int numSlides = 0;
        foreach( const MMSSmilPart& smilPart, smil.parts )
        {
            if( numSlides )
                addSlide();

            MMSSlide *curSlide = slide( slideCount() -1 );

            // ...for each part in the message...
            for( uint i = 0 ; i < mail.partCount(); ++i ) {
                if(i == smilPartIndex)
                    continue;

                const QMailMessagePart& part = mail.partAt( i );
                QString fileName(part.attachmentPath());

                // ...see if this part is used in this slide
                if (smilPartMatch(smilPart.text, part)) {
                    QString t = part.body().data();
                    curSlide->textContent()->setText( t );
                } else if (smilPartMatch(smilPart.image, part)) {
                    if (!fileName.isEmpty()) {
                        curSlide->imageContent()->setImage( QContent(fileName) );
                    } else {
                        QPixmap pix;
                        pix.loadFromData(part.body().data(QMailMessageBody::Decoded));
                        curSlide->imageContent()->setImage( pix );
                    }
                } else if (smilPartMatch(smilPart.audio, part)) {
                    if (!fileName.isEmpty()) {
                        curSlide->audioContent()->setAudio( QContent(fileName) );
                    } else {
                        QByteArray data = part.body().data(QMailMessageBody::Decoded);
                        curSlide->audioContent()->setAudio( data, part.displayName() );
                    }
                }
            }
            ++numSlides;
        }
        if( smil.bgColor.isValid() )
            setBackgroundColor( smil.bgColor );
        if( smil.fgColor.isValid() )
            setTextColor( smil.fgColor );
        setCurrentSlide( 0 );
    } else {
        QString bodyData(mail.body().data());
        if (!bodyData.isEmpty()) {
            curSlide->textContent()->setText(bodyData);
        }
        for (uint i = 0 ; i < mail.partCount() ; ++i) {
            QMailMessagePart part = mail.partAt( i );
            QMailMessageContentType contentType = part.contentType();
            QString fileName(part.attachmentPath());

            if( contentType.type().toLower() == "text" ) {
                QString t = part.body().data();
                curSlide->textContent()->setText( t );
            } else if( contentType.type().toLower() == "image" ) {
                if (!fileName.isEmpty()) {
                    curSlide->imageContent()->setImage( QContent(fileName) );
                } else {
                    QPixmap pix;
                    pix.loadFromData(part.body().data(QMailMessageBody::Decoded));
                    curSlide->imageContent()->setImage( pix );
                }
            } else if( contentType.type().toLower() == "audio" ) {
                if (!fileName.isEmpty()) {
                    curSlide->audioContent()->setAudio( QContent(fileName) );
                } else {
                    QByteArray data = part.body().data(QMailMessageBody::Decoded);
                    curSlide->audioContent()->setAudio( data, part.displayName() );
                }
            } else {
                qWarning() << "Unhandled MMS part:" << part.displayName();
            }
        }
    }

    m_removeSlide->setVisible(slideCount() > 1);
}

bool MMSComposer::isEmpty() const
{
    for( uint i = 0 ; i < slideCount() ; ++i )
        if( !slide( i )->imageContent()->isEmpty() ||
            !slide( i )->textContent()->isEmpty() ||
            !slide( i )->audioContent()->isEmpty()) {
            return false;
        }
    return true;
}

void MMSComposer::clear()
{
    while( slideCount() > 1 )
        removeSlide( slideCount() - 1 );
    if( slideCount() )
    {
        MMSSlide *cur = slide( currentSlide() );
        cur->imageContent()->setImage( QContent() );
        cur->textContent()->setText( QString() );
        cur->audioContent()->setAudio( QContent() );
    }
}

MMSComposerInterface::MMSComposerInterface( QWidget *parent )
    : QMailComposerInterface( parent )
{
    m_composer = new MMSComposer(parent);
    connect( m_composer, SIGNAL(contentChanged()), this, SIGNAL(contentChanged()) );
    connect( m_composer, SIGNAL(finished()), this, SIGNAL(finished()) );
}

MMSComposerInterface::~MMSComposerInterface()
{
    delete m_composer;
}

bool MMSComposerInterface::isEmpty() const
{
    return m_composer->isEmpty();
}

void MMSComposerInterface::setMessage( const QMailMessage &mail )
{
    m_composer->setMessage( mail );
}

// Inline parts: <<content-type, name>, data>
typedef QPair<QPair<QByteArray, QByteArray>, QByteArray> InlinePartDetails;
typedef QPair<QString, InlinePartDetails> PartDetails;

QMailMessage MMSComposerInterface::message() const
{
    QMailMessage mmsMail;

    QList<PartDetails> partDetails;
    InlinePartDetails nullPart(qMakePair(qMakePair(QByteArray(), QByteArray()), QByteArray()));

    //clean slate, generate document
    static const QString docTemplate =
    "<smil>\n"
    "   <head>\n"
    "       <meta name=\"title\" content=\"mms\"/>\n"
    "       <meta name=\"author\" content=\"%1\"/>\n"
    "       <layout>\n"
    "%2"
    "       </layout>\n"
    "   </head>\n"
    "   <body>\n"
    "%3"
    "   </body>\n"
    "</smil>\n"
    ; // 1.author 2.rootlayout&regions 3.parts
    static const QString rootLayoutTemplate =
    "           <root-layout width=\"%1\" height=\"%2\"/>\n"
    ; // 1.width 2.height 
    static const QString regionTemplate =
    "           <region id=\"%1\" width=\"%2\" height=\"%3\" left=\"%4\" top=\"%5\"%6/>\n"
    ; // 1.id 2.width 3.height 4.left 5.top 6.background-color-spec
    static const QString partTemplate =
    "      <par dur=\"%1\">\n"
    "%2"
    "      </par>\n"
    ; // 1.duration 2.contentitems
    static const QString imageTemplate =
    "         <img src=\"%1\" region=\"%2\"/>\n"
    ; // 1.src 2.region
    static const QString textTemplate =
    "         <text src=\"%1\" region=\"%2\"/>\n"
    ; // 1.src 2.region
    static const QString audioTemplate =
    "         <audio src=\"%1\"/>\n"
    ; // 1.src

    /*
       if the composer only contains one piece of content
       either an image or some text, then we don't need to
       generate smil output
    */
    int contentCount = 0;
    MMSSlideImage *imageContent = 0;
    MMSSlideText *textContent = 0;
    MMSSlideAudio *audioContent = 0;
    for( uint s = 0 ; s < m_composer->slideCount() ; ++s )
    {
        MMSSlide *curSlide = m_composer->slide( s );
        if( !curSlide->imageContent()->isEmpty() )
        {
            imageContent = curSlide->imageContent();
            ++contentCount;
        }
        if( !curSlide->textContent()->isEmpty() )
        {
            textContent = curSlide->textContent();
            ++contentCount;
        }
        if( !curSlide->audioContent()->isEmpty() )
        {
            audioContent = curSlide->audioContent();
            ++contentCount;
        }
        if( contentCount > 1 )
            break;
    }
    if( contentCount == 1 )
    {
        // Don't write a SMIL presentation - add the single part to the message
        if( textContent ) {
            // Add the text as the message body
            QMailMessageContentType type("text/plain; charset=UTF-8");
            mmsMail.setBody(QMailMessageBody::fromData(textContent->text(), type, QMailMessageBody::Base64));
        } else if( imageContent ) {
            if (imageContent->document().isValid()) {
                // Add the image as an attachment
                mmsMail.setMultipartType(QMailMessagePartContainer::MultipartMixed);
                partDetails.append(qMakePair(imageContent->document().fileName(), nullPart));
            } else {
                // Write the image out to a buffer in JPEG format
                QBuffer buffer;
                imageContent->image().save(&buffer, "JPEG");
                
                // Add the image data as the message body
                QMailMessageContentType type("image/jpeg");
                mmsMail.setBody(QMailMessageBody::fromData(buffer.data(), type, QMailMessageBody::Base64));
            }
        } else if (audioContent) {
            if (audioContent->document().isValid()) {
                // Add the audio as an attachment
                mmsMail.setMultipartType(QMailMessagePartContainer::MultipartMixed);
                partDetails.append(qMakePair(audioContent->document().fileName(), nullPart));
            } else {
                // Add the audio data as the message body
                QMailMessageContentType type( audioContent->mimeType().toLatin1() );
                mmsMail.setBody(QMailMessageBody::fromData(audioContent->audio(), type, QMailMessageBody::Base64));
            }
        }
    }
    else if( contentCount > 1 )
    {
        // Generate the full SMIL show
        QString parts;
        QRect largestText, largestImage;

        for( int s = 0 ; s < static_cast<int>(m_composer->slideCount()) ; ++s )
        {
            MMSSlide *curSlide = m_composer->slide( s );
            imageContent = curSlide->imageContent();
            textContent = curSlide->textContent();
            audioContent = curSlide->audioContent();

            QString part;
            if( !textContent->isEmpty() )
            {
                QRect cr = textContent->contentsRect();
                if( cr.height() > largestText.height() )
                    largestText.setHeight( cr.height() );

                QString textFileName = "mmstext" + QString::number( s ) + ".txt";
                part += textTemplate.arg( Qt::escape(textFileName) ).arg( "text" );

                // Write the text to a buffer in UTF-8 format
                QByteArray buffer;
                {
                    QTextStream out(&buffer);
                    out.setCodec("UTF-8");
                    out << textContent->text();
                }

                const QByteArray type("text/plain; charset=UTF-8");
                PartDetails details(qMakePair(QString(), qMakePair(qMakePair(type, textFileName.toLatin1()), buffer)));
                if (!partDetails.contains(details))
                    partDetails.append(details);
            }
            if( !imageContent->isEmpty() )
            {
                QRect cr = imageContent->contentsRect();
                if( cr.width() > largestImage.width() )
                    largestImage.setWidth( cr.width() );
                if( cr.height() > largestImage.height() )
                    largestImage.setHeight( cr.height() );

                QString imgFileName;
                if (imageContent->document().isValid()) {
                    imgFileName = imageContent->document().fileName();

                    PartDetails details(qMakePair(imgFileName, nullPart));
                    if (!partDetails.contains(details))
                        partDetails.append(details);

                    QFileInfo fi(imgFileName);
                    imgFileName = fi.fileName();
                } else {
                    imgFileName = "mmsimage" + QString::number( s ) + ".jpg";

                    // Write the data to a buffer in JPEG format
                    QBuffer buffer;
                    imageContent->image().save(&buffer, "JPEG");

                    const QByteArray type("image/jpeg");
                    PartDetails details(qMakePair(QString(), qMakePair(qMakePair(type, imgFileName.toLatin1()), buffer.data())));
                    if (!partDetails.contains(details))
                        partDetails.append(details);
                }

                part += imageTemplate.arg( Qt::escape(imgFileName) ).arg( "image" );
            }
            if( !audioContent->isEmpty() ) {
                QString audioFileName;
                if (audioContent->document().isValid()) {
                    audioFileName = audioContent->document().fileName();

                    PartDetails details(qMakePair(audioFileName, nullPart));
                    if (!partDetails.contains(details))
                        partDetails.append(details);

                    QFileInfo fi(audioFileName);
                    audioFileName = fi.fileName();
                } else {
                    QMimeType mimeType(audioContent->mimeType());
                    QString ext = mimeType.extension();
                    audioFileName = "mmsaudio" + QString::number( s ) + '.' + ext;

                    PartDetails details(qMakePair(QString(), qMakePair(qMakePair(mimeType.id().toLatin1(), audioFileName.toLatin1()), audioContent->audio())));
                    if (!partDetails.contains(details))
                        partDetails.append(details);
                }

                part += audioTemplate.arg( Qt::escape(audioFileName) );
            }
            if( !part.isEmpty() )
            {
                part = partTemplate.arg( QString::number( curSlide->duration() ) + "ms" ).arg( part );
                parts += part;
            }
        }

        QRect imageRect;
        imageRect.setX( 0 );
        imageRect.setY( 0 );
        imageRect.setWidth( m_composer->contentsRect().width() );
        imageRect.setHeight( m_composer->contentsRect().height() - largestText.height() );

        largestText.setX( 0 );
        int h = largestText.height();
        largestText.setY( imageRect.height() );
        largestText.setHeight( h );
        largestText.setWidth(m_composer->contentsRect().width());

        QString regions;
        QString backgroundParam;
        if( m_composer->backgroundColor().isValid() )
        {
            backgroundParam = QString(" background-color=\"%1\"").arg(m_composer->backgroundColor().name().toUpper());
        }
        if( imageRect.width() > 0 && imageRect.height() > 0 )
        {
            regions += regionTemplate.arg( "image" )
                                     .arg( imageRect.width() )
                                     .arg( imageRect.height() )
                                     .arg( imageRect.x() )
                                     .arg( imageRect.y() )
                                     .arg( backgroundParam );
        }
        if( largestText.width() > 0 && largestText.height() > 0 )
        {
            regions += regionTemplate.arg( "text" )
                                     .arg( largestText.width() )
                                     .arg( largestText.height() )
                                     .arg( largestText.x() )
                                     .arg( largestText.y() )
                                     .arg( backgroundParam );
        }

        QString rootLayout = rootLayoutTemplate.arg( m_composer->contentsRect().width() ).arg( m_composer->contentsRect().height() );
        QString doc = docTemplate.arg( Qtopia::ownerName() ) .arg( rootLayout + regions ) .arg( parts );

        // add the smil document to the message
        mmsMail.setMultipartType( QMailMessage::MultipartRelated );

        QMailMessagePart docPart;
        docPart.setContentID( "<presentation-part>" );

        QMailMessageContentType type("application/smil");
        docPart.setBody( QMailMessageBody::fromData(doc.toLatin1(), type, QMailMessageBody::EightBit, QMailMessageBody::AlreadyEncoded));

        mmsMail.appendPart( docPart );
    }

    mmsMail.setMessageType( QMailMessage::Mms );

    // add binary data as mail attachments
    foreach (const PartDetails& details, partDetails) {
        QMailMessagePart part;
        QByteArray identifier;

        if (!details.first.isNull()) {
            // Ensure that the nominated file is accessible
            QFileInfo fi( details.first );
            QString path = fi.absoluteFilePath();

            {
                QFile f(path);
                if( !f.open( QIODevice::ReadOnly ) ) {
                    qWarning() << "Could not open MMS attachment for reading" << path;
                    continue;
                }
            }

            // Add this part from the file
            identifier = fi.fileName().toLatin1();

            QMailMessageContentDisposition disposition(QMailMessageContentDisposition::Attachment);
            disposition.setFilename(identifier);

            QMailMessageContentType type( QMimeType(path).id().toLatin1() );
            type.setName(identifier);

            part = QMailMessagePart(QMailMessagePart::fromFile(path, disposition, type, QMailMessageBody::Base64, QMailMessageBody::RequiresEncoding));
        } else {
            // Add the part from the data
            const InlinePartDetails& inlineDetails(details.second);
            identifier = inlineDetails.first.second;

            QMailMessageContentDisposition disposition(QMailMessageContentDisposition::Attachment);
            disposition.setFilename(identifier);

            QMailMessageContentType type(inlineDetails.first.first);
            type.setName(identifier);

            part = QMailMessagePart(QMailMessagePart::fromData(inlineDetails.second, disposition, type, QMailMessageBody::Base64, QMailMessageBody::RequiresEncoding));
        }

        // Set the Content-ID and Content-Location fields to prevent them being synthesized by MMSC
        part.setContentID(identifier);
        part.setContentLocation(identifier);

        mmsMail.appendPart(part);
    }

    return mmsMail;
}

void MMSComposerInterface::attach( const QContent &lnk, QMailMessage::AttachmentsAction action )
{
    if (action != QMailMessage::LinkToAttachments) {
        // TODO: handle temporary files
        qWarning() << "MMS: Unable to handle attachment of transient document!";
        return;
    }

    if (!m_composer->slideCount())
        m_composer->addSlide();

    MMSSlide *curSlide = m_composer->slide( m_composer->slideCount()-1 );

    if (lnk.type().startsWith("image/")) {
        if (lnk.isValid()) {
            if (!curSlide->imageContent()->isEmpty()) {
                // If there is already an image in the last slide, add a new one
                m_composer->addSlide();
                curSlide = m_composer->slide( m_composer->slideCount()-1 );
            }
            curSlide->imageContent()->setImage(lnk);
        }
    } else if (lnk.type() == "text/plain") {
        QFile file(lnk.fileName());
        if (file.open(QIODevice::ReadOnly)) {
            QTextStream ts(&file);
            if (!curSlide->textContent()->isEmpty()) {
                // If there is already text in the last slide, add a new one
                m_composer->addSlide();
                curSlide = m_composer->slide( m_composer->slideCount()-1 );
            }
            curSlide->textContent()->setText(ts.readAll());
        }
    } else if (lnk.type().startsWith("audio/")) {
        if (!curSlide->audioContent()->isEmpty()) {
            // If there is already audio in the last slide, add a new one
            m_composer->addSlide();
            curSlide = m_composer->slide( m_composer->slideCount()-1 );
        }
        curSlide->audioContent()->setAudio(lnk);
    } else {
        // TODO: deal with other attachments
    }
}

void MMSComposerInterface::clear()
{
    m_composer->clear();
}

QWidget *MMSComposerInterface::widget() const
{
    return m_composer;
}

void MMSComposerInterface::addActions(QMenu* menu) const
{
    m_composer->addActions(menu);
}

QTOPIA_EXPORT_PLUGIN( MMSComposerPlugin )

MMSComposerPlugin::MMSComposerPlugin()
    : QMailComposerPlugin()
{
}

QString MMSComposerPlugin::key() const
{
    return "MMSComposer";
}

QMailMessage::MessageType MMSComposerPlugin::messageType() const
{
    return QMailMessage::Mms;
}

QString MMSComposerPlugin::name() const
{
    return tr("Multimedia message");
}

QString MMSComposerPlugin::displayName() const
{
    return tr("MMS");
}

QIcon MMSComposerPlugin::displayIcon() const
{
    static QIcon icon(":icon/multimedia");
    return icon;
}

QMailComposerInterface* MMSComposerPlugin::create( QWidget *parent )
{
    return new MMSComposerInterface( parent );
}

