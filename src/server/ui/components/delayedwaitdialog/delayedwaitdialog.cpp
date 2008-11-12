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

#include "delayedwaitdialog.h"

#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QApplication>
#include <QDesktopWidget>
#include <QSoftMenuBar>
#include <QStyle>
#include <QKeyEvent>

class Icon : public QLabel
{
public:
    Icon( QWidget *parent = 0, Qt::WFlags fl = 0 )
        : QLabel( parent, fl ), mOpacity( 0.0 )
    {
        setPixmap( QPixmap( ":icon/play" ) );
    }
    ~Icon()
    {
    }

    void setOpacity( qreal opacity )
    {
        mOpacity = opacity;
    }

    qreal opacity()
    {
        return mOpacity;
    }

protected:
    virtual void paintEvent( QPaintEvent *pe )
    {
        Q_UNUSED( pe );
        QPainter painter( this );
        painter.setOpacity( mOpacity );
        painter.drawPixmap( 0, 0, *pixmap() );
    }
private:
    qreal mOpacity;
};

DelayedWaitDialog::DelayedWaitDialog( QWidget *parent, Qt::WFlags fl )
    : QDialog( parent, fl | Qt::FramelessWindowHint ), mDelay(0), mTid(0)
{
    QVBoxLayout *vl = new QVBoxLayout( this );
    vl->setMargin( 0 );

    QHBoxLayout *hl = new QHBoxLayout;
    text = new QLabel( this );
    hl->addStretch();
    hl->addWidget( text );
    hl->addStretch();
    vl->addLayout( hl );

    hl = new QHBoxLayout;
    hl->setMargin( 0 );
    hl->addStretch();
    Icon *icon = 0;
    for ( int i = 0; i < NUMBEROFICON; ++i ) {
        icon = new Icon( this );
        hl->addWidget( icon );
        mIconList.append( icon );
    }
    hl->addStretch();
    vl->addLayout( hl, 1 );

    QRect d = QApplication::desktop()->screenGeometry();
    int dw = d.width();
    int dh = d.height();
    setGeometry(0, 70*dh/100, dw, QApplication::style()->pixelMetric(QStyle::PM_SmallIconSize));

    mTimer = new QTimer( this );
    // two seconds to complete each round
    mTimer->setInterval( 2000 / NUMBEROFICON );
    connect( mTimer, SIGNAL(timeout()), this, SLOT(update()) );
    QSoftMenuBar::setLabel( this, QSoftMenuBar::Back, QSoftMenuBar::NoLabel );
}

DelayedWaitDialog::~DelayedWaitDialog()
{
}

void DelayedWaitDialog::setText( const QString &str )
{
    text->setText( str );
}

/*!
    The dialog will appear after \a ms milliseconds delay.
*/
void DelayedWaitDialog::setDelay( int ms )
{
    mDelay = ms;
}

void DelayedWaitDialog::show()
{
    if ( mDelay )
        mTid = startTimer( mDelay );
    else
        QDialog::show();
}

void DelayedWaitDialog::hide()
{
    if ( mTid )
        killTimer( mTid );
    mTid = 0;
    QDialog::hide();
}

void DelayedWaitDialog::showEvent( QShowEvent *se )
{
    QDialog::showEvent( se );
    mTimer->start();
}

void DelayedWaitDialog::hideEvent( QHideEvent *he )
{
    QDialog::hideEvent( he );
    mTimer->stop();
}

void DelayedWaitDialog::keyReleaseEvent( QKeyEvent *ke )
{
    if ( ke->key() == Qt::Key_Hangup
            || ke->key() == Qt::Key_Call )
        QDialog::keyReleaseEvent( ke );
    else
        ke->ignore();
}

void DelayedWaitDialog::timerEvent( QTimerEvent *te )
{
    // display wait widget, new command might take time to arrive
    if ( te->timerId() == mTid ) {
        QDialog::show();
        killTimer( mTid );
        mTid = 0;
    }
}

void DelayedWaitDialog::update()
{
    static int highlightIndex = 0;
    int stepsToHighlightIndex = 0;
    for( int i = 0; i < NUMBEROFICON; ++i ) {
        stepsToHighlightIndex = i < highlightIndex ?
            highlightIndex - i : NUMBEROFICON - i + highlightIndex;
        mIconList[i]->setOpacity( 1.0 - ( 1.0 / NUMBEROFICON * stepsToHighlightIndex ) );
        mIconList[i]->repaint();
    }
    highlightIndex++;
    if ( highlightIndex == NUMBEROFICON )
        highlightIndex = 0;
}

