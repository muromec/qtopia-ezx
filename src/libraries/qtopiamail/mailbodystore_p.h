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

#ifndef __MAILBODYSTORE_H
#define __MAILBODYSTORE_H

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

#include <QString>

class QMailMessage;

class MailBodyStore
{
public:
    MailBodyStore();
    ~MailBodyStore();

    bool insert(const QMailMessage& m, QString* newFile);
    bool remove(const QString& fileName);
    bool update(const QString& fileName, const QMailMessage& data);
    bool load(const QString& fileName, QMailMessage* out) const;

private:
    QString generateUniqueFileName() const;
    QString randomString(int length) const;

private:
    QString _storePath;
};

#endif //__MAILBODYSTORE_H
