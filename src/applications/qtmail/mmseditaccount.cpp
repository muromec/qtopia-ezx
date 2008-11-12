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

#include "mmseditaccount.h"
#include "account.h"
#include <qtopiaapplication.h>
#include <qtopiaservices.h>
#include <QWapAccount>
#include <QLineEdit>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>

MmsEditAccount::MmsEditAccount(QWidget *parent)
    : QDialog(parent)
{
    setObjectName("mms-account");
    setupUi(this);
    connect(networkBtn, SIGNAL(clicked()), this, SLOT(configureNetworks()));
    QtopiaIpcAdaptor* netChannel = new QtopiaIpcAdaptor("QPE/NetworkState", this);
    QtopiaIpcAdaptor::connect(netChannel, MESSAGE(wapChanged()),
            this, SLOT(updateNetwork()));
}

void MmsEditAccount::populateNetwork()
{
    // Find available configs.
    QString path = Qtopia::applicationFileName("Network", "wap");
    QDir configDir(path);
    configDir.mkdir(path);

    QStringList files = configDir.entryList( QStringList("*.conf") );
    QStringList configList;
    foreach( QString item, files ) {
        configList.append( configDir.filePath( item ) );
    }

    // Get default
    QSettings cfg("Trolltech", "Network");
    cfg.beginGroup("WAP");
    QString defaultWap = cfg.value("DefaultAccount").toString();
    cfg.endGroup();
    int defaultConfig = -1;

    // Add to combo
    networkCombo->clear();
    foreach( QString config, configList ) {
        QWapAccount acc( config );
        networkCombo->addItem(QIcon(":icon/netsetup/wap"), acc.name(), config);
        if ( config == defaultWap ) {
            defaultConfig = networkCombo->count()-1;
        }
        if ( config == account->networkConfig() ) {
            networkCombo->setCurrentIndex(networkCombo->count()-1);
        }
    }

    if (networkCombo->currentIndex() == -1 && defaultConfig >= 0)
        networkCombo->setCurrentIndex(defaultConfig);

    if (!networkCombo->count()) {
        networkCombo->addItem(tr("<None configured>", "No network profiles have been configured"));
        networkCombo->setCurrentIndex(0);
    }
}

void MmsEditAccount::setAccount(QMailAccount *in)
{
    account = in;
    populateNetwork();
    autoRetrieve->setChecked(account->autoDownload());
}

void MmsEditAccount::accept()
{
    int currItem = networkCombo->currentIndex();
    if (currItem >= 0 && networkCombo->itemData(currItem).isValid()) {
        account->setNetworkConfig(networkCombo->itemData(currItem).toString());
    } else {
        account->setNetworkConfig(QString());
    }
    account->setAutoDownload(autoRetrieve->isChecked());
    QDialog::accept();
}

void MmsEditAccount::configureNetworks()
{
    QtopiaServiceRequest serv("NetworkSetup", "configureWap()");
    serv.send();
}

void MmsEditAccount::updateNetwork()
{
    populateNetwork();
}

