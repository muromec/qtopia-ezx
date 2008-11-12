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



#ifndef ACCOUNTLIST_H
#define ACCOUNTLIST_H

#include <qobject.h>
#include <qlist.h>
#include <qstringlist.h>

#include <qtopiaglobal.h>

#include "account.h"

class QTOPIAMAIL_EXPORT AccountList : public QObject
{
    Q_OBJECT

public:
    AccountList(QObject *parent=0, const char *name=0);

    QListIterator<QMailAccount*> accountIterator();
    int count();

    //temp while converting
    QMailAccount* at(int x);
    void append(QMailAccount *a);
    void remove(int pos);
    void remove(QMailAccount* const a);

    void readAccounts();
    void saveAccounts();

    uint defaultMailServerCount();
    QMailAccount* defaultMailServer();
    void setDefaultMailServer(QMailAccount *account);

    QMailAccount* getSmtpRef();
    QMailAccount* getPopRefByAccount(QString user);
    QMailAccount* getAccountById(QString id);
    QMailAccount* getSmtpRefByMail(QString email);
    QStringList emailAccounts();
    QString getUserName();

signals:
    void accountListChanged(); //not currently used
    void checkAccount(int);

public slots:
    void intervalCheck(QMailAccount*);

private:
    QList<QMailAccount*> list;
};

#endif
