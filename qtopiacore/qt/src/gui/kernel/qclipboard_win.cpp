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

#include "qclipboard.h"

#ifndef QT_NO_CLIPBOARD

#include "qapplication.h"
#include "qapplication_p.h"
#include "qeventloop.h"
#include "qwidget.h"
#include "qevent.h"
#include "qmime.h"
#include "qt_windows.h"
#include "qdnd_p.h"


class QClipboardWatcher : public QInternalMimeData {
public:
    QClipboardWatcher()
        : QInternalMimeData()
    {
    }

    bool hasFormat_sys(const QString &mimetype) const;
    QStringList formats_sys() const;
    QVariant retrieveData_sys(const QString &mimetype, QVariant::Type preferredType) const;
};


bool QClipboardWatcher::hasFormat_sys(const QString &mime) const
{
    IDataObject * pDataObj = 0;

    if (OleGetClipboard(&pDataObj) != S_OK && !pDataObj) // Sanity
        return false;

    bool has = QWindowsMime::converterToMime(mime, pDataObj) != 0;

    pDataObj->Release();

    return has;
}

QStringList QClipboardWatcher::formats_sys() const
{
    QStringList fmts;
    IDataObject * pDataObj = 0;

    if (OleGetClipboard(&pDataObj) != S_OK && !pDataObj) // Sanity
        return QStringList();

    fmts = QWindowsMime::allMimesForFormats(pDataObj);

    pDataObj->Release();

    return fmts;
}

QVariant QClipboardWatcher::retrieveData_sys(const QString &mimeType, QVariant::Type type) const
{
    QVariant result;
    IDataObject * pDataObj = 0;

    if (OleGetClipboard(&pDataObj) != S_OK && !pDataObj) // Sanity
        return result;

    QWindowsMime *converter = QWindowsMime::converterToMime(mimeType, pDataObj);

    if (converter)
        result = converter->convertToMime(mimeType, pDataObj, type);

    pDataObj->Release();

    return result;
}

class QClipboardData
{
public:
    QClipboardData()
        : iData(0)
        , nextClipboardViewer(0)
    {
        clipBoardViewer = new QWidget();
        clipBoardViewer->createWinId();
        clipBoardViewer->setObjectName(QLatin1String("internal clipboard owner"));
    }

    ~QClipboardData()
    {
        Q_ASSERT(clipBoardViewer->testAttribute(Qt::WA_WState_Created));
        ChangeClipboardChain(clipBoardViewer->internalWinId(), nextClipboardViewer);
        delete clipBoardViewer;
        releaseIData();
    }

    void releaseIData()
    {
        if (iData) {
            delete iData->mimeData();
            iData->releaseQt();
            iData->Release();
            iData = 0;
        }
    }

    QOleDataObject * iData;
    QWidget *clipBoardViewer;
    HWND nextClipboardViewer;
    QClipboardWatcher watcher;
};

static QClipboardData *ptrClipboardData = 0;

static QClipboardData *clipboardData()
{
    if (ptrClipboardData == 0) {
        ptrClipboardData = new QClipboardData;
        // this needs to be done here to avoid recursion
        Q_ASSERT(ptrClipboardData->clipBoardViewer->testAttribute(Qt::WA_WState_Created));
        ptrClipboardData->nextClipboardViewer = SetClipboardViewer(ptrClipboardData->clipBoardViewer->internalWinId());
    }
    return ptrClipboardData;
}

static void cleanupClipboardData()
{
    delete ptrClipboardData;
    ptrClipboardData = 0;
}

QClipboard::~QClipboard()
{
    cleanupClipboardData();
}

void QClipboard::setMimeData(QMimeData *src, Mode mode)
{
    if (mode != Clipboard)
        return;

    QClipboardData *d = clipboardData();

    if (!(d->iData && d->iData->mimeData() == src)) {
        d->releaseIData();
        d->iData = new QOleDataObject(src);
    }

    if (OleSetClipboard(d->iData) != S_OK) {
        d->releaseIData();
        qErrnoWarning("QClipboard::setMimeData: Failed to set data on clipboard");
        return;
    }

}

void QClipboard::clear(Mode mode)
{
    if (mode != Clipboard) return;

    QClipboardData *d = clipboardData();

    d->releaseIData();

    if (OleSetClipboard(0) != S_OK) {
        qErrnoWarning("QClipboard::clear: Failed to clear data on clipboard");
        return;
    }
}

bool QClipboard::event(QEvent *e)
{
    if (e->type() != QEvent::Clipboard)
        return QObject::event(e);

    QClipboardData *d = clipboardData();

    MSG *m = (MSG *)((QClipboardEvent*)e)->data();
    if (!m) {
        // this is sent to render all formats at app shut down
        if (ownsClipboard()) {
            OleFlushClipboard();
            d->releaseIData();
        }
        return true;
    }

    bool propagate = false;

    if (m->message == WM_CHANGECBCHAIN) {
        if ((HWND)m->wParam == d->nextClipboardViewer)
            d->nextClipboardViewer = (HWND)m->lParam;
        else
            propagate = true;
    } else if (m->message == WM_DRAWCLIPBOARD) {
        emitChanged(QClipboard::Clipboard);
        if (!ownsClipboard() && d->iData)
            // clean up the clipboard object if we no longer own the clipboard
            d->releaseIData();
        propagate = true;
    }

    if (propagate && d->nextClipboardViewer) {
        QT_WA({
            SendMessage(d->nextClipboardViewer, m->message,
                         m->wParam, m->lParam);
        } , {
            SendMessageA(d->nextClipboardViewer, m->message,
                         m->wParam, m->lParam);
        });
    }

    return true;
}

void QClipboard::connectNotify(const char *signal)
{
    if (qstrcmp(signal,SIGNAL(dataChanged())) == 0) {
        // ensure we are up and running but block signals so the dataChange signal
        // is not emitted while being connected to.
        bool blocked = blockSignals(true);
        QClipboardData *d = clipboardData();
        blockSignals(blocked);
        Q_UNUSED(d);
    }
}

const QMimeData *QClipboard::mimeData(Mode mode) const
{
    if (mode != Clipboard)
        return 0;

    QClipboardData *data = clipboardData();
    // sort cut for local copy / paste
    if (ownsClipboard() && data->iData->mimeData())
        return data->iData->mimeData();
    return &data->watcher;
}

bool QClipboard::supportsMode(Mode mode) const
{
    return (mode == Clipboard);
}

bool QClipboard::ownsMode(Mode mode) const
{
    if (mode == Clipboard) {
        QClipboardData *d = clipboardData();
        return d->iData && OleIsCurrentClipboard(d->iData) == S_OK;
    } else {
        return false;
    }
}

void QClipboard::ownerDestroyed()
{
}

#endif // QT_NO_CLIPBOARD
