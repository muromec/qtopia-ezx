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
#ifndef SELECTFOLDER_H
#define SELECTFOLDER_H

#include <qdialog.h>
#include <qstring.h>
#include <qstringlist.h>

class QListWidget;

class SelectFolderDialog : public QDialog
{
    Q_OBJECT

public:
    SelectFolderDialog(const QStringList list,
                       QWidget *parent = 0);
    virtual ~SelectFolderDialog();
    int folder();
    void getFolders();

private slots:
    void selected();
private:
    QListWidget *mFolderList;
    QStringList mMailboxList;
};

#endif
