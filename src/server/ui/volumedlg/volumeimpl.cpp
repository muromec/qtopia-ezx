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

#include "volumeimpl.h"
#include <QKeyEvent>
#include <QWidget>
#include <QApplication>
#include <QDesktopWidget>
#include <QPainter>
#include <QVBoxLayout>
#include <QValueSpaceItem>

#include <math.h>

class VolumeWidget : public QWidget
{

public:
    VolumeWidget(QWidget *parent = 0, Qt::WindowFlags flags = 0)
        : QWidget(parent, flags), m_steps(10), m_current(0)
    {
    }

    ~VolumeWidget() {}

    int heightForWidth(int width) const
    {
        return qRound(0.33 * width);
    }

    int value() const
    {
        return m_current;
    }

    int steps() const
    {
        return m_steps;
    }

    void setCurrent(int i)
    {
        if ((i <= 0) || (i > m_steps))
            return;

        m_current = i;
        update();
    }

    QLabel *l;
    QLabel *m;

protected:
    void paintEvent(QPaintEvent * /*event*/)
    {
        int w = rect().width() - (3 * m_steps);
        barWidth = qRound(w / (m_steps - 1));
        barHeight = qRound(rect().height() / (m_steps - 1));

        QPainter painter(this);
        painter.setPen(palette().text().color());
        painter.setBrush(palette().highlight());

        for (int n = 1; n < m_current; n++) {
            QRect r;
            r.setTopLeft(QPoint(((n-1) * barWidth) + (n * 3) - 1, (m_steps-1-n)*barHeight));
            r.setWidth(barWidth);
            r.setHeight(n * barHeight);
            painter.drawRect(r);
        }
    }

    QSizePolicy sizePolicy() const
    {
        return QSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);
    }

    QSize sizeHint() const
    {
        return QSize(27, 9);
    }

private:
    int m_steps;
    int m_current;
    int barWidth;
    int barHeight;
};

class VolumeDialogImplPrivate
{
public:
    VolumeDialogImplPrivate();
    ~VolumeDialogImplPrivate();

    QValueSpaceItem* m_vsVolume;
};

VolumeDialogImplPrivate::VolumeDialogImplPrivate()
{
    m_vsVolume  = new QValueSpaceItem("Volume/CurrentVolume");
}

VolumeDialogImplPrivate::~VolumeDialogImplPrivate()
{
    delete m_vsVolume;
}


VolumeDialogImpl::VolumeDialogImpl( QWidget* parent, Qt::WFlags fl )
    : QDialog( parent, fl ), m_tid(0), m_oldValue(0), m_d(new VolumeDialogImplPrivate)
{
    QRect d = QApplication::desktop()->screenGeometry();
    int dw = d.width();
    int dh = d.height();
    setGeometry(20*dw/100, 30*dh/100, 60*dw/100, 40*dh/100);

    QColor c(Qt::black);
    c.setAlpha(255);     //XXX: Make fully opaque for now, for  DirectPainter widgets in the background

    setAttribute(Qt::WA_SetPalette, true);

    QPalette p = palette();
    p.setBrush(QPalette::Window, c);
    setPalette(p);

    QVBoxLayout *vBox = new QVBoxLayout(this);
    QHBoxLayout *hBox = new QHBoxLayout;

    volumeWidget = new VolumeWidget(this);

    QIcon icon(":icon/sound");
    QIcon mute(":icon/mute");

    QLabel* volumeLevel = new QLabel(this);
    volumeLevel->setAlignment(Qt::AlignRight | Qt::AlignCenter);
    volumeLevel->setMinimumWidth( fontMetrics().width("100%") );
    volumeLevel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Maximum);
    connect(this, SIGNAL(setText(QString)), volumeLevel, SLOT(setText(QString)));

    volumeWidget->l = new QLabel(this);
    volumeWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    volumeWidget->l->setPixmap(icon.pixmap(64, 64));
    volumeWidget->m = new QLabel(this);

    volumeWidget->m->hide();
    volumeWidget->m->setPixmap(mute.pixmap(64, 64));

    hBox->addStretch();
    hBox->addWidget(volumeWidget->l);
    hBox->addWidget(volumeWidget->m);
    hBox->addStretch();

    QHBoxLayout *wp = new QHBoxLayout;
    wp->addWidget(volumeWidget);
    wp->addWidget(volumeLevel);

    vBox->addLayout(hBox);
    vBox->addLayout(wp);

    connect(m_d->m_vsVolume, SIGNAL(contentsChanged()), this, SLOT(valueSpaceVolumeChanged()));
}

void VolumeDialogImpl::timerEvent( QTimerEvent *e )
{
    Q_UNUSED(e)
    close();
}

void VolumeDialogImpl::setVolume( bool up)
{
    m_oldValue = volumeWidget->value();
    volumeWidget->setCurrent( up ? m_oldValue + 1 : m_oldValue - 1 );
    int value = volumeWidget->value();

    if ( m_oldValue < value )
        emit volumeChanged( true );
    else if ( m_oldValue > value )
        emit volumeChanged( false );

    resetTimer();
}

void VolumeDialogImpl::resetTimer()
{
    killTimer( m_tid );
    m_tid = startTimer( TIMEOUT );
}

void VolumeDialogImpl::valueSpaceVolumeChanged()
{
    int volume = m_d->m_vsVolume->value().toInt();
    int slot =   qBound(1,(int)::ceil(volume/(volumeWidget->steps()))+1,10);
    volumeWidget->setCurrent( slot );

    QString str;
    str.setNum(volume);
    emit setText(str+"%");

    if(volume > 0)
    {
        volumeWidget->m->hide();
        volumeWidget->l->show();
    }
    else
    {
        volumeWidget->l->hide();
        volumeWidget->m->show();
        volumeWidget->setCurrent( 1 );
    }
    this->show();
    resetTimer();
}
