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

#include "qmailcomposerplugin.h"

#include <QApplication>
#include <qtopialog.h>

/*!
    \class QMailComposerPluginInterface
    \mainclass
    \brief The QMailComposerPluginInterface class defines the interface to plug-ins that provide mail message composers.
    \ingroup messaginglibrary

    The QMailComposerPluginInterface class defines the interface to mail message composer plug-ins.  Plug-ins will 
    typically inherit from QMailComposerPlugin rather than this class.

    \sa QMailComposerPlugin, QMailComposerInterface, QMailComposerFactory
*/

/*!
    \fn QString QMailComposerPluginInterface::key() const

    Returns a string identifying this plug-in.
*/

/*!
    \fn QMailMessage::MessageType QMailComposerPluginInterface::messageType() const

    Returns the type of message created by the composer provided by this plug-in.
*/

/*!
    \fn QString QMailComposerPluginInterface::name() const

    Returns the translated name of the message type created by the composer provided by this plug-in.
*/

/*!
    \fn QString QMailComposerPluginInterface::displayName() const

    Returns the translated name of the message type created by the composer provided by this plug-in, in a form suitable
    for display on a button or menu.
*/

/*!
    \fn QIcon QMailComposerPluginInterface::displayIcon() const

    Returns the icon representing the message type created by the composer provided by this plug-in.
*/

/*!
    \fn bool QMailComposerPluginInterface::isSupported( QMailMessage::MessageType type ) const

    Returns true if the composer provided by this plug-in can create a mail message with \a type content; otherwise returns false.
*/

/*!
    \fn QMailComposerInterface* QMailComposerPluginInterface::create( QWidget* parent )

    Creates an instance of the message composer provided by this plug-in, setting the returned object to 
    have the parent widget \a parent.
*/

/*!
    \class QMailComposerPlugin
    \mainclass
    \brief The QMailComposerPlugin class defines a base class for implementing mail message composer plug-ins.
    \ingroup messaginglibrary

    The QMailComposerPlugin class provides a base class for plug-in classes that provide mail message composing
    functionality.  Classes that inherit QMailComposerPlugin need to provide overrides of the
    \l {QMailComposerPlugin::key()}{key()}, 
    \l {QMailComposerPlugin::messageType()}{messageType()}, 
    \l {QMailComposerPlugin::name()}{name()}, 
    \l {QMailComposerPlugin::displayName()}{displayName()}, 
    \l {QMailComposerPlugin::displayIcon()}{displayIcon()} and 
    \l {QMailComposerPlugin::create()}{create()} member functions.

    \sa QMailComposerPluginInterface, QMailComposerInterface
*/

/*!
    Create a mail message viewer plug-in instance.
*/
QMailComposerPlugin::QMailComposerPlugin()
{
}

/*!
    Destructs the QMailComposerPlugin object.
*/
QMailComposerPlugin::~QMailComposerPlugin()
{
}

/*!
    Returns the list of interfaces implemented by this plug-in.
*/
QStringList QMailComposerPlugin::keys() const
{
    QStringList list;
    return list << "QMailComposerPluginInterface";
}

/*!
    Returns true if the composer provided by this plug-in can compose a mail message with \a type content; otherwise returns false.
*/
bool QMailComposerPlugin::isSupported( QMailMessage::MessageType type ) const
{
    return ((type == QMailMessage::AnyType) || (type == this->messageType()));
}
