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
#ifndef LANCONFIGIMPL_H
#define LANCONFIGIMPL_H

#include <qtopianetworkinterface.h>
#include <qtranslatablesettings.h>

#include <QDialog>
class LanUI;

class LANConfig : public QtopiaNetworkConfiguration
{
public:
    LANConfig( const QString& confFile );
    virtual ~LANConfig();

    virtual QString configFile() const
    {
        return currentConfig;
    }

    QStringList types() const;

    virtual QVariant property( const QString& key ) const;

    virtual QDialog* configure( QWidget* parent, const QString& type = QString() );
    virtual QtopiaNetworkProperties getProperties() const;
    virtual void writeProperties( const QtopiaNetworkProperties& properties );

private:
    QString currentConfig;
    mutable QTranslatableSettings cfg;
};

#endif
