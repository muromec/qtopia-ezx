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

#include <QMap>
#include <QTimer>

#include "gstreamerbushelper.h"


namespace gstreamer
{

class BusHelperPrivate : public QObject
{
    Q_OBJECT
    typedef QMap<BusHelper*, GstBus*>   HelperMap;

public:
    void addBusWatch(GstBus* bus, BusHelper* helper)
    {
        m_helperMap.insert(helper, bus);

        if (m_helperMap.size() == 1)
        {
            m_intervalTimer->start();
        }
    }

    void removeBusWatch(BusHelper* helper)
    {
        m_helperMap.remove(helper);

        if (m_helperMap.size() == 0)
        {
            m_intervalTimer->stop();
        }
    }

    static BusHelperPrivate* instance()
    {
        static BusHelperPrivate    self;

        return &self;
    }

private slots:
    void interval()
    {
        for (HelperMap::iterator it = m_helperMap.begin(); it != m_helperMap.end(); ++it)
        {
            GstMessage* message;

            while ((message = gst_bus_poll(it.value(), GST_MESSAGE_ANY, 0)) != 0)
            {
                it.key()->message(message);
                gst_message_unref(message);
            }

            it.key()->message(Message());
        }
    }

private:
    BusHelperPrivate()
    {
        m_intervalTimer = new QTimer(this);
        m_intervalTimer->setInterval(1000);

        connect(m_intervalTimer, SIGNAL(timeout()), this, SLOT(interval()));
    }

    HelperMap   m_helperMap;
    QTimer*     m_intervalTimer;
};

/*!
    \class gstreamer::BusHelper
    \internal
*/

BusHelper::BusHelper(GstBus* bus, QObject* parent):
    QObject(parent)
{
    d = BusHelperPrivate::instance();

    d->addBusWatch(bus, this);
}

BusHelper::~BusHelper()
{
    d->removeBusWatch(this);
}

}   // ns gstreamer

#include "gstreamerbushelper.moc"

