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

#ifndef __QTOPIA_MEDIA_ABSTRACTCONTROLSERVER_H
#define __QTOPIA_MEDIA_ABSTRACTCONTROLSERVER_H

#include <QObject>
#include <QString>
#include <QVariant>

#include "qmediahandle_p.h"

#include <qtopiaglobal.h>

class QMediaAbstractControlServerPrivate;

class QTOPIAMEDIA_EXPORT QMediaAbstractControlServer : public QObject
{
    Q_OBJECT

public:
    QMediaAbstractControlServer(QMediaHandle const& handle,
                                QString const& name,
                                QObject* parent = 0);
    ~QMediaAbstractControlServer();

signals:
    void controlAvailable(QString const& name);
    void controlUnavailable(QString const& name);

protected:
    void setValue(QString const& name, QVariant const& value);

    void proxyAll();

private slots:
    void itemRemove(const QByteArray &attribute);
    void itemSetValue(const QByteArray &attribute, const QVariant &value);

private:
    QMediaAbstractControlServerPrivate* d;
};

#endif  // __QTOPIA_MEDIA_ABSTRACTCONTROLSERVER_H
