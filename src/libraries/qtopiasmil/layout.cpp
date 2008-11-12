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

#include "layout.h"
#include <qxml.h>
#include <qpainter.h>

SmilLayout::SmilLayout(SmilSystem *sys, SmilElement *p, const QString &n, const QXmlAttributes &atts)
    : SmilElement(sys, p, n, atts)
{
    currentState = Active;
    vis = true;
}

SmilLayout::~SmilLayout()
{
}

//===========================================================================

SmilRootLayout::SmilRootLayout(SmilSystem *sys, SmilElement *p, const QString &n, const QXmlAttributes &atts)
    : SmilElement(sys, p, n, atts)
{
    currentState = Active;
    vis = true;
    QString val = atts.value("width");
    if (!val.isEmpty())
        size.setWidth(val.toInt());
    val = atts.value("height");
    if (!val.isEmpty())
        size.setHeight(val.toInt());
    val = atts.value("backgroundColor");
    if (val.isEmpty())
        val = atts.value("background-color");
    if (!val.isEmpty())
        setBackgroundColor(parseColor(val));
    else
        setBackgroundColor(Qt::white);
}

void SmilRootLayout::process()
{
    SmilElement *root = parent();
    while (root->parent())
        root = root->parent();

    if (root) {
        QRect r = root->rect();
        if (size.width())
            r.setWidth(size.width());
        if (size.height())
            r.setHeight(size.height());
        root->setRect(r);
    }

    setRect(root->rect());
}

void SmilRootLayout::paint(QPainter *p)
{
    p->fillRect(rect(), backgroundColor());
}

//===========================================================================

SmilRegion::SmilRegion(SmilSystem *sys, SmilElement *p, const QString &n, const QXmlAttributes &atts)
    : SmilElement(sys, p, n, atts), fit(FillFit), showBackground(ShowAlways), zIndex(0)
{
    currentState = Active;
    vis = true;
    QString val = atts.value("backgroundColor");
    if (val.isEmpty())
        val = atts.value("background-color");
    if (!val.isEmpty())
        setBackgroundColor(parseColor(val));
    else
        setBackgroundColor(Qt::white);
    val = atts.value("fit");
    if (val == "hidden")
        fit = HiddenFit;
    else if (val == "meet")
        fit = MeetFit;
    else if (val == "scroll")
        fit = ScrollFit;
    else if (val == "slice")
        fit = SliceFit;
    regionName = atts.value("regionName");
    if (regionName.isEmpty())
        regionName = atts.value("id");
    if (atts.value("showBackground") == "whenActive")
        showBackground = ShowActive;
    val = atts.value("z-index");
    if (!val.isEmpty())
        zIndex = val.toInt();

    right = parsePosition(atts.value("right"));
    width = parsePosition(atts.value("width"));
    left = parsePosition(atts.value("left"));

    bottom = parsePosition(atts.value("bottom"));
    height = parsePosition(atts.value("height"));
    top = parsePosition(atts.value("top"));
}

void SmilRegion::process()
{
    SmilElement::process();

    SmilElement *root = parent();
    while (root->parent())
        root = root->parent();

    QRect rr;
    rr.setSize(root->rect().size());

    int w = root->rect().width();
    int h = root->rect().height();

    // Horizontal
    if (left.type == Position::Unspecified) {
        if (width.type == Position::Unspecified)
            rr.setWidth(w-right.value);
        else if (width.type == Position::Absolute)
            rr.setWidth(width.value);
        else if (width.type == Position::Percent)
            rr.setWidth(w*width.value/100);
    } else {
        if (left.type == Position::Percent)
            rr.setLeft(w*left.value/100);
        else
            rr.setLeft(left.value);
        if (width.type == Position::Unspecified)
            rr.setWidth(w-rr.left());
        else if (width.type == Position::Absolute)
            rr.setWidth(width.value);
        else if (width.type == Position::Percent)
            rr.setWidth(w*width.value/100);
    }

    // Vertical
    if (top.type == Position::Unspecified) {
        if (height.type == Position::Unspecified)
            rr.setHeight(h-bottom.value);
        else if (height.type == Position::Absolute)
            rr.setHeight(height.value);
        else if (height.type == Position::Percent)
            rr.setHeight(h*height.value/100);
    } else {
        if (top.type == Position::Percent)
            rr.setTop(h*top.value/100);
        else
            rr.setTop(top.value);
        if (height.type == Position::Unspecified)
            rr.setHeight(h-rr.top());
        else if (height.type == Position::Absolute)
            rr.setHeight(height.value);
        else if (height.type == Position::Percent)
            rr.setHeight(h*height.value/100);
    }

    setRect(rr);
}

