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

#ifndef __QTOPIA_MEDIALIBRARY_MEDIAHANDLE_H
#define __QTOPIA_MEDIALIBRARY_MEDIAHANDLE_H

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

#include <QUuid>

#include <qtopiaglobal.h>


class QTOPIAMEDIA_EXPORT QMediaHandle
{
public:
    QMediaHandle() {}
    explicit QMediaHandle(QUuid const& id):
        m_id(id) {}
    QMediaHandle(QMediaHandle const& c):
        m_id(c.m_id) {}

    QMediaHandle& operator=(QMediaHandle const& rhs)
    {
        m_id = rhs.m_id;
        return *this;
    }

    QString toString() const
    {
        return m_id.toString();
    }

    QUuid const& id() const
    {
        return m_id;
    }

private:
    QUuid   m_id;
};


#endif  // __QTOPIA_MEDIALIBRARY_MEDIAHANDLE_H

