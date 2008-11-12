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



#ifndef SEARCH_H
#define SEARCH_H

#include <QDate>
#include <QObject>
#include <QString>

#include <qtopiaglobal.h>

#include <QMailMessage>

class QSettings;

class QTOPIAMAIL_EXPORT Search : public QObject
{
    Q_OBJECT

public:
    Search();

    enum SearchFlag {
        Any = 0,
        Read,
        Unread,
        Replied
    };
    typedef uint MailStatus;

    void reset();
    bool matches(const QMailMessage& in) const;
    void setMailbox(QString mailbox);
    QString mailbox() const;
    QString name() const;
    void setName(QString in);
    void setMailFrom(QString from);
    void setMailTo(QString to);
    void setMailSubject(QString subject);
    void setMailBody(QString body);
    void setStatus(MailStatus s);
    void setBeforeDate(QDate date);
    void setAfterDate(QDate date);
    void setFromFolder(QString _folder);
    void setFromAccount(QString _fromAccount);

    uint status() const;
    QString getFrom() const;
    QString getTo() const;
    QString getSubject() const;
    QString getBody() const;
    QDate getBeforeDate() const;
    QDate getAfterDate() const;

    void readSettings(QSettings*);
    void saveSettings(QSettings*);

private:
    bool matchesTo(const QMailMessage& mail) const;
    bool matchesBody(const QMailMessage& mail) const;
    bool matchesStatus(const QMailMessage& mail) const;

    bool matchesBeforeDate(const QMailMessage& mail) const;
    bool matchesAfterDate(const QMailMessage& mail) const;
    bool matchesFolder(const QMailMessage& mail) const;
    bool matchesAccount(const QMailMessage& mail) const;

    static bool match(const QString &source, const QString &target);

private:
    uint _status;
    QString _name, fromMail, recipient, subject, body, folder;
    QString fromAccount;
    QString _mailbox;
    QDate beforeDate, afterDate;
};

#endif
