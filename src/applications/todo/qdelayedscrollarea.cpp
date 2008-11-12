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

#include "qdelayedscrollarea.h"
#include <QEvent>
#include <QDebug>
#include <QLayout>

QDelayedScrollArea::QDelayedScrollArea(int index, QWidget *parent) : QScrollArea(parent), i(index)
{
    setWidgetResizable(true);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFocusPolicy(Qt::NoFocus);
    setFrameStyle(QFrame::NoFrame);
    viewport()->installEventFilter(this);
}

QDelayedScrollArea::QDelayedScrollArea(QWidget *parent) : QScrollArea(parent), i(-1) {}

QDelayedScrollArea::~QDelayedScrollArea() {}

void QDelayedScrollArea::adjustWidget(int width)
{
    QWidget *w = widget();
    if (w) {
        w->setFixedWidth(width);
        QLayout *l = w->layout();
        if (l) {
            if (l->hasHeightForWidth())
                w->setMinimumHeight(l->heightForWidth(width));
        } else {
            if (w->sizePolicy().hasHeightForWidth())
                w->setMinimumHeight(w->heightForWidth(width));
        }
    }
}

bool QDelayedScrollArea::eventFilter( QObject *receiver, QEvent *event )
{
    if (widget() && receiver == viewport() && event->type() == QEvent::Resize )
        adjustWidget(viewport()->width());
    if (widget() == receiver && event->type() == QEvent::LayoutRequest)
        adjustWidget(viewport()->width());
    return false;
}

void QDelayedScrollArea::showEvent(QShowEvent *event)
{
    emit aboutToShow(i);
    adjustWidget(viewport()->width());

    QScrollArea::showEvent(event);
}

void QDelayedScrollArea::resizeEvent(QResizeEvent *re)
{
    adjustWidget(viewport()->width());
    QScrollArea::resizeEvent(re);
}

