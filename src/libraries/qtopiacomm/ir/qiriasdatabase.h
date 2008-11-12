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

#ifndef __QIRIASDATABASE_H__
#define __QIRIASDATABASE_H__

#include <qirglobal.h>

class QIR_EXPORT QIrIasDatabase
{
public:
    static bool setAttribute(const QString &className,
                             const QString &attribName,
                             const QVariant &attr);
    static QVariant attribute(const QString &className,
                                     const QString &attribName);
    static bool removeAttribute(const QString &className,
                                const QString &attribName);
};

#endif
