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

#include <gst/gst.h>

#include "gstreamermessage.h"


static int wuchi = qRegisterMetaType<gstreamer::Message>();

namespace gstreamer
{

/*!
    \class gstreamer::Message
    \internal
*/

Message::Message():
    m_message(0)
{
}

Message::Message(GstMessage* message):
    m_message(message)
{
    gst_message_ref(m_message);
}

Message::~Message()
{
    if (m_message != 0)
        gst_message_unref(m_message);
}

GstMessage* Message::rawMessage() const
{
    return m_message;
}


}   // ns gstreamer

