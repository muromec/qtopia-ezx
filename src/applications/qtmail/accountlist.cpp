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

#include "accountlist.h"
#include "account.h"
#include <qsettings.h>
#include <qtopialog.h>

AccountList::AccountList(QObject *parent, const char *name)
    : QObject(parent)
{
    setObjectName( name );
}

QListIterator<QMailAccount*> AccountList::accountIterator()
{
    return ( QListIterator<QMailAccount*>(list) );
}

int AccountList::count()
{
    return list.count();
}

QMailAccount* AccountList::at(int x)
{
    return list.at(x);
};


void AccountList::readAccounts()
{
    QSettings accountconf("Trolltech","qtmail_account");
    QMailAccount *account;

    accountconf.beginGroup( "accountglobal" );
    int count = accountconf.value("accounts", 0).toInt();
#ifndef QTOPIA_NO_SMS
    bool smsExists = false;
#endif
    bool systemExists = false;
#ifndef QTOPIA_NO_MMS
    bool mmsExists = false;
#endif

    for (int x = 0; x < count; x++) {
        account = new QMailAccount();
        accountconf.endGroup();
        accountconf.beginGroup( "account_" + QString::number(x) );
        account->readSettings(&accountconf);
        append(account);

#ifndef QTOPIA_NO_SMS
        if (account->accountType() == QMailAccount::SMS)
            smsExists = true;
#endif
        if (account->accountType() == QMailAccount::System)
            systemExists = true;

#ifndef QTOPIA_NO_MMS
        else if (account->accountType() == QMailAccount::MMS)
            mmsExists = true;
#endif
    }
    accountconf.endGroup();

    static const char *const account_names[] = {
        QT_TRANSLATE_NOOP( "AccountList", "SMS" ),
        QT_TRANSLATE_NOOP( "AccountList", "System" ),
        QT_TRANSLATE_NOOP( "AccountList", "MMS" ),
    };
    Q_UNUSED(account_names);

#ifndef QTOPIA_NO_SMS
    if (!smsExists) {
        account = new QMailAccount();
        account->setAccountType( QMailAccount::SMS );
        account->setAccountName( "SMS" );
        account->setMailServer( "SMS" );
        append(account);
    }
#endif
    if (!systemExists) {
        account = new QMailAccount();
        account->setAccountType( QMailAccount::System );
        account->setAccountName( "System" );
        account->setMailServer( "System" );
        append(account);
    }
#ifndef QTOPIA_NO_MMS
    if (!mmsExists) {
        qLog(Messaging) << "Adding MMS account";
        account = new QMailAccount();
        account->setAccountType( QMailAccount::MMS );
        account->setAccountName( "MMS" );
        account->setMailServer("MMS");
        append(account);
    }
#endif
}

void AccountList::saveAccounts()
{
    QSettings accountconf("Trolltech","qtmail_account");
    accountconf.beginGroup( "accountglobal" );

    accountconf.setValue("accounts", count() );
    int count = 0;
    QListIterator<QMailAccount*> it(list);
    while (it.hasNext()) {
        accountconf.endGroup();
        accountconf.beginGroup( "account_" + QString::number(count) );
        accountconf.remove("");
        it.next()->saveSettings(&accountconf);
        count++;
    }

    // Just in case an account has been deleted, clear it in the conf file
    // Assumes this method is called each time an account is removed
    { accountconf.endGroup(); accountconf.beginGroup( "account_" + QString::number(count) ); };
    accountconf.remove("");

    accountconf.sync();
    accountconf.endGroup();
}

/* As we can manipulate each individual account outside this
   class, this is the only way of seeing whether more than one
   account has been made default (which doesn't make sense) */
uint AccountList::defaultMailServerCount()
{
    uint count = 0;

    QListIterator<QMailAccount*> it(list);
    while (it.hasNext()) {
        if ( it.next()->defaultMailServer() )
            count++;
    }

    return count;
}

