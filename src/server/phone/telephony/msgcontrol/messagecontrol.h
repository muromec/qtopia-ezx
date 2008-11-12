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

#ifndef _MESSAGECONTROL_H_
#define _MESSAGECONTROL_H_

#include <QObject>
#include <qvaluespace.h>
#ifdef QTOPIA_CELL
#include <qsmsreader.h>
#endif
#include <qtopiaipcenvelope.h>
#include <QtopiaIpcAdaptor>
class QString;

class QCommServiceManager;

class MessageControl : public QObject
{
Q_OBJECT
public:
    static MessageControl *instance();

    int messageCount() const;
    bool smsFull() const;
    QString lastSmsId() const;

signals:
    void messageCount(int, bool, bool, bool);
    void newMessage(const QString &type, const QString &from, const QString &subject);
    void smsMemoryFull(bool);
    void messageRejected();

private slots:
    void smsUnreadCountChanged();
    void telephonyServicesChanged();
    void sysMessage(const QString& message, const QByteArray&);
    void smsMemoryFullChanged();
    void messageCountChanged();

private:
    QValueSpaceObject phoneValueSpace;
    QValueSpaceItem smsMemFull;
    void doNewCount(bool write=true, bool fromSystem=false, bool notify=true);
    MessageControl();
    QCommServiceManager *mgr;
#ifdef QTOPIA_CELL
    QSMSReader *smsreq;
#endif
    QtopiaIpcAdaptor *messageCountUpdate;
    QtopiaChannel channel;
    QString smsId;

    int smsCount;
    int mmsCount;
    int systemCount;
    bool smsIsFull;
    int prevSmsMemoryFull;
};

#endif // _MESSAGECONTROL_H_
