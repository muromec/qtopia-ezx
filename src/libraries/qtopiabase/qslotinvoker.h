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

#ifndef _QTOPIAREMOTE_QSLOTINVOKER_H
#define _QTOPIAREMOTE_QSLOTINVOKER_H

#include <QtCore/qobject.h>
#include <QtCore/qvariant.h>
#include <QtCore/qlist.h>
#include <qtopiaglobal.h>

class QSlotInvokerPrivate;

class QTOPIABASE_EXPORT QSlotInvoker : public QObject
{
    Q_OBJECT
public:
    QSlotInvoker( QObject *receiver, const QByteArray& member,
                  QObject *parent=0 );
    ~QSlotInvoker();

    bool memberPresent() const;
    bool canInvoke( int numArgs ) const;
    QObject *receiver() const;
    QByteArray member() const;
    int *parameterTypes() const;
    int parameterTypesCount() const;

public slots:
    QVariant invoke( const QList<QVariant>& args );

private slots:
    void receiverDestroyed();

private:
    QSlotInvokerPrivate *d;
};

#endif /* _QTOPIAREMOTE_QSLOTINVOKER_H */
