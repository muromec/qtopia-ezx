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

#include "content.h"

SmilPrefetch::SmilPrefetch(SmilSystem *sys, SmilElement *p, const QString &n, const QXmlAttributes &atts)
    : SmilElement(sys, p, n, atts)
{
}

SmilSwitch::SmilSwitch(SmilSystem *sys, SmilElement *p, const QString &n, const QXmlAttributes &atts)
    : SmilElement(sys, p, n, atts)
{
}

//===========================================================================

SmilContentModule::SmilContentModule()
    : SmilModule()
{
}

SmilContentModule::~SmilContentModule()
{
}

SmilElement *SmilContentModule::beginParseElement(SmilSystem *sys, SmilElement *e, const QString &qName, const QXmlAttributes &atts)
{
    if (qName == "switch") {
        return new SmilSwitch(sys, e, qName, atts);
    } else if (qName == "prefetch") {
        return new SmilPrefetch(sys, e, qName, atts);
    }

    return 0;
}

bool SmilContentModule::parseAttributes(SmilSystem *, SmilElement *, const QXmlAttributes &)
{
    return false;
}

void SmilContentModule::endParseElement(SmilElement *, const QString &)
{
    //### we need to process now, and remove element where necessary.
}

QStringList SmilContentModule::elements() const
{
    QStringList l;
    l.append("switch");
    l.append("prefetch");
    return l;
}