QMailAccount* AccountList::defaultMailServer()
{
    QListIterator<QMailAccount*> it(list);
    while (it.hasNext()) {
        QMailAccount *account = it.next();
        if ( account->defaultMailServer() )
            return account;
    }
    return NULL;
}

void AccountList::setDefaultMailServer(QMailAccount *account)
{
    QListIterator<QMailAccount*> it(list);
    while (it.hasNext()) {
        QMailAccount *a = it.next();
        if ( a != account ) {
            a->setDefaultMailServer( false );
        } else {
            a->setDefaultMailServer( true );
        }
    }
}

QStringList AccountList::emailAccounts()
{
    QStringList l;

    QListIterator<QMailAccount*> it(list);
    while (it.hasNext()) {
        QMailAccount *account = it.next();
        if ( !account->emailAddress().isEmpty() ) {
            if( account == defaultMailServer() )
                l.prepend( account->emailAddress() );
            else
                l.append( account->emailAddress() );
        }
    }

    return l;
}

QMailAccount* AccountList::getSmtpRef()
{
    QListIterator<QMailAccount*> it(list);
    while (it.hasNext()) {
        QMailAccount *account = it.next();
        if ( !account->smtpServer().isEmpty() )
            return account;
    }

    return NULL;
}

// gets a user: oystein@pop.mymail.com and returns a reference
// to the correct mailaccount object.  Note: this will return
// any account it can find if the account doesn't exist.  For this
// reason, new code should use "getAccountById" instead and
// check for NULL.
QMailAccount* AccountList::getPopRefByAccount(QString user)
{
    QString thisUser;
    QMailAccount* any=0;
    QListIterator<QMailAccount*> it(list);
    while (it.hasNext()) {
        QMailAccount *a = it.next();
        if ( !a->mailServer().isEmpty() ) {
            if ( !any ) any = a;
            thisUser = a->id();
            if (thisUser == user)
                return a;
        }
    }

    return any;
}

// gets an account that matches a particular id.  NULL if no such account.
QMailAccount* AccountList::getAccountById(QString id)
{
    QListIterator<QMailAccount*> it(list);
    while (it.hasNext()) {
        QMailAccount *a = it.next();
        if ( a->id() == id ) {
            return a;
        }
    }
    return 0;
}

/*  Find the account matching the email address */
QMailAccount* AccountList::getSmtpRefByMail(QString email)
{
    QListIterator<QMailAccount*> it(list);
    while (it.hasNext()) {
        QMailAccount *account = it.next();
        if ( !account->smtpServer().isEmpty() ) {
            if ( account->emailAddress() == email )
                return account;
        }
    }

    return NULL;
}

//scans through the list to retrieve the first encountered username
QString AccountList::getUserName()
{
    QListIterator<QMailAccount*> it(list);
    while (it.hasNext()) {
        QMailAccount *account = it.next();
        if ( !account->userName().isEmpty() )
            return account->userName();
    }

    return "";
}

void AccountList::intervalCheck(QMailAccount *account)
{
    int idCount = 0;
    QListIterator<QMailAccount*> it(list);
    while (it.hasNext()) {
        QMailAccount *a = it.next();
        if ( a == account ) {
            emit checkAccount( idCount );
            break;
        }
        idCount++;
    }
}

void AccountList::append(QMailAccount *a)
{
    list.append(a);
    connect( a, SIGNAL(intervalCheck(QMailAccount*)),
             this, SLOT(intervalCheck(QMailAccount*)) );
};

void AccountList::remove(int pos)
{
    list.removeAt(pos);
    if ( at( pos ) )
        disconnect( at( pos ), SIGNAL(intervalCheck(QMailAccount*)),
                    this, SLOT(intervalCheck(QMailAccount*)) );
};

void AccountList::remove(QMailAccount* const a)
{
    list.removeAll(a);
    disconnect( a, SIGNAL(intervalCheck(QMailAccount*)),
                this, SLOT(intervalCheck(QMailAccount*)) );
};
