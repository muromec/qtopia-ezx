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

#ifndef MMSEDITACCOUNT_H
#define MMSEDITACCOUNT_H

#include "ui_mmseditaccountbase.h"
#include <QDialog>

class QMailAccount;

class MmsEditAccount : public QDialog, Ui_MmsEditAccountBase
{
    Q_OBJECT
public:
    MmsEditAccount(QWidget *parent=0);

    void setAccount(QMailAccount *in);

protected slots:
    void accept();
    void configureNetworks();
    void updateNetwork();

private:
    void populateNetwork();

private:
    QMailAccount *account;
};

#endif
