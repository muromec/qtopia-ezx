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

#ifndef _QTOPIAFEATURES_H_
#define _QTOPIAFEATURES_H_

#include <qtopiaglobal.h>
class QString;

class QTOPIA_EXPORT QtopiaFeatures
{
public:
    static void setFeature(const QString &);
    static void removeFeature(const QString &);
    static bool hasFeature(const QString &);
};

#endif // _QTOPIAFEATURES_H_
