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

#ifndef QCONTENTPLUGIN_H
#define QCONTENTPLUGIN_H

#include <qcontent.h>
#include <qtopiaglobal.h>

class QTOPIA_EXPORT QContentPlugin
{
public:
    virtual ~QContentPlugin();

    virtual QStringList keys() const = 0;

    virtual bool installContent( const QString &path, QContent *content ) = 0;
    virtual bool updateContent( QContent *content );
};

Q_DECLARE_INTERFACE( QContentPlugin, "com.trolltech.Qtopia.QContentPlugin/1.0" );

#endif
