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

#include "smil.h"
#include "system.h"
#include "structure.h"
#include "layout.h"
#include "content.h"
#include "timing.h"
#include "media.h"
#include "transfer.h"
#include <qxml.h>
#include <qstack.h>
#include <qmap.h>
#include <qfile.h>
#include <qpainter.h>


class SmilParser : public QXmlDefaultHandler
{
public:
    SmilParser(SmilSystem *s);

    bool endDocument();
    bool startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &atts);
    bool endElement(const QString &namespaceURI, const QString &localName, const QString &qName);
    bool characters(const QString &ch);
    bool error(const QXmlParseException &exception);
    bool fatalError(const QXmlParseException &exception);

private:
    SmilSystem *sys;
    QStack<SmilElement*> parseStack;
    SmilElement *root;
    SmilElement *current;
    int ignore;
};

SmilParser::SmilParser(SmilSystem *s)
    : QXmlDefaultHandler(), sys(s), root(0), current(0),
        ignore(0)
{
    sys->addModule("Structure", new SmilStructureModule());
    sys->addModule("Content", new SmilContentModule());
    sys->addModule("Layout", new SmilLayoutModule());
    sys->addModule("Timing", new SmilTimingModule());
    sys->addModule("Media", new SmilMediaModule());
}

bool SmilParser::endDocument()
{
    if (root) {
        sys->setRootElement(root);
        return true;
    }

    return false;
}

bool SmilParser::startElement(const QString &/*namespaceURI*/, const QString &/*localName*/, const QString &qName, const QXmlAttributes &atts)
{
    SmilElement *e = 0;
    QMap<QString, SmilModule *>::ConstIterator it;
    for (it = sys->modules().begin(); it != sys->modules().end(); ++it) {
        e = (*it)->beginParseElement(sys, current, qName, atts);
        if (e) {
            if (current)
                current->addChild(e);
            parseStack.push(e);
            break;
        }
    }

    if (e) {
        current = e;
        if (!root)
            root = e;
        for (it = sys->modules().begin(); it != sys->modules().end(); ++it) {
            (*it)->parseAttributes(sys, e, atts);
        }
    } else {
        ++ignore;
    }

    return true;
}

bool SmilParser::endElement(const QString &/*namespaceURI*/, const QString &/*localName*/, const QString &qName)
{
    if (ignore) {
        ignore--;
    } else {
        QMap<QString, SmilModule *>::ConstIterator it;
        for (it = sys->modules().begin(); it != sys->modules().end(); ++it) {
            (*it)->endParseElement(current, qName);
        }
        if (parseStack.top() != root)
            parseStack.pop();
        current = parseStack.top();
    }

    return true;
}

bool SmilParser::characters(const QString &ch)
{
    if (current && !ignore) {
        current->addCharacters(ch);
    }

    return true;
}

bool SmilParser::error(const QXmlParseException &exception)
{
    qWarning("%s (line %d)", exception.message().toLatin1().constData(), exception.lineNumber());
    return true;
}

bool SmilParser::fatalError(const QXmlParseException &exception)
{
    qWarning("%s (line %d)", exception.message().toLatin1().constData(), exception.lineNumber());
    return true;
}

//===========================================================================

class SmilViewPrivate
{
public:
    SmilViewPrivate() : sys(0), parser(0) {}

    SmilSystem *sys;
    SmilParser *parser;
};


SmilView::SmilView(QWidget *parent, Qt::WFlags f)
    : QWidget(parent, f), d(0)
{
    d = new SmilViewPrivate;
}

SmilView::~SmilView()
{
    clear();
    delete d;
}

bool SmilView::setSource(const QString &str)
{
    clear();
    d->sys = new SmilSystem();
    d->sys->setTarget(this);
    connect(d->sys->transferServer(), SIGNAL(transferRequested(SmilDataSource*,QString)),
            this, SIGNAL(transferRequested(SmilDataSource*,QString)));
    connect(d->sys->transferServer(), SIGNAL(transferCancelled(SmilDataSource*,QString)),
            this, SIGNAL(transferCancelled(SmilDataSource*,QString)));
    connect(d->sys, SIGNAL(finished()), this, SIGNAL(finished()));
    d->parser = new SmilParser(d->sys);
    QXmlInputSource source;
    source.setData(str);
    QXmlSimpleReader reader;
    reader.setContentHandler(d->parser);
    reader.setErrorHandler(d->parser);
    if (!reader.parse( source )){
        qWarning("Unable to parse SMIL file");
        clear();
        return false;
    } else {
        QPalette pal;
        pal.setColor(QPalette::Window, d->sys->rootColor());
        setPalette(pal);
    }

    return true;
}

void SmilView::play()
{
    if (d->sys)
        d->sys->play();
}

void SmilView::reset()
{
    if (d->sys)
        d->sys->reset();
}

SmilElement *SmilView::rootElement() const
{
    if (d->sys)
        return d->sys->rootElement();
    return 0;
}

void SmilView::paintEvent(QPaintEvent *)
{
    if (d->sys) {
        // ### handle layer z-order.
        QPainter p(this);
        p.setClipRegion(d->sys->dirtyRegion());
        d->sys->paint(&p);
        d->sys->setDirty(QRect());
    }
}

void SmilView::clear()
{
    delete d->sys;
    delete d->parser;
    d->sys = 0;
    d->parser = 0;
}

