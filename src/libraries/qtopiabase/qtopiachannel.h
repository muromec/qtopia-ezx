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

#ifndef __QTOPIACHANNEL_H__
#define __QTOPIACHANNEL_H__

#include <QObject>
#include <qtopiaglobal.h>

class QtopiaChannel_Private;
class QTOPIABASE_EXPORT QtopiaChannel : public QObject
{
    Q_OBJECT

public:
    explicit QtopiaChannel(const QString& channel, QObject *parent=0);
    virtual ~QtopiaChannel();

    QString channel() const;

    static bool isRegistered(const QString&  channel);
    static bool send(const QString& channel, const QString& msg);
    static bool send(const QString& channel, const QString& msg,
                     const QByteArray &data);
    static bool flush();

signals:
    void received(const QString& msg, const QByteArray &data);

private:
    QtopiaChannel_Private *m_data;
    friend class QtopiaChannel_Private;
};

#endif
