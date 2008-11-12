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

#ifndef VOIPNETWORKREGISTER_H
#define VOIPNETWORKREGISTER_H

#include <qnetworkregistration.h>
#include <qpresence.h>
#include <qtelephonyconfiguration.h>

#include <QListWidget>

class QModemNetworkRegistration;
class QWaitWidget;

class VoipNetworkRegister : public QListWidget
{
    Q_OBJECT
public:
    VoipNetworkRegister( QWidget *parent = 0 );
    ~VoipNetworkRegister();

protected:
    void showEvent( QShowEvent * );

private slots:
    void operationSelected( QListWidgetItem * );
    void registrationStateChanged();
    void localPresenceChanged();

private:
    QNetworkRegistration *m_client;
    QPresence *m_presence;
    QTelephonyConfiguration *m_config;
    QListWidgetItem *m_regItem, *m_presenceItem, *m_configItem;

    void init();
    inline bool registered() { return m_client->registrationState() == QTelephony::RegistrationHome; }
    inline bool visible() { return m_presence->localPresence() == QPresence::Available; }
    void updateRegistrationState();
    void updatePresenceState();
    void configureVoIP();
};

#endif /* VOIPNETWORKREGISTER_H */
