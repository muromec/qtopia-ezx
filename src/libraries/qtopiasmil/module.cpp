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

#include "module.h"
#include "element.h"

SmilModule::~SmilModule()
{
}

QStringList SmilModule::elements() const
{
    QStringList l;
    return l;
}

QStringList SmilModule::attributes() const
{
    QStringList l;
    return l;
}

void SmilModule::process()
{
}

//===========================================================================

SmilModuleAttribute::SmilModuleAttribute(SmilElement *e, const QXmlAttributes &/*atts*/)
    : element(e)
{
    e->addModule(this);
}

SmilModuleAttribute::~SmilModuleAttribute()
{
}

void SmilModuleAttribute::process()
{
}

void SmilModuleAttribute::reset()
{
}

void SmilModuleAttribute::event(SmilElement *, SmilEvent *)
{
}

QString SmilModuleAttribute::name() const
{
    return "";
}

void SmilModuleAttribute::paint(QPainter *)
{
}

