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

#ifndef _DEVICELEDS_H_
#define _DEVICELEDS_H_

#include <QObject>
#include <QDeviceIndicators>

class QDeviceIndicatorsProviderPrivate;
class QDeviceIndicatorsProvider : public QObject
{
Q_OBJECT
public:
    QDeviceIndicatorsProvider(QObject *parent = 0);

protected:
    void setSupportedIndicators(const QStringList &);
    void setIndicatorState(const QString &, QDeviceIndicators::IndicatorState);

    virtual void changeIndicatorState(const QString &,
                                      QDeviceIndicators::IndicatorState) = 0;

private slots:
    void itemSetValue(const QString &attribute, const QVariant &data);

private:
    QDeviceIndicatorsProviderPrivate *d;
};

#endif // _DEVICELEDS_H_
