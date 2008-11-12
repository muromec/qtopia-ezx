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

#include <qtopia/smil/element.h>
#include <qtopia/smil/module.h>

class SmilPrefetch : public SmilElement
{
public:
    SmilPrefetch(SmilSystem *sys, SmilElement *p, const QString &n, const QXmlAttributes &atts);
};

class SmilSwitch : public SmilElement
{
public:
    SmilSwitch(SmilSystem *sys, SmilElement *p, const QString &n, const QXmlAttributes &atts);
};

//===========================================================================

class SmilContentModule : public SmilModule
{
public:
    SmilContentModule();
    virtual ~SmilContentModule();

    virtual SmilElement *beginParseElement(SmilSystem *, SmilElement *, const QString &qName, const QXmlAttributes &atts);
    virtual bool parseAttributes(SmilSystem *sys, SmilElement *e, const QXmlAttributes &atts);
    virtual void endParseElement(SmilElement *, const QString &qName);
    virtual QStringList elements() const;
};

