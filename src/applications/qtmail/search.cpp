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



#include <QDateTime>

#include "search.h"
#include <qsettings.h>

Search::Search()
{
    reset();
}

void Search::reset()
{
    _status = Any;

    _mailbox = "inbox";
    _name = "";
    fromMail = "";
    recipient = "";
    subject = "";
    body = "";
    folder = "";
    fromAccount = "";

    beforeDate = QDate();
    afterDate = QDate();
}

void Search::setMailbox(QString mailbox)
{
    _mailbox = mailbox.toLower();
}

QString Search::mailbox() const
{
    return _mailbox;
}

void Search::setName(QString in)
{
    _name = in;
}

QString Search::name() const
{
    if ( !_name.isEmpty() ) {
        return _name;
    } else {
        return tr("(No name)");
    }
}

// organized after lest expensive search
bool Search::matches(const QMailMessage& in) const
{
    if ( !matchesStatus(in) )
        return false;

    if ( !matchesAccount(in) )
        return false;
    if ( !matchesFolder(in) )
        return false;

    if ( !match(fromMail, in.from().toString() ) )
        return false;

    if ( !matchesTo(in) )
        return false;

    if ( !match(subject, in.subject() ) )
        return false;

    if ( !matchesBeforeDate(in) )
        return false;
    if ( !matchesAfterDate(in) )
        return false;

    if ( !matchesBody(in) )
        return false;

    return true;
}

void Search::setMailFrom(QString from)
{
    fromMail = from;
}

void Search::setMailTo(QString to)
{
    recipient = to;
}

void Search::setMailSubject(QString subject)
{
    this->subject = subject;
}

void Search::setMailBody(QString body)
{
    this->body = body;
}

void Search::setStatus(MailStatus s)
{
    _status = s;
}

uint Search::status() const
{
    return _status;
}

void Search::setBeforeDate(QDate date)
{
    beforeDate = date;
}

void Search::setAfterDate(QDate date)
{
    afterDate = date;
}

void Search::setFromFolder(QString _folder)
{
    folder = _folder;
}

void Search::setFromAccount(QString _fromAccount)
{
    fromAccount = _fromAccount;
}

/*  TODO:  We should swap the mail in to get the full to, cc and bcc
    lists.  Only to.first() are currently cached.  Reading all the mails
    from disk are currently to slow a process though
*/
bool Search::matchesTo(const QMailMessage& mail) const
{
    if ( recipient.isEmpty() )
        return true;

    foreach (const QMailAddress& address, mail.to())
        if ( match( recipient, address.toString() ) )
            return true;

    return false;
}

/*  Allows matching of subsets.  If source is of type A, B, C then
    any instances of A B or C in target will return true.  Eg. you can match
    all mail from: Peter Pan, Wendy, Captain Hook
*/
bool Search::match(const QString &source, const QString &target)
{
    QStringList list = source.split(",");
    if ( list.count() == 0 )
        return true;

    for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it) {
        if ( target.indexOf( (*it).trimmed(), 0, Qt::CaseInsensitive) != -1 )
            return true;
    }

    return false;
}

/*  Reading entire mailbox is very slow on a large mailbox  */
bool Search::matchesBody(const QMailMessage& mail) const
{
    if ( body.isEmpty() )
        return true;

    return match(body, mail.body().data());
}

bool Search::matchesStatus(const QMailMessage& mail) const
{
    switch( _status ) {
        case Any:       return true;
        case Read:      return ( mail.status() & (QMailMessage::Read | QMailMessage::ReadElsewhere) );
        case Unread:    return ( !(mail.status() & (QMailMessage::Read | QMailMessage::ReadElsewhere)) );
        case Replied:   return ( (mail.status() & QMailMessage::Replied) 
                                 || (mail.status() & QMailMessage::RepliedAll) );
    }

    return false;
}

// match against date, if the mails date is not parsed, always return true
bool Search::matchesBeforeDate(const QMailMessage& mail) const
{
    if ( beforeDate.isNull() )
        return true;

    QDateTime dateTime = mail.date().toLocalTime();
    if ( dateTime.isNull() )
        return true;

    if ( beforeDate > dateTime.date() )
        return true;

    return false;
}

// match against date, if the mails date is not parsed, always return true
bool Search::matchesAfterDate(const QMailMessage& mail) const
{
    if ( afterDate.isNull() )
        return true;

    QDateTime dateTime = mail.date().toLocalTime();
    if ( dateTime.isNull() )
        return true;

    if ( afterDate < dateTime.date() )
        return true;

    return false;
}

bool Search::matchesFolder(const QMailMessage& mail) const
{
    if ( folder.isEmpty() )
        return true;

    return ( folder == mail.fromMailbox() );
}

bool Search::matchesAccount(const QMailMessage& mail) const
{
    if ( fromAccount.isEmpty() )
        return true;
    if (fromAccount == mail.fromAccount() )
        return true;

    return false;
}

QString Search::getFrom() const
{
    return fromMail;
}

QString Search::getTo() const
{
    return recipient;
}

QString Search::getSubject() const
{
    return subject;
}

QString Search::getBody() const
{
    return body;
}

QDate Search::getBeforeDate() const
{
    return beforeDate;
}

QDate Search::getAfterDate() const
{
    return afterDate;
}

void Search::readSettings(QSettings* config)
{
    setMailbox( config->value("mailbox", "inbox").toString() );
    setMailFrom( config->value("from").toString().trimmed() );
    setMailTo( config->value("to").toString().trimmed() );
    setMailSubject( config->value("subject").toString().trimmed() );
    setMailBody( config->value("body").toString().trimmed() );
    setStatus( config->value("status", Search::Any).toInt() );
    setName( config->value("name").toString().trimmed() );

    QString strDate = config->value("dateafter").toString();
    if (!strDate.isEmpty()) {
        QMailTimeStamp timeStamp(strDate);
        setAfterDate( timeStamp.toLocalTime().date() );
    }
    strDate = config->value("datebefore").toString();
    if (!strDate.isEmpty()) {
        QMailTimeStamp timeStamp(strDate);
        setBeforeDate( timeStamp.toLocalTime().date() );
    }
}

void Search::saveSettings(QSettings *config)
{
    config->setValue("mailbox", mailbox() );
    config->setValue("name", name() );
    config->setValue("from", getFrom() );
    config->setValue("to", getTo() );
    config->setValue("subject", getSubject() );
    config->setValue("body", getBody() );
    config->setValue("status", status() );
    if ( !getBeforeDate().isNull() ) {
        config->setValue("datebefore", getBeforeDate().toString() );
    } else {
        config->setValue("datebefore", "" );
    }
    if ( !getAfterDate().isNull() ) {
        config->setValue("dateafter", getAfterDate().toString() );
    } else {
        config->setValue("dateafter", "" );
    }
}
