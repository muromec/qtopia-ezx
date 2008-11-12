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

#include "qmailcomposer.h"

#include <QApplication>
#include <QIcon>
#include <QMap>
#include <QWidget>

#include <qcontent.h>
#include <qtopialog.h>
#include <qpluginmanager.h>

#include "qmailmessage.h"
#include "qmailcomposerplugin.h"

#define PLUGIN_KEY "composers"

// Previously, we used plugins as a general extensibility mechanism for 
// adding composer types.  Now, we will use this mechanism to instead control
// which parts of code are loaded, and when.

// Instead of querying the plugins for their meta-data, we will codify the
// ones we know about.  Unknown plugins can no longer be accessed.

class ComposerPluginDescriptor
{
public:
    ComposerPluginDescriptor(QPluginManager& manager) : pluginManager(manager), pluginInterface(0) {}
    virtual ~ComposerPluginDescriptor() {}

    // Support the interface of QMailComposerPluginInterface
    virtual QString key() const = 0;
    virtual QMailMessage::MessageType messageType() const = 0;

    virtual QString name() const = 0;
    virtual QString displayName() const = 0;
    virtual QIcon displayIcon() const = 0;

    virtual bool isSupported(QMailMessage::MessageType t) const { return ((t == messageType()) || (t == QMailMessage::AnyType)); }

    // Load the plugin if necessary and create a composer object
    virtual QMailComposerInterface* create(QWidget* parent)
    {
        if (!pluginInterface)
        {
            if (QObject* plugin = pluginManager.instance(pluginFilename()))
                pluginInterface = qobject_cast<QMailComposerPluginInterface*>(plugin);
        }

        return pluginInterface ? pluginInterface->create(parent) : 0;
    }

private:
    virtual QString pluginFilename() const
    {
        // The plugin manager knows how to find libraries with this name on each platform
        return key().toLower();
    }

    QPluginManager& pluginManager;
    QMailComposerPluginInterface* pluginInterface;
};

// Describe the plugins we know about
class EmailComposerPluginDescriptor : public ComposerPluginDescriptor
{
public:
    EmailComposerPluginDescriptor(QPluginManager& manager) : ComposerPluginDescriptor(manager) {}

    static QString pluginKey() { return "EmailComposer"; }

    virtual QString key() const { return pluginKey(); }
    virtual QMailMessage::MessageType messageType() const { return QMailMessage::Email; }

    virtual QString name() const { return qApp->translate("EmailComposerPlugin","Email"); }
    virtual QString displayName() const { return qApp->translate("EmailComposerPlugin","Email"); }
    virtual QIcon displayIcon() const { return QIcon(":icon/email"); }
};

class GenericComposerPluginDescriptor : public ComposerPluginDescriptor
{
public:
    GenericComposerPluginDescriptor(QPluginManager& manager) : ComposerPluginDescriptor(manager) {}

    static QString pluginKey() { return "GenericComposer"; }

    virtual QString key() const { return pluginKey(); }
    virtual QMailMessage::MessageType messageType() const { return QMailMessage::Sms; }

    virtual QString name() const { return qApp->translate("GenericComposerPlugin","Text message"); }
    virtual QString displayName() const { return qApp->translate("GenericComposerPlugin","Message"); }
    virtual QIcon displayIcon() const { return QIcon(":icon/txt"); }

    virtual bool isSupported( QMailMessage::MessageType type ) const
    {
        return ((type == QMailMessage::Sms) || 
                (type == static_cast<QMailMessage::MessageType>(QMailMessage::Sms | QMailMessage::Email)) ||
                (type == QMailMessage::AnyType));
    }
};

class MMSComposerPluginDescriptor : public ComposerPluginDescriptor
{
public:
    MMSComposerPluginDescriptor(QPluginManager& manager) : ComposerPluginDescriptor(manager) {}

    static QString pluginKey() { return "MMSComposer"; }

    virtual QString key() const { return pluginKey(); }
    virtual QMailMessage::MessageType messageType() const { return QMailMessage::Mms; }

    virtual QString name() const { return qApp->translate("MMSComposerPlugin","Multimedia message"); }
    virtual QString displayName() const { return qApp->translate("MMSComposerPlugin","MMS"); }
    virtual QIcon displayIcon() const { return QIcon(":icon/multimedia"); }
};

typedef QMap<QString, ComposerPluginDescriptor*> PluginMap;

