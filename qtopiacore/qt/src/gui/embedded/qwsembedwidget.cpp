/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qwsembedwidget.h"

#ifndef QT_NO_QWSEMBEDWIDGET

#include <qwsdisplay_qws.h>
#include <private/qwidget_p.h>
#include <private/qwsdisplay_qws_p.h>
#include <private/qwscommand_qws_p.h>

// TODO:
// Must remove window decorations from the embedded window
// Focus In/Out, Keyboard/Mouse...
//
// BUG: what if my parent change parent?

class QWSEmbedWidgetPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QWSEmbedWidget);

public:
    QWSEmbedWidgetPrivate(int winId);
    void updateWindow();
    void resize(const QSize &size);

    QWidget *window;
    WId windowId;
    WId embeddedId;
};

QWSEmbedWidgetPrivate::QWSEmbedWidgetPrivate(int winId)
    : window(0), windowId(0), embeddedId(winId)
{
}

void QWSEmbedWidgetPrivate::updateWindow()
{
    Q_Q(QWSEmbedWidget);

    QWidget *win = q->window();
    if (win == window)
        return;

    if (window) {
        window->removeEventFilter(q);
        QWSEmbedCommand command;
        command.setData(windowId, embeddedId, QWSEmbedEvent::StopEmbed);
        QWSDisplay::instance()->d->sendCommand(command);
    }

    window = win;
    if (!window)
        return;
    windowId = window->winId();

    QWSEmbedCommand command;
    command.setData(windowId, embeddedId, QWSEmbedEvent::StartEmbed);
    QWSDisplay::instance()->d->sendCommand(command);
    window->installEventFilter(q);
    q->installEventFilter(q);
}

void QWSEmbedWidgetPrivate::resize(const QSize &size)
{
    if (!window)
        return;

    Q_Q(QWSEmbedWidget);

    QWSEmbedCommand command;
    command.setData(windowId, embeddedId, QWSEmbedEvent::Region,
                    QRect(q->mapToGlobal(QPoint(0, 0)), size));
    QWSDisplay::instance()->d->sendCommand(command);
}

/*!
    \class QWSEmbedWidget
    \since 4.2
    \ingroup qws

    \brief The QWSEmbedWidget class enabels embedded top-level widgets
    in Qtopia Core.

    Note that this class is only available in \l {Qtopia Core}.

    QWSEmbedWidget inherits QWidget and acts as any other widget, but
    in addition it is capable of embedding another top-level widget.

    An example of use is when painting directly onto the screen using
    the QDirectPainter class. Then the reserved region can be embedded
    into an instance of the QWSEmbedWidget class, providing for
    example event handling and size policies for the reserved region.

    All that is required to embed a top-level widget is its window ID.

    \sa {Qtopia Core Architecture}
*/

/*!
    Constructs a widget with the given \a parent, embedding the widget
    identified by the given window \a id.
*/
QWSEmbedWidget::QWSEmbedWidget(WId id, QWidget *parent)
    : QWidget(*new QWSEmbedWidgetPrivate(id), parent, 0)
{
    Q_D(QWSEmbedWidget);
    d->updateWindow();
}

/*!
    Destroys this widget.
*/
QWSEmbedWidget::~QWSEmbedWidget()
{
    Q_D(QWSEmbedWidget);
    if (!d->window)
        return;

    QWSEmbedCommand command;
    command.setData(d->windowId, d->embeddedId, QWSEmbedEvent::StopEmbed);
    QWSDisplay::instance()->d->sendCommand(command);
}

/*!
    \reimp
*/
bool QWSEmbedWidget::eventFilter(QObject *object, QEvent *event)
{
    Q_D(QWSEmbedWidget);
    if (object == d->window && event->type() == QEvent::Move)
        resizeEvent(0);
    else if (object == this && event->type() == QEvent::Hide)
        d->resize(QSize());
    return QWidget::eventFilter(object, event);
}

/*!
    \reimp
*/
void QWSEmbedWidget::changeEvent(QEvent *event)
{
    Q_D(QWSEmbedWidget);
    if (event->type() == QEvent::ParentChange)
        d->updateWindow();
}

/*!
    \reimp
*/
void QWSEmbedWidget::resizeEvent(QResizeEvent*)
{
    Q_D(QWSEmbedWidget);
    d->resize(rect().size());
}

/*!
    \reimp
*/
void QWSEmbedWidget::moveEvent(QMoveEvent*)
{
    resizeEvent(0);
}

/*!
    \reimp
*/
void QWSEmbedWidget::hideEvent(QHideEvent*)
{
    Q_D(QWSEmbedWidget);
    d->resize(QSize());
}

/*!
    \reimp
*/
void QWSEmbedWidget::showEvent(QShowEvent*)
{
    Q_D(QWSEmbedWidget);
    d->resize(rect().size());
}

#endif // QT_NO_QWSEMBEDWIDGET
