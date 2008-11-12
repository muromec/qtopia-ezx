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

#include <qdialog.h>
#include <qmap.h>

class AccountList;
class QMailAccount;
class EmailClient;
class StatusDisplay;
class QMenu;
class QListWidget;
class QListWidgetItem;
class QAction;

class AccountSettings : public QDialog
{
    Q_OBJECT
public:
    AccountSettings(AccountList *al, EmailClient *parent, const char *name=0, bool modal=true);

signals:
    void changedAccount(QMailAccount *account);
    void deleteAccount(QMailAccount *account);

public slots:
    void addAccount();

private slots:
    void editAccount();
    void removeAccount();
    void accountSelected(QListWidgetItem*);
    void accountSelected(int idx);
    void accountHighlighted(int idx);
    void displayProgress(uint, uint);

private:
    void populateAccountList();
    void editAccount(QMailAccount *account, bool newAccount=false);

private:
    AccountList *accountList;
    QMap<int,int> listToAccountIdx;
    QListWidget *accountListBox;
    QMenu *context;
    QAction *addAccountAction;
    QAction *removeAccountAction;
    StatusDisplay *statusDisplay;
};