// Load all the viewer plugins into a map for quicker reference
static PluginMap initMap(QPluginManager& manager)
{
    PluginMap map;

    map.insert(EmailComposerPluginDescriptor::pluginKey(), new EmailComposerPluginDescriptor(manager));
    map.insert(GenericComposerPluginDescriptor::pluginKey(), new GenericComposerPluginDescriptor(manager));
#ifndef QTOPIA_NO_MMS
    map.insert(MMSComposerPluginDescriptor::pluginKey(), new MMSComposerPluginDescriptor(manager));
#endif

    return map;
}

// Return a reference to a map containing all loaded plugin objects
static PluginMap& pluginMap()
{
    static QPluginManager pluginManager(PLUGIN_KEY);
    static PluginMap map(initMap(pluginManager));

    return map;
}

// Return the composer plugin object matching the specified ID
static ComposerPluginDescriptor* mapping(const QString& key)
{
    PluginMap::ConstIterator it;
    if ((it = pluginMap().find(key)) != pluginMap().end())
        return it.value();

    qWarning() << "Failed attempt to map composer:" << key;
    return 0;
}

/*!
    \class QMailComposerInterface
    \mainclass
    \brief The QMailComposerInterface class defines the interface to objects that can compose a mail message.
    \ingroup messaginglibrary

    Qtopia uses the QMailComposerInterface interface for composing mail messages.  A class may implement the 
    QMailComposerInterface interface to compose a mail message format.
    
    The composer class may start composing with no associated message, or it may be provided with an existing
    message to edit, via the \l {QMailComposerInterface::setMessage()}{setMessage()} or 
    \l {QMailComposerInterface::setText()}{setText()} functions.  A client can query whether the composer 
    object is empty with the \l {QMailComposerInterface::isEmpty()}{isEmpty()} function, and extract the 
    composed message with the \l {QMailComposerInterface::message()}{message()} function.  If the 
    message type supports attachments, these can be attached with the 
    \l {QMailComposerInterface::attach()}{attach()} function.  The current state of composition can be cleared
    with the \l {QMailComposerInterface::clear()}{clear()} function.

    The composer object should emit the \l {QMailComposerInterface::contentChanged()}{contentChanged()} signal 
    whenever the composed message changes.

    Each composer class must export metadata describing itself and the messages it is able to compose.  To do
    this, the composer must implement the \l {QMailComposerInterface::key()}{key()},
    \l {QMailComposerInterface::messageType()}{messageType()},
    \l {QMailComposerInterface::name()}{name()},
    \l {QMailComposerInterface::displayName()}{displayName()} and
    \l {QMailComposerInterface::displayIcon()}{displayIcon()} functions.

    Rather than creating objects that implement the QMailComposerInterface directly, clients should create an object
    of an appropriate type by using the QMailComposerFactory class:

    \code
    QString key = QMailComposerFactory::defaultKey( QMailMessage::Email );
    QMailComposerInterface* emailComposer = QMailComposerFactory::create( key, this, "emailComposer" );
    \endcode

    To allow a class to be created through the QMailComposerFactory interface, a plug-in class derived from
    QMailComposerPlugin should be implemented.

    \sa QMailComposerFactory, QMailComposerPlugin
*/

/*!
    \fn bool QMailComposerInterface::isEmpty() const

    Returns true if the composer contains no message content; otherwise returns false.
*/

/*!
    \fn QMailMessage QMailComposerInterface::message() const

    Returns the current content of the composer.
*/

/*!
    \fn void QMailComposerInterface::setMessage(const QMailMessage& mail)

    Sets the content of the composer to \a mail.
*/

/*!
    \fn void QMailComposerInterface::clear()

    Clears any message content contained in the composer.
*/

/*!
    \fn QWidget* QMailComposerInterface::widget() const

    Returns the widget implementing the composer interface.
*/

/*!
    \fn QMailComposerInterface::contentChanged()

    This signal is emitted whenever the composer's content is changed.
*/

/*!
    \fn QMailComposerInterface::finished()

    This signal is emitted when the user has completed editing.
*/

/*!
    Constructs the QMailComposerInterface object with the parent widget \a parent.
*/
QMailComposerInterface::QMailComposerInterface( QWidget *parent )
    : QObject( parent )
{
}

/*! 
    Destructs the QMailComposerInterface object.
*/
QMailComposerInterface::~QMailComposerInterface()
{
}

