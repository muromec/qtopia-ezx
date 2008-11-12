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

#ifndef __BTSETTINGS_H__
#define __BTSETTINGS_H__

#include <qcommdevicecontroller.h>
#include <QMainWindow>

class QMenu;
class QTabWidget;
class QBluetoothLocalDevice;

class BTSettingsMainWindow : public QMainWindow
{
    Q_OBJECT
public:
    BTSettingsMainWindow(QWidget *parent = 0, Qt::WFlags fl = 0);
    virtual ~BTSettingsMainWindow();

private slots:
    void init();
    void tabChanged(int index);
    void setTabsEnabled(QCommDeviceController::PowerState state);

private:
    QMenu *m_menu;
    QTabWidget *m_tabs;
    QBluetoothLocalDevice *const m_localDevice;
    QCommDeviceController *m_controller;
};

#endif
