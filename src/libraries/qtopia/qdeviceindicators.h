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

#ifndef _QDEVICEINDICATORS_H_
#define _QDEVICEINDICATORS_H_

#include <QObject>
#include <qtopiaglobal.h>

class QDeviceIndicatorsPrivate;
class QTOPIA_EXPORT QDeviceIndicators : public QObject
{
Q_OBJECT
public:
    explicit QDeviceIndicators(QObject *parent = 0);
    virtual ~QDeviceIndicators();

    enum IndicatorState { Off, On };

    bool isIndicatorSupported(const QString &);
    bool isIndicatorStateSupported(const QString &, IndicatorState);

    IndicatorState indicatorState(const QString &);
    void setIndicatorState(const QString &name, IndicatorState state);
    QStringList supportedIndicators() const;

signals:
    void indicatorStateChanged(const QString &name, IndicatorState newState);

private:
    QDeviceIndicatorsPrivate *d;
};

#endif // _QDEVICELEDS_H_
