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

#include "smilviewer.h"

#include <qtopia/smil/element.h>
#include <qtopia/smil/smil.h>
#include <qtopia/smil/timing.h>
#include <qtopia/smil/transfer.h>
#include <qtopialog.h>

#include <QBuffer>
#include <QKeyEvent>
#include <QMailMessage>
#include <QSoftMenuBar>

SmilViewer::SmilViewer( QWidget* parent )
    : QMailViewerInterface( parent ),
      view( new SmilView( parent ) ),
      menuKey( QSoftMenuBar::menuKey() ),
      mail( 0 )
{
    view->setGeometry(parent->rect());
    view->installEventFilter(this);
    view->setFocusPolicy(Qt::StrongFocus);

    connect(view, SIGNAL(transferRequested(SmilDataSource*,QString)),
            this, SLOT(requestTransfer(SmilDataSource*,QString)));
    connect(view, SIGNAL(transferCancelled(SmilDataSource*,QString)),
            this, SLOT(cancelTransfer(SmilDataSource*,QString)));

    connect(view, SIGNAL(finished()),
            this, SIGNAL(finished()));
}

SmilViewer::~SmilViewer()
{
}

QWidget* SmilViewer::widget() const
{
    return view;
}

// This logic is replicated in the MMS composer...
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

bool SmilViewer::setMessage(const QMailMessage& msg)
{
    mail = &msg;

    uint smilPartIndex = smilStartIndex(*mail);

    const QMailMessagePart &part = mail->partAt( smilPartIndex );
    QString smil(part.body().data());

    if (view->setSource(smil) && view->rootElement()) {
        tweakView();
        view->play();
        return true;
    }

    return false;
}

bool SmilViewer::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == view && event->type() == QEvent::KeyPress) {
        if (QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event)) {
            if (keyEvent->key() == Qt::Key_Select) {
                advanceSlide();
                return true;
            } else if (keyEvent->key() == Qt::Key_Back) {
                emit finished();
                return true;
            } else if (keyEvent->key() == menuKey) {
                // The menu should not be activated while we're active
                return true;
            }
        }
    }

    return false;
}

void SmilViewer::clear()
{
    view->reset();

    mail = 0;
    QMap<SmilDataSource*,QIODevice*>::ConstIterator it = transfers.begin(), end = transfers.end();
    for ( ; it != end; ++it)
        (*it)->deleteLater();

    transfers.clear();
}

void SmilViewer::tweakView()
{
    // Try to make sure the layout works on our display
    SmilElement *layout = view->rootElement()->findChild(QString(), "layout", true);
    if (!layout)
        return;

    QRect rl = view->rect();
    SmilElement *rootLayout = layout->findChild(QString(), "root-layout");
    if (rootLayout) {
        if (rootLayout->rect().width() > view->width() || 
            rootLayout->rect().height() > view->height()) {
            rootLayout->setRect(QRect(0, 0, view->width(), view->height()));
        }
        rl = rootLayout->rect();
    }

    SmilElement *imageLayout = layout->findChild("Image", "region");
    if (!imageLayout)
        imageLayout = layout->findChild("image", "region");

    SmilElement *textLayout = layout->findChild("Text", "region");
    if (!textLayout)
        textLayout = layout->findChild("text", "region");

    if (imageLayout && textLayout) {
        QRect il = imageLayout->rect();
        QRect tl = textLayout->rect();
        if (il.bottom() > tl.top() ||
            il.right() > rl.right() ||
            tl.right() > rl.right() ||
            il.bottom() > rl.bottom() ||
            tl.bottom() > rl.bottom()) {
            // Not going to fit - use our preferred sizes.
            il = tl = rl;
            il.setBottom(il.top() + rl.height() * 2 / 3);
            tl.setTop(il.bottom() + 1);
            tl.setHeight(rl.height() - il.height());
            imageLayout->setRect(il);
            textLayout->setRect(tl);
        }
    }
}

void SmilViewer::advanceSlide()
{
    // Try to advance to the next slide
    SmilElement *smil = view->rootElement();
    if (!smil)
        return;

    SmilElement *body = smil->findChild(QString(), "body", true);
    if (!body)
        return;

    SmilElementList::ConstIterator it;
    for (it = body->children().begin(); it != body->children().end(); ++it) {
        SmilElement *e = *it;
        if (e->name() == "par") {
            if (e->state() == SmilElement::Active) {
                // This should be the current active slide
                SmilTimingAttribute *at = static_cast<SmilTimingAttribute*>(e->module("Timing"));
                Duration d(at->startTime.elapsed());    // i.e. end now.
                e->setCurrentEnd(d);
                break;
            }
        }
    }
}

void SmilViewer::requestTransfer(SmilDataSource* dataSource, const QString &src)
{
    for ( uint i = 0; i < mail->partCount(); i++ ) {
        const QMailMessagePart &part = mail->partAt( i );
        if (smilPartMatch(src, part)) {
            dataSource->setMimeType(part.contentType().content());

            const QString filename(part.attachmentPath());
            if (filename.isEmpty()) {
                QBuffer *data = new QBuffer();
                data->setData(part.body().data(QMailMessageBody::Decoded));
                data->open(QIODevice::ReadOnly);
                dataSource->setDevice(data);
                transfers[dataSource] = data;
            } else {
                QFile *file = new QFile(filename);
                file->open(QIODevice::ReadOnly);
                dataSource->setDevice(file);
                transfers[dataSource] = file;
            }
            break;
        }
    }
}

void SmilViewer::cancelTransfer(SmilDataSource *dataSource, const QString &src)
{
    if (transfers.contains(dataSource))
        transfers.take(dataSource)->deleteLater();

    Q_UNUSED(src)
}

QTOPIA_EXPORT_PLUGIN( SmilViewerPlugin )

SmilViewerPlugin::SmilViewerPlugin()
    : QMailViewerPlugin()
{
}

QString SmilViewerPlugin::key() const
{
    return "SmilViewer";
}

bool SmilViewerPlugin::isSupported( QMailViewerFactory::ContentType type ) const
{
    return ( type == QMailViewerFactory::SmilContent || type == QMailViewerFactory::AnyContent );
}

QMailViewerInterface* SmilViewerPlugin::create( QWidget *parent )
{
    return new SmilViewer( parent );
}

