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
#ifndef IPCONFIGIMPL_H
#define IPCONFIGIMPL_H

#include <QWidget>
#include <qtopianetworkinterface.h>
#include <qtopiaglobal.h>

class QCheckBox;
class QLabel;
class QLineEdit;
class QGroupBox;

class QTOPIACOMM_EXPORT IPPage : public QWidget
{
    Q_OBJECT
public:
    explicit IPPage( const QtopiaNetworkProperties& cfg, QWidget* parent = 0, Qt::WFlags flags = 0 );
    virtual ~IPPage();

    QtopiaNetworkProperties properties();
private slots:
    void connectWdgts();

private:
    void init();

    void readConfig( const QtopiaNetworkProperties& prop);

private:
    QCheckBox* autoIp;
    QGroupBox* dhcpGroup;
    QLabel* ipLabel;
    QLineEdit* ipAddress;

    QLabel* dnsLabel1, *dnsLabel2;
    QLineEdit* dnsAddress1, *dnsAddress2;

    QLabel* broadcastLabel;
    QLineEdit* broadcast;
    QLabel* gatewayLabel;
    QLineEdit* gateway;
    QLabel* subnetLabel;
    QLineEdit* subnet;


};
#endif // IPCONFIGIMPL_H