/*!
    Returns a string identifying the composer.
*/

QString QMailComposerInterface::key() const
{
    QString val = metaObject()->className();
    val.chop(9); // remove trailing "Interface"
    return val;
}

/*!
    Returns the type of message created by the composer.
*/
QMailMessage::MessageType QMailComposerInterface::messageType() const
{
    return mapping(key())->messageType();
}

/*!
    Returns the translated name of the message type created by the composer.
*/
QString QMailComposerInterface::name() const
{
    return mapping(key())->name();
}

/*!
    Returns the translated name of the message type created by the composer, in a form suitable
    for display on a button or menu.
*/
QString QMailComposerInterface::displayName() const
{
    return mapping(key())->displayName();
}

/*!
    Returns the icon representing the message type created by the composer.
*/
QIcon QMailComposerInterface::displayIcon() const
{
    return mapping(key())->displayIcon();
}

/*!
    Sets the message to contain \a text, if that is meaningful to the message type created by the composer.
    The text has the mime-type \a type.
*/
void QMailComposerInterface::setText( const QString& text, const QString &type )
{
    // default implementation does nothing
    Q_UNUSED(text)
    Q_UNUSED(type)
}

/*!
    Adds \a item as an attachment to the message in the composer. The \a action parameter
    specifies what the composer should do with \a item.
*/
void QMailComposerInterface::attach( const QContent& item, QMailMessage::AttachmentsAction action )
{
    // default implementation does nothing
    Q_UNUSED(item)
    Q_UNUSED(action)
}

/*!
    Sets the composer to append \a signature to the body of the message, when creating a message.
*/
void QMailComposerInterface::setSignature( const QString& signature )
{
    // default implementation does nothing
    Q_UNUSED(signature)
}

/*!
    Allows the composer object to add any relevant actions to the application \a menu supplied.
*/
void QMailComposerInterface::addActions( QMenu* menu ) const
{
    // default implementation does nothing
    Q_UNUSED(menu)
}

/*!
    \class QMailComposerFactory
    \mainclass
    \brief The QMailComposerFactory class creates objects implementing the QMailComposerInterface interface.
    \ingroup messaginglibrary

    The QMailComposerFactory class creates objects that are able to compose mail messages, and 
    that implement the QMailComposerInterface interface.  The factory chooses an implementation
    based on the type of message to be composed.

    The QMailComposerInterface class describes the interface supported by classes that can be created
    by the QMailComposerFactory class.  To create a new class that can be created via the QMailComposerFactory,
    implement a plug-in that derives from QMailComposerPlugin.

    \sa QMailComposerInterface, QMailComposerPlugin
*/

/*!
    Returns a list of keys identifying classes that can create a message containing \a type content.
*/
QStringList QMailComposerFactory::keys( QMailMessage::MessageType type )
{
    QStringList in;

    foreach (PluginMap::mapped_type plugin, pluginMap())
        if (plugin->isSupported(type))
            in << plugin->key();

    return in;
}

/*!
    Returns the key identifying the first class found that can create a message containing \a type content.
*/
QString QMailComposerFactory::defaultKey( QMailMessage::MessageType type )
{
    QStringList list(QMailComposerFactory::keys(type));
    return (list.isEmpty() ? QString() : list.first());
}

/*!
    Returns the message type for the composer identified by \a key.

    \sa QMailComposerInterface::messageType()
*/
QMailMessage::MessageType QMailComposerFactory::messageType( const QString& key )
{
    return mapping(key)->messageType();
}

/*!
    Returns the name for the composer identified by \a key.

    \sa QMailComposerInterface::name()
*/
QString QMailComposerFactory::name( const QString& key )
{
    return mapping(key)->name();
}

/*!
    Returns the display name for the composer identified by \a key.

    \sa QMailComposerInterface::displayName()
*/
QString QMailComposerFactory::displayName( const QString& key )
{
    return mapping(key)->displayName();
}

/*!
    Returns the display icon for the composer identified by \a key.

    \sa QMailComposerInterface::displayIcon()
*/
QIcon QMailComposerFactory::displayIcon( const QString& key )
{
    return mapping(key)->displayIcon();
}

/*!
    Creates a composer object of the class identified by \a key, setting the returned object to 
    have the parent widget \a parent.
*/
QMailComposerInterface *QMailComposerFactory::create( const QString& key, QWidget *parent )
{
    return mapping(key)->create(parent);
}

