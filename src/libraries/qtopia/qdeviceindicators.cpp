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

#include "qdeviceindicators.h"
#include <QValueSpaceItem>
#include <QHash>
#include <QSet>
#include <QStringList>

// declare QDeviceIndicatorsPrivate
class QDeviceIndicatorsPrivate : public QObject
{
Q_OBJECT
public:
    QDeviceIndicatorsPrivate();

signals:
    void indicatorStateChanged(const QString &name,
                               QDeviceIndicators::IndicatorState newState);

private slots:
    void update();

public:
    typedef QHash<QString, QDeviceIndicators::IndicatorState> Indicators;
    Indicators m_states;
    QValueSpaceItem *m_item;
};
Q_GLOBAL_STATIC(QDeviceIndicatorsPrivate, deviceIndicators);

// declare QDeviceIndicatorsPrivate
QDeviceIndicatorsPrivate::QDeviceIndicatorsPrivate()
: QObject(0), m_item(0)
{
    m_item = new QValueSpaceItem("/Hardware/IndicatorLights");
    QObject::connect(m_item, SIGNAL(contentsChanged()), this, SLOT(update()));
    update();
}

void QDeviceIndicatorsPrivate::update()
{
    QStringList subPaths = m_item->subPaths();

    QSet<QString> seen;
    for(int ii = 0; ii < subPaths.count(); ++ii) {
        const QString &subPath = subPaths.at(ii);

        seen.insert(subPath);
        QDeviceIndicators::IndicatorState newState =
            (QDeviceIndicators::IndicatorState)m_item->value(subPath).toInt();

        Indicators::Iterator iter = m_states.find(subPath);
        if(iter == m_states.end()) {
            m_states.insert(subPath, newState);
            emit indicatorStateChanged(subPath, newState);
        } else if(*iter != newState) {
            *iter = newState;
            emit indicatorStateChanged(subPath, newState);
        }
    }

    for(Indicators::Iterator iter = m_states.begin();
        iter != m_states.end(); ++iter) {

        const QString &subPath = iter.key();
        if(seen.contains(subPath))
            continue;

        QDeviceIndicators::IndicatorState newState =
            (QDeviceIndicators::IndicatorState)m_item->value(subPath).toInt();

        if(*iter != newState) {
            *iter = newState;
            emit indicatorStateChanged(subPath, newState);
        }
    }
}

// define QDeviceIndicatorsPrivate

/*!
  \class QDeviceIndicators
  \mainclass
  \brief The QDeviceIndicators class allows applications to query, enable
         and disable indicator lights on a device.

  The QDeviceIndicators class can be used by applications to control visual
  lights on a device.  For example, many devices have an "Email" light that is
  illuminated whenever there are new messages.  The exact names of the
  available indicators is device specific, and can be queried through the
  supportedIndicators() and isIndicatorSupported() methods.

  QDeviceIndicators acts as a thin convenience wrapper around entries in the
  Qtopia Value Space.  The current status on indicators is located under the
  \c {/Hardware/IndicatorLights} path.  For example, the "Email" indicator
  status could be read and set directly through the
  \c {/Hardware/IndicatorLights/Email} item.

  \ingroup hardware
  \sa QDeviceIndicatorsProvider
 */

/*!
  \enum QDeviceIndicators::IndicatorState

  Represents the state of a device indicator.
  \value Off The indicator light is not illuminated.
  \value On The indicator light is illuminated.
 */

/*!
  Create a new QDeviceIndicators instance with the provided \a parent.
 */
QDeviceIndicators::QDeviceIndicators(QObject *parent)
: QObject(parent), d(0)
{
    d = deviceIndicators();
    QObject::connect(d,
                     SIGNAL(indicatorStateChanged(QString,IndicatorState)),
                     this,
                     SIGNAL(indicatorStateChanged(QString,IndicatorState)));
}

/*!
  Destroys the instance.
 */
QDeviceIndicators::~QDeviceIndicators()
{
}

/*!
  Returns the list of all indicators supported by the device.
 */
QStringList QDeviceIndicators::supportedIndicators() const
{
    return d->m_states.keys();
}

/*!
  Returns true if the indicator \a name is supported by the device.
 */
bool QDeviceIndicators::isIndicatorSupported(const QString &name)
{
    return d->m_states.contains(name);
}

/*!
  Returns true if the indicator \a name supports the given \a state.
 */
bool QDeviceIndicators::isIndicatorStateSupported(const QString &name,
                                                  IndicatorState state)
{
    Q_UNUSED(state);
    return isIndicatorSupported(name);
}

/*!
  Returns the current indicator state for \a name.
 */
QDeviceIndicators::IndicatorState
QDeviceIndicators::indicatorState(const QString &name)
{
    QDeviceIndicatorsPrivate::Indicators::ConstIterator iter = d->m_states.find(name);
    if(iter == d->m_states.end())
        return Off;
    else
        return *iter;
}

/*!
  Attempts to change the indicator \a name to the provided \a state.  Setting
  an indicator is an asynchronous event.  If necessary, callers should monitor
  the indicatorStateChanged() signal to determine when the state change occurs.
 */
void QDeviceIndicators::setIndicatorState(const QString &name,
                                          IndicatorState state)
{
    d->m_item->setValue(name, state);
}

/*!
  \fn void QDeviceIndicators::indicatorStateChanged(const QString &name, IndicatorState newState)

  Emitted whenever the indicator \a name changes state.  \a newState will be
  set to the new indicator state.
 */

#include "qdeviceindicators.moc"
