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
#ifndef QDRMCONTENTENGINE_P_H
#define QDRMCONTENTENGINE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qtopia API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qtopia/private/qfscontentengine_p.h>

class QDrmContentEngine : public QFSContentEngine
{
public:
    QDrmContentEngine();
    virtual ~QDrmContentEngine();

    virtual QIODevice *open( QIODevice::OpenMode mode );

    virtual bool execute( const QStringList &arguments ) const;

    virtual QDrmRights::Permissions queryPermissions();

    virtual QDrmRights rights( QDrmRights::Permission ) const;

    virtual bool canActivate() const;

    virtual bool activate( QDrmRights::Permission permission, QWidget *parent );

    virtual bool reactivate( QDrmRights::Permission permission, QWidget *parent );

    virtual QDrmContentLicense *requestLicense( QDrmRights::Permission permission, QDrmContent::LicenseOptions options );

    virtual QContentEngine *createCopy() const;

protected:
    QDrmContentEngine( const QString &engineType );
};

#endif
