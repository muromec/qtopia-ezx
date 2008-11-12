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

#ifndef ADD_NETWORK_UI_H
#define ADD_NETWORK_UI_H

#include <QDialog>
class QListWidget;
class QLabel;

class AddNetworkUI : public QDialog
{
    Q_OBJECT
public:
    AddNetworkUI(QWidget* parent = 0, Qt::WFlags fl = 0);
    ~AddNetworkUI();

    QString selectedConfig() const;

private:
    void init();

private slots:
    void updateHint();
    void itemSelected();

private:
    QListWidget* list;
    QLabel *hint;
};


#endif
