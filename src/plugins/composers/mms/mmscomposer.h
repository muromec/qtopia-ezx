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

#ifndef MMSCOMPOSER_H
#define MMSCOMPOSER_H

#include <qcontent.h>
#include <qwidget.h>
#include <qlist.h>
#include <qlabel.h>
#include <qtextedit.h>
#include <qpixmap.h>
#include <qpushbutton.h>

#include <qtopia/mail/qmailcomposer.h>
#include <qtopia/mail/qmailcomposerplugin.h>

class QAction;
class QMenu;
class QStackedWidget;
class QEvent;
class QKeyEvent;
class QPaintEvent;
class QShowEvent;
class QMouseEvent;

class MMSComposer;

class MMSSlideImage : public QLabel
{
    Q_OBJECT
public:
    MMSSlideImage(QWidget *parent);

    QRect contentsRect() const;
    QSize sizeHint() const;

    void setImage( const QContent &image );
    void setImage( const QPixmap &image );

    QPixmap image() const;

    bool isEmpty() const;

    QContent& document();

signals:
    void clicked();
    void changed();
    void leftPressed(); // for keypad mode only
    void rightPressed(); // ""

protected slots:
    void select();

protected:
    void resizeEvent( QResizeEvent *event );
    void mousePressEvent( QMouseEvent *event );
    void mouseReleaseEvent( QMouseEvent *event );
    void keyPressEvent( QKeyEvent *event );
    void paintEvent( QPaintEvent *event );
    void showEvent( QShowEvent *event );

    QPixmap loadImage() const;

    QPixmap scale( const QPixmap &src ) const;

private:
    QPixmap m_image;
    QContent m_content;
    QSize m_contentSize;
    bool m_pressed;
};

class MMSSlideText : public QTextEdit
{
    Q_OBJECT
public:
    MMSSlideText(QWidget *parent);

    void setText( const QString &txt );
    QString text() const;

    bool isEmpty() const;

    QSize sizeHint() const;

    const QString defaultText;

    QRect contentsRect() const;

signals:
    void leftPressed(); // for keypad mode only
    void rightPressed(); // ""

protected:
    virtual void mousePressEvent ( QMouseEvent * e );
    void keyPressEvent( QKeyEvent *e );
    bool event( QEvent *e );

private:
    bool m_hasFocus;
};

class MMSSlideAudio : public QPushButton
{
    Q_OBJECT
public:
    MMSSlideAudio(QWidget *parent);

    void setAudio( const QContent &fn );
    void setAudio( const QByteArray &, const QString & );
    QByteArray audio() const;
    QString mimeType() const;

    bool isEmpty() const;

    QContent& document();

private slots:
    void select();

protected:
    void keyPressEvent( QKeyEvent *e );

signals:
    void leftPressed(); // for keypad mode only
    void rightPressed(); // ""
    void changed();

private:
    QContent audioContent;
    mutable QByteArray audioData;
    QString audioName;
    mutable QString audioType;
};

struct MMSSmilPart
{
    MMSSmilPart() : duration( 5000 ) {}
    int duration;
    QString image;
    QString text;
    QString audio;
};

struct MMSSmil
{
    QColor bgColor;
    QColor fgColor;
    QList<MMSSmilPart> parts;
};


class MMSSlide : public QWidget
{
    friend class MMSComposer; // crap, composer needs to install context menus on widgets

    Q_OBJECT
public:
    MMSSlide(QWidget *parent = 0);

    MMSSlideImage *imageContent() const;
    MMSSlideText *textContent() const;
    MMSSlideAudio *audioContent() const;

    void setDuration( int t );
    int duration() const;

signals:
    void leftPressed();
    void rightPressed();
    void durationChanged(int);

private:
    MMSSlideImage *m_imageContent;
    MMSSlideText *m_textContent;
    MMSSlideAudio *m_audioContent;
    int m_duration;
};

class MMSComposer : public QWidget
{
    Q_OBJECT
public:
    MMSComposer(QWidget *parent = 0);
    ~MMSComposer();

    void addActions(QMenu* menu);

    QRect contentsRect() const;

    bool isEmpty() const;

    uint slideCount() const;
    MMSSlide *slide( uint slide ) const;
    int currentSlide() const;

public slots:
    void setMessage( const QMailMessage &mail );
    void clear();
    void addSlide();
    void removeSlide();
    void addSlide( int a_slide );
    void removeSlide( int a_slide );
    void nextSlide();
    void previousSlide();
    void setCurrentSlide( int slide );
    void updateLabels();
    void setTextColor( const QColor &col );
    void setBackgroundColor( const QColor &col );
    QColor textColor() const;
    QColor backgroundColor() const;

protected slots:
    void slideOptions();
    void elementChanged();

protected:
    void keyPressEvent(QKeyEvent *);

private:
    MMSSmil parseSmil( const QString &doc );

signals:
    void currentChanged( uint );
    void contentChanged();
    void finished();

private:
    QLabel *m_slideLabel, *m_durationLabel;
    QStackedWidget *m_slideStack;
    int m_curSlide;
    QList<MMSSlide*> m_slides;
    bool m_internalUpdate;
    QColor m_textColor, m_backgroundColor;
    QAction* m_removeSlide;
};

class MMSComposerInterface : public QMailComposerInterface
{
    Q_OBJECT

public:
    MMSComposerInterface(QWidget *parent = 0);
    ~MMSComposerInterface();

    bool isEmpty() const;
    QMailMessage message() const;

    QWidget *widget() const;

    virtual void addActions(QMenu* menu) const;

public slots:
    void setMessage( const QMailMessage &mail );
    void clear();
    virtual void attach( const QContent &lnk, QMailMessage::AttachmentsAction action = QMailMessage::LinkToAttachments );

private:
    MMSComposer *m_composer;
};

class MMSComposerPlugin : public QMailComposerPlugin
{
    Q_OBJECT

public:
    MMSComposerPlugin();

    virtual QString key() const;
    virtual QMailMessage::MessageType messageType() const;

    virtual QString name() const;
    virtual QString displayName() const;
    virtual QIcon displayIcon() const;

    QMailComposerInterface* create( QWidget* parent );
};

#endif
