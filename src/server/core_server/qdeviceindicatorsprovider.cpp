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

#include "qdeviceindicatorsprovider.h"
#include <QtopiaChannel>
#include <QStringList>
#include <QValueSpaceObject>
#include <QSet>
#include <QDataStream>

// declare QDeviceIndicatorsProviderPrivate
class QDeviceIndicatorsProviderPrivate
{
public:
    QValueSpaceObject *m_provider;
    QStringList m_supported;

    static QSet<QString> providedIndicators;
};
QSet<QString> QDeviceIndicatorsProviderPrivate::providedIndicators;

/*!
  \class QDeviceIndicatorsProvider
  \ingroup QtopiaServer
  \brief The QDeviceIndicatorsProvider class provides the backend for the QDeviceIndicator API.

  QDeviceIndicatorsProvider derived types control the status of device
  indicators.  During construction, derived types usually call
  setSupportedIndicators() to set the indicators they provide. Derived classes
  must implement the changeIndicatorState() function which performs the actual hardware
  operation involved.

  This class is part of the Qtopia server and a specific implementation should be provided
  as part of a server task.

  \sa QDeviceIndicators

 */

/*!
  Create a new QDeviceIndicatorsProvider instance with the specified \a parent.
 */
QDeviceIndicatorsProvider::QDeviceIndicatorsProvider(QObject *parent)
: QObject(parent), d(new QDeviceIndicatorsProviderPrivate)
{
    d->m_provider = new QValueSpaceObject("/Hardware/IndicatorLights", this);
    QObject::connect(d->m_provider, SIGNAL(itemSetValue(QString,QVariant)),
                     this, SLOT(itemSetValue(QString,QVariant)));
}

/*! \internal */
void QDeviceIndicatorsProvider::itemSetValue(const QString &attribute,
                                           const QVariant &data)
{
    for(int ii = 0; ii < d->m_supported.count(); ++ii) {
        if(attribute == d->m_supported.at(ii)) {
            int val = data.toInt();
            if(val >= 0 && val <= QDeviceIndicators::On) {
                QDeviceIndicators::IndicatorState status =
                    (QDeviceIndicators::IndicatorState)val;
                changeIndicatorState(attribute, status);
                d->m_provider->setAttribute(attribute, val);
            }
            return;
        }
    }
    // Not our indicator
}

/*!
  \fn void QDeviceIndicatorsProvider::setSupportedIndicators(const QStringList &indicators)

  Set the \a indicators that this QDeviceIndicatorsProvider instance will
  provide.  changeIndicatorState() callbacks will only occur for indicators set
  in this way.  The class will call qFatal() if an indicator exists in the
  \a indicators list that is already being provided by another
  QDeviceIndicatorsProvider instance.

  The initial state of the \a indicators is QDeviceIndicators::Off.
 */
void QDeviceIndicatorsProvider::setSupportedIndicators(const QStringList &in)
{
    for(int ii = 0; ii < d->m_supported.count(); ++ii) {
        QDeviceIndicatorsProviderPrivate::providedIndicators.remove(d->m_supported.at(ii));
    }
    d->m_provider->removeAttribute(QByteArray());

    for(int ii = 0; ii < in.count(); ++ii) {
        if(QDeviceIndicatorsProviderPrivate::providedIndicators.contains(in.at(ii))) {
            qFatal("QDeviceIndicatorsProvider: Indicator '%s' already supported.", in.at(ii).toLatin1().constData());
        }
    }

    for(int ii = 0; ii < in.count(); ++ii) {
        QDeviceIndicatorsProviderPrivate::providedIndicators.insert(in.at(ii));
        d->m_provider->setAttribute(in.at(ii), (int)QDeviceIndicators::Off);
    }

    d->m_supported = in;
}

/*!
  Set the published \a indicator state to \a state.  This is generally called
  from within the changeIndicatorState() callback to indicate that the state was
  successfully  changes or at startup to set the initial state, but it can be
  called at any time.
 */
void QDeviceIndicatorsProvider::setIndicatorState(const QString &indicator, QDeviceIndicators::IndicatorState state)
{
    for(int ii = 0; ii < d->m_supported.count(); ++ii) {
        if(d->m_supported.at(ii) == indicator) {
            d->m_provider->setAttribute(d->m_supported.at(ii), (int)state);
        }
    }
}

/*!
    \fn void QDeviceIndicatorsProvider::changeIndicatorState(const QString &indicator, QDeviceIndicators::IndicatorState state) = 0

    Called when the \a indicator should be set to \a state.  This is usually
    in response to a QDeviceIndicators::setIndicatorState() call. Subclasses should implement 
    hardware specific operation in this function.
 */
