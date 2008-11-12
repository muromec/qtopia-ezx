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

#ifndef ADVANCEDCONFIG_H
#define ADVANCEDCONFIG_H

#include <QWidget>
#include <qtopianetworkinterface.h>

#include "ui_advancedbase.h"

class AdvancedPage : public QWidget
{
    Q_OBJECT
public:
    AdvancedPage( const QtopiaNetworkProperties& cfg, QWidget* parent = 0, Qt::WFlags flags = 0 );
    virtual ~AdvancedPage();

    QtopiaNetworkProperties properties();

protected:
    bool eventFilter( QObject*, QEvent* );
private slots:
    void externalState( int state );

private:
    void init();
    void readConfig( const QtopiaNetworkProperties& prop);
    void selectCombo( QComboBox* combo, const QString& item );

    bool GPRS;

private:
    Ui::AdvancedBase ui;

};
#endif // ADVANCEDCONFIG_H

