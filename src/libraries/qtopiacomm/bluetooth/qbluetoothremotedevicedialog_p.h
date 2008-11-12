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
#ifndef __QBLUETOOTHREMOTEDEVICEDIALOG_P_H__
#define __QBLUETOOTHREMOTEDEVICEDIALOG_P_H__

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qtopia API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qbluetoothremotedevicedialog.h"
#include <qbluetoothsdpquery.h>
#include <qbluetoothlocaldevice.h>

#include <QList>
#include <QIcon>

class QBluetoothRemoteDeviceSelector;
class QLabel;
class QWaitWidget;
class QBluetoothSdpQueryResult;
class QActionGroup;
class QMenu;
class QTableWidgetItem;


class DiscoveryStatusIcon : public QObject
{
    Q_OBJECT
public:
    enum State
    {
        Active,
        Inactive,
        Disabled
    };

    DiscoveryStatusIcon(QObject *parent = 0, int blinkInterval = 300);
    ~DiscoveryStatusIcon();

    void setState(State state);
    QLabel *iconLabel() const;

private slots:
    void toggleIconImage();

private:
    QLabel *m_iconLabel;
    QTimer *m_timer;
    QPixmap m_pixmapOnline;
    QPixmap m_pixmapOffline;
};


class QBluetoothRemoteDeviceDialogPrivate : public QWidget
{
    Q_OBJECT

public:
    QBluetoothRemoteDeviceDialogPrivate(QBluetoothLocalDevice *local, QBluetoothRemoteDeviceDialog *parent);
    ~QBluetoothRemoteDeviceDialogPrivate();

    QBluetoothAddress selectedDevice() const;

    void addDeviceAction(QAction *action);
    void removeDeviceAction(QAction *action);

    void cleanUp();
    QSet<QBluetooth::SDPProfile> m_validProfiles;
    QBluetoothRemoteDeviceDialogFilter *m_filter;

protected:
    void showEvent(QShowEvent *event);

private slots:
    void deviceSelectionChanged();
    void activated(const QBluetoothAddress &addr);

    void triggeredDiscoveryAction();
    void discoveryCompleted();
    void discoveredDevice(const QBluetoothRemoteDevice &device);
    void reallyStartDiscovery();

    void validateProfiles();

    void serviceSearchCompleted(const QBluetoothSdpQueryResult &result);
    void serviceSearchCancelled();
    void serviceSearchCancelCompleted();

private:
    void startDiscovery();
    bool cancelDiscovery();

    void deviceActivatedOk();

    void validationError();
    bool serviceProfilesMatch(const QList<QBluetoothSdpRecord> services);

    void setDeviceActionsEnabled(bool enabled);
    void addDeviceToDisplay(const QBluetoothRemoteDevice &device);

    void initWidgets();
    void initLayout();
    void initActions();

    const QString TEXT_DISCOVERY_CANCEL;

    QBluetoothRemoteDeviceDialog *m_parent;
    QPointer<QBluetoothLocalDevice> m_local;
    bool m_triedAutoDiscovery;

    // device discovery
    bool m_discovering;
    bool m_cancellingDiscovery;
    int m_discoveryAttempts;

    // service search
    QBluetoothSdpQuery m_sdap;
    QBluetoothAddress m_deviceUnderValidation;
    bool m_cancellingServiceSearch;

    // gui
    QBluetoothRemoteDeviceSelector *m_browser;
    QLabel *m_statusLabel;
    DiscoveryStatusIcon *m_statusIcon;
    QWaitWidget *m_validationWaitWidget;

    // actions
    QMenu *m_menu;
    QAction *m_discoveryAction;
    QList<QAction*> m_deviceActions;
    QAction *m_deviceActionsSeparator;

    // action icons
    QIcon m_discoveryStartIcon;
    QIcon m_discoveryCancelIcon;
};

#endif
