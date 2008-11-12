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

#include "mailbodystore_p.h"
#include <qtopianamespace.h>
#include <QDir>
#include <QDebug>
#include <QTextStream>
#include <QSystemMutex>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include "qmailmessage.h"

MailBodyStore::MailBodyStore()
{
    _storePath = Qtopia::applicationFileName("qtmail", "") + "mail";
    QDir dir(_storePath);
    if(!dir.exists())
        if(!dir.mkpath(_storePath))
            qWarning() << "Unable to create mail file storage directory " << _storePath;
}

MailBodyStore::~MailBodyStore()
{

}

bool MailBodyStore::insert(const QMailMessage& m, QString* fileName)
{
    QString newName = generateUniqueFileName();
    QString newPath = _storePath + "/" + newName;

    int id = (int)::ftok(newPath.toAscii(),0);
    QSystemMutex lock(id, true);

    QFile file(newPath);

    if (!file.open(QIODevice::ReadWrite))
    {
        qWarning("Could not open new mail file %s", qPrintable(newName));
        return false;
    }

    QDataStream out(&file);
    m.toRfc2822(out, QMailMessage::StorageFormat);
    if (out.status() == QDataStream::Ok)
    {
        *fileName = newName;
    }
    else
    {

        qWarning("Could not save mail, removing temporary mail file..");
        //remove the mail file
        if(!QFile::remove(newPath))
            qWarning("Could not remove temporary mail file %s",qPrintable(newName));
        lock.unlock();
        return false;
    }
    lock.unlock();
    return true;
}

bool MailBodyStore::remove(const QString& fileName)
{

    //remove the mail file
    QString path = _storePath + "/" + fileName;

    int id = (int)::ftok(path.toAscii(), 0);
    QSystemMutex lock(id, true);


    bool result = false;

    if(QFile::exists(path))
        result = QFile::remove(path);

    lock.unlock();
    return result;
}

bool MailBodyStore::update(const QString& fileName, const QMailMessage& data)
{
    //backup the old file

    QString backupFile = _storePath + "/" + fileName + ".tmp";
    QString newFile = _storePath + "/" + fileName;

    int id = (int)::ftok(newFile.toAscii(),0);
    QSystemMutex lock(id, true);

    if(!QFile::rename(newFile,backupFile))
    {
        lock.unlock();
        qWarning("Count not create temp backup file %s",qPrintable(backupFile));
        return false;
    }

    //save the new file


    QFile file(newFile);

    if (!file.open(QIODevice::ReadWrite))
    {
        qWarning("Could not open new mail file %s", qPrintable(newFile));
        //rename the old file back
        if(!QFile::rename(backupFile,newFile))
            qWarning() << "Could not restore temp backup file %s",qPrintable(backupFile);
        lock.unlock();
        return false;
    }

    QDataStream out(&file);
    data.toRfc2822(out, QMailMessage::StorageFormat);
    if (out.status() != QDataStream::Ok)
    {
        qWarning("Could not save mail, removing temporary mail file..");
        //remove the mail file
        if(!QFile::remove(newFile))
            qWarning("Could not remove temporary mail file %s",qPrintable(newFile));
        //restore the backup file
        if(!QFile::rename(backupFile,newFile))
            qWarning() << "Could not restore temp backup file %s",qPrintable(backupFile);
        lock.unlock();
        return false;
    }
    //delete the temp mail file

    if(!QFile::remove(backupFile))
        qWarning() << "Could not remove the backup file";

    lock.unlock();

    return true;
}

bool MailBodyStore::load(const QString& fileName, QMailMessage* out) const
{
    QString mailFile = _storePath + "/" + fileName;
    int id = (int)::ftok(mailFile.toAscii(),0);
    QSystemMutex lock(id, true);

    if(!QFile::exists( mailFile ))
    {
        lock.unlock();
        qWarning("Could not load mail file %s, does not exist",qPrintable(mailFile));
        return false;
    }
    *out = QMailMessage::fromRfc2822File( mailFile );

    lock.unlock();

    static const QString smsTag("@sms");
    static const QString fromField("From");

    // Remove sms-origin tag, if present - the SMS client previously appended
    // "@sms" to the from address, which is no longer necesary
    QString sender(out->headerFieldText(fromField));
    if (sender.endsWith(smsTag)) {
        sender.chop(smsTag.length());
        out->setHeaderField(fromField, sender);
    }
    return true;
}

QString MailBodyStore::generateUniqueFileName() const
{
    //format: seconds_epoch.pid.randomchars

    bool exists = true;
    QString filename;

    while(exists)
    {
        qint64 pid = 0;
        pid = getpid();
        filename.sprintf("%ld.%ld.",(unsigned long)time(0),(long)pid);
        filename += randomString(5);
        //check if it exists
        exists = QFile::exists(_storePath + "/" + filename);
    }

    return filename;
}

QString MailBodyStore::randomString(int length) const
{
    if (length <=0 ) return QString();

    QString str;
    str.resize( length );

    int i = 0;
    while (length--){
        int r=qrand() % 62;
        r+=48;
        if (r>57) r+=7;
        if (r>90) r+=6;
        str[i++] =  char(r);
    }
    return str;
}

