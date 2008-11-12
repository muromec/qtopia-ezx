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



#ifndef FOLDER_H
#define FOLDER_H

#include <qstring.h>
#include <qobject.h>

#include <qtopiaglobal.h>
#include <qsoftmenubar.h>

class Search;
class QMailMessage;

const int FolderTypeSystem = 1;
const int FolderTypeSearch = 2;
const int FolderTypeAccount = 3;
const int FolderTypeMailbox = 4;

const int SystemTypeMailbox = 1;
const int SystemTypeSearch = 2;

class QTOPIAMAIL_EXPORT Folder : public QObject
{
    Q_OBJECT

public:
    Folder(int type);
    virtual ~Folder();

    int folderType() const { return _folderType; };

    virtual bool matchesEmail(const QMailMessage& message) const;
    virtual QString mailbox() const;
    virtual QString name() const;
    virtual QString fullName() const;
    virtual QString displayName() const;
    virtual void setName(QString str);
    virtual QSoftMenuBar::StandardLabel menuLabel() const;

private:
    int _folderType;
    QString _name;
};


class QTOPIAMAIL_EXPORT SystemFolder : public Folder
{
    Q_OBJECT
public:
    SystemFolder(int systemType, const QString &mailbox);
    bool matchesEmail(const QMailMessage& message) const;
    QString mailbox() const;
    bool isSearch() const;
    int systemType() const;
    QString name() const;
    QString fullName() const { return name(); };

    void setSearch(Search *in);
    Search *search() const;
private:
    int _systemType;
    QString _mailbox;
    Search *_search;
};

class QTOPIAMAIL_EXPORT SearchFolder : public Folder
{
    Q_OBJECT
public:
    SearchFolder(Search *in);
    ~SearchFolder();

    void setSearch(Search *in);
    Search *search() const;

    bool matchesEmail(const QMailMessage& message) const;
    QString mailbox() const;
    QString name() const;
    QString fullName() const { return name(); };

private:
    Search *_search;
};

#endif