void SmilRegion::setRect(const QRect &r)
{
    SmilElement::setRect(r);
    SmilElementList::ConstIterator it;
    for (it = referrers.begin(); it != referrers.end(); ++it) {
        (*it)->setRect(r);
    }
}

void SmilRegion::paint(QPainter *p)
{
    bool show = true;
    if (showBackground == ShowActive) {
        show = false;
        SmilElementList::ConstIterator it;
        for (it = referrers.begin(); it != referrers.end(); ++it) {
            if ((*it)->state() == Active) {
                show = true;
                break;
            }
        }
    }
    if (show)
        p->fillRect(rect(), backgroundColor());
}

void SmilRegion::addReferrer(SmilElement *e)
{
    referrers.append(e);
}

SmilRegion::Position SmilRegion::parsePosition(const QString &val)
{
    Position pos;
    if (val.isEmpty())
        return pos;

    int idx = val.indexOf('%');
    if (idx > 0) {
        pos.value = val.left(idx).toInt();
        pos.type = Position::Percent;
        return pos;
    }

    idx = val.indexOf("px");
    if (idx < 0)
        idx = val.length();
    pos.value = val.left(idx).toInt();
    pos.type = Position::Absolute;

    return pos;
}

//===========================================================================

SmilLayoutModule::SmilLayoutModule()
{
}

SmilLayoutModule::~SmilLayoutModule()
{
}

SmilElement *SmilLayoutModule::beginParseElement(SmilSystem *sys, SmilElement *e, const QString &qName, const QXmlAttributes &atts)
{
    if (qName == "layout") {
        SmilLayout *l = new SmilLayout(sys, e, qName, atts);
        layouts.append(l);
        return l;
    } else if (qName == "region") {
        return new SmilRegion(sys, e, qName, atts);
    } else if (qName == "root-layout") {
        return new SmilRootLayout(sys, e, qName, atts);
    }

    return 0;
}

bool SmilLayoutModule::parseAttributes(SmilSystem *, SmilElement *e, const QXmlAttributes &atts)
{
    QString val = atts.value("region");
    if (!val.isEmpty()) {
        new RegionAttribute(this, e, atts);
        return true;
    }

    return false;
}

void SmilLayoutModule::endParseElement(SmilElement *e, const QString &)
{
    if (e->name() == "head") {
        // Ensure there is a layout
        if (!e->findChild(QString(), "layout")) {
            // We need to add a default layout
            QXmlAttributes atts;
            SmilLayout* layout = new SmilLayout(e->system(), e, "layout", atts);

            atts.clear();
            atts.append("id", QString(), QString(), "image");
            atts.append("width", QString(), QString(), "100%");
            atts.append("height", QString(), QString(), "67%");
            SmilRegion* region = new SmilRegion(e->system(), layout, "region", atts);
            layout->addChild(region);

            atts.clear();
            atts.append("id", QString(), QString(), "text");
            atts.append("width", QString(), QString(), "100%");
            atts.append("height", QString(), QString(), "33%");
            region = new SmilRegion(e->system(), layout, "region", atts);
            layout->addChild(region);

            e->addChild(layout);
            layouts.append(layout);
        }
    }
}

QStringList SmilLayoutModule::elements() const
{
    QStringList l;
    l.append("layout");
    l.append("region");
    l.append("root-layout");
    return l;
}

QStringList SmilLayoutModule::attributes() const
{
    QStringList l;
    l.append("region");
    return l;
}

SmilRegion *SmilLayoutModule::findRegion(const QString &id)
{
    QList<SmilLayout*>::Iterator it;
    for (it = layouts.begin(); it != layouts.end(); ++it) {
        SmilLayout *l = *it;
        SmilElementList::ConstIterator lit;
        for (lit = l->children().begin(); lit != l->children().end(); ++lit) {
            if ((*lit)->name() == "region") {
                SmilRegion *r = (SmilRegion*)(*lit);
                if (r->regionName == id)
                    return r;
            }
        }
    }

    return 0;
}

//===========================================================================

RegionAttribute::RegionAttribute(SmilLayoutModule *m, SmilElement *e, const QXmlAttributes &atts)
    : SmilModuleAttribute(e, atts), layoutModule(m)
{
    rgn = atts.value("region");
}

void RegionAttribute::process()
{
    SmilRegion *r = layoutModule->findRegion(rgn);
    if (r) {
        r->addReferrer(element);

        element->setRect(r->rect());
        element->setBackgroundColor(r->backgroundColor());
    } else {
        qWarning("Cannot find region: %s", rgn.toLatin1().constData());
    }
}

