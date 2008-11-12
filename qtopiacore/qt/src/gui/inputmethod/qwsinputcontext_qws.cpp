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

#include "qwsinputcontext_p.h"
#include "qwsdisplay_qws.h"
#include "qwsevent_qws.h"
#include "private/qwscommand_qws_p.h"
#include "qwindowsystem_qws.h"
#include "qevent.h"
#include "qtextformat.h"

#include <qbuffer.h>

#include <qdebug.h>

#ifndef QT_NO_QWS_INPUTMETHODS

static QWidget* activeWidget = 0;

//#define EXTRA_DEBUG

QWSInputContext::QWSInputContext(QObject *parent)
    :QInputContext(parent)
{
}

void QWSInputContext::reset()
{
    QPaintDevice::qwsDisplay()->resetIM();
}


void QWSInputContext::setFocusWidget( QWidget *w )
{
  QWidget *oldFocus = focusWidget();
    if (oldFocus == w)
        return;

    if (oldFocus) {
        QWidget *tlw = oldFocus->window();
        int winid = tlw->internalWinId();

        int widgetid = oldFocus->internalWinId();
        QPaintDevice::qwsDisplay()->sendIMUpdate(QWSInputMethod::FocusOut, winid, widgetid);
    }

    QInputContext::setFocusWidget(w);

    if (!w)
        return;

    QWidget *tlw = w->window();
    int winid = tlw->winId();

    int widgetid = w->winId();
    QPaintDevice::qwsDisplay()->sendIMUpdate(QWSInputMethod::FocusIn, winid, widgetid);

    //setfocus ???

    update();
}


void QWSInputContext::widgetDestroyed(QWidget *w)
{
    if (w == ::activeWidget)
        ::activeWidget = 0;
    QInputContext::widgetDestroyed(w);
}

void QWSInputContext::update()
{
    QWidget *w = focusWidget();
    if (!w)
        return;

    QWidget *tlw = w->window();
    int winid = tlw->winId();

    int widgetid = w->winId();
    QPaintDevice::qwsDisplay()->sendIMUpdate(QWSInputMethod::Update, winid, widgetid);

}

void QWSInputContext::mouseHandler( int x, QMouseEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress || event->type() == QEvent::MouseButtonRelease)
        QPaintDevice::qwsDisplay()->sendIMMouseEvent( x, event->type() == QEvent::MouseButtonPress );
}

QWidget *QWSInputContext::activeWidget()
{
    return ::activeWidget;
}


bool QWSInputContext::isComposing() const
{
    return ::activeWidget != 0;
}

bool QWSInputContext::translateIMQueryEvent(QWidget *w, const QWSIMQueryEvent *e)
{
    Qt::InputMethodQuery type = static_cast<Qt::InputMethodQuery>(e->simpleData.property);
    QVariant result = w->inputMethodQuery(type);
    QWidget *tlw = w->window();
    int winId = tlw->winId();

    if ( type == Qt::ImMicroFocus ) {
        // translate to relative to tlw
        QRect mf = result.toRect();
        mf.moveTopLeft(w->mapTo(tlw,mf.topLeft()));
        result = mf;
    }

    QPaintDevice::qwsDisplay()->sendIMResponse(winId, e->simpleData.property, result);

    return false;
}

bool QWSInputContext::translateIMInitEvent(const QWSIMInitEvent *e)
{
    Q_UNUSED(e);
    qDebug("### QWSInputContext::translateIMInitEvent not implemented ###");
    return false;
}

bool QWSInputContext::translateIMEvent(QWidget *w, const QWSIMEvent *e)
{
    QDataStream stream(e->streamingData);
    QString preedit;
    QString commit;

    stream >> preedit;
    stream >> commit;

    if (preedit.isEmpty() && ::activeWidget)
        w = ::activeWidget;

    QInputContext *qic = w->inputContext();
    if (!qic)
        return false;

    QList<QInputMethodEvent::Attribute> attrs;


    while (!stream.atEnd()) {
        int type = -1;
        int start = -1;
        int length = -1;
        QVariant data;
        stream >> type >> start >> length >> data;
        if (stream.status() != QDataStream::Ok) {
            qWarning("corrupted QWSIMEvent");
            //qic->reset(); //???
            return false;
        }
        if (type == QInputMethodEvent::TextFormat)
            data = qic->standardFormat(static_cast<QInputContext::StandardFormat>(data.toInt()));
        attrs << QInputMethodEvent::Attribute(static_cast<QInputMethodEvent::AttributeType>(type), start, length, data);
    }
#ifdef EXTRA_DEBUG
    qDebug() << "preedit" << preedit << "len" << preedit.length() <<"commit" << commit << "len" << commit.length()
             << "n attr" << attrs.count();
#endif


    QInputMethodEvent ime(preedit, attrs);
    if (!commit.isEmpty() || e->simpleData.replaceLength > 0)
        ime.setCommitString(commit, e->simpleData.replaceFrom, e->simpleData.replaceLength);


    extern bool qt_sendSpontaneousEvent(QObject *, QEvent *); //qapplication_qws.cpp
    qt_sendSpontaneousEvent(w, &ime);

    if (preedit.isEmpty())
        ::activeWidget = 0;
    else
        ::activeWidget = w;

    return true;
}





#endif // QT_NO_QWS_INPUTMETHODS
