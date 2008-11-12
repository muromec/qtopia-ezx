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

#include "qmailviewer.h"

#include <QApplication>
#include <QIcon>
#include <QMap>
#include <QWidget>

#include <qtopialog.h>
#include <qpluginmanager.h>

#include "qmailviewerplugin.h"

#define PLUGIN_KEY "viewers"

// Previously, we used plugins as a general extensibility mechanism for 
// adding viewer types.  Now, we will use this mechanism to instead control
// which parts of code are loaded, and when.

// Instead of querying the plugins for their meta-data, we will codify the
// ones we know about.  Unknown plugins can no longer be accessed.

class ViewerPluginDescriptor
{
public:
    ViewerPluginDescriptor(QPluginManager& manager) : pluginManager(manager), pluginInterface(0) {}
    virtual ~ViewerPluginDescriptor() {}

    // Support the interface of QMailViewerPluginInterface
    virtual QString key() const = 0;
    virtual int type() const = 0;

    bool isSupported(int t) const { return (t == QMailViewerFactory::AnyContent || t == type()); }

    // Load the plugin if necessary and create a viewer object
    virtual QMailViewerInterface* create(QWidget* parent)
    {
        if (!pluginInterface)
        {
            if (QObject* plugin = pluginManager.instance(pluginFilename()))
                pluginInterface = qobject_cast<QMailViewerPluginInterface*>(plugin);
        }

        return pluginInterface ? pluginInterface->create(parent) : 0;
    }

private:
    virtual QString pluginFilename() const
    {
        return key().toLower();
    }

    QPluginManager& pluginManager;
    QMailViewerPluginInterface* pluginInterface;
};

// Describe the plugins we know about
class GenericViewerPluginDescriptor : public ViewerPluginDescriptor
{
public:
    GenericViewerPluginDescriptor(QPluginManager& manager) : ViewerPluginDescriptor(manager) {}

    static QString pluginKey() { return "GenericViewer"; }

    virtual QString key() const { return pluginKey(); }
    virtual int type() const { return QMailViewerFactory::StaticContent; }
};

class SmilViewerPluginDescriptor : public ViewerPluginDescriptor
{
public:
    SmilViewerPluginDescriptor(QPluginManager& manager) : ViewerPluginDescriptor(manager) {}

    static QString pluginKey() { return "SmilViewer"; }

    virtual QString key() const { return pluginKey(); }
    virtual int type() const { return QMailViewerFactory::SmilContent; }
};

typedef QMap<QString, ViewerPluginDescriptor*> PluginMap;

// Load all the viewer plugins into a map for quicker reference
static PluginMap initMap(QPluginManager& manager)
{
    PluginMap map;

    map.insert(GenericViewerPluginDescriptor::pluginKey(), new GenericViewerPluginDescriptor(manager));
#ifndef QTOPIA_NO_MMS
    map.insert(SmilViewerPluginDescriptor::pluginKey(), new SmilViewerPluginDescriptor(manager));
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

// Return the viewer plugin object matching the specified ID
static ViewerPluginDescriptor* mapping(const QString& key)
{
    PluginMap::ConstIterator it;
    if ((it = pluginMap().find(key)) != pluginMap().end())
        return it.value();

    qWarning() << "Failed attempt to map viewer:" << key;
    return 0;
}

/*!
    \class QMailViewerInterface
    \mainclass
    \brief The QMailViewerInterface class defines the interface to objects that can display a mail message.
    \ingroup messaginglibrary

    Qtopia uses the QMailViewerInterface interface for displaying mail messages.  A class may implement the 
    QMailViewerInterface interface to display a mail message format.
    
    The message to be displayed is provided to the viewer class using the \l {QMailViewerInterface::setMessage()}
    {setMessage()} function.  If the message refers to external resources, these should be provided using the 
    \l {QMailViewerInterface::setResource()}{setResource()} function.  The  \l {QMailViewerInterface::clear()}{clear()}
    function clears any message or resources previously set.

    The viewer object should emit the \l {QMailViewerInterface::anchorClicked()}{anchorClicked()} signal if the user 
    selects a link in the message.  If the message supports a concept of completion, then the 
    \l {QMailViewerInterface::finished()}{finished()} signal should be emitted after the display has been completed.

    Rather than creating objects that implement the QMailViewerInterface directly, clients should create an object
    of an appropriate type by using the QMailViewerFactory class:

    \code
    QString key = QMailViewerFactory::defaultKey( QMailViewerFactory::SmilContent );
    QMailViewerInterface* smilViewer = QMailViewerFactory::create( key, this, "smilViewer" );
    \endcode

    To allow a class to be created through the QMailViewerFactory interface, a plug-in class derived from
    QMailViewerPlugin should be implemented.

    \sa QMailViewerFactory, QMailViewerPlugin
*/

/*!
    \fn bool QMailViewerInterface::setMessage(const QMailMessage& mail)

    Displays the contents of \a mail.  Returns whether the message could be successfully displayed.
*/

/*!
    \fn void QMailViewerInterface::clear()

    Resets the display to have no content, and removes any resource associations.
*/

/*!
    \fn QWidget* QMailViewerInterface::widget() const

    Returns the widget implementing the display interface.
*/

/*!
    \fn QMailViewerInterface::anchorClicked(const QUrl& link)

    This signal is emitted when the user presses the select key while the display has the 
    anchor \a link selected.
*/

/*!
    \fn QMailViewerInterface::finished()

    This signal is emitted when the display of the current mail message is completed.  This signal 
    is emitted only for message types that define a concept of completion, such as SMIL slideshows.
*/

/*!
    Constructs the QMailViewerInterface object with the parent widget \a parent.
*/
QMailViewerInterface::QMailViewerInterface( QWidget *parent )
    : QObject( parent )
{
}

/*! 
    Destructs the QMailViewerInterface object.
*/
QMailViewerInterface::~QMailViewerInterface()
{
}

/*!
    Scrolls the display to position the \a link within the viewable area.
*/
void QMailViewerInterface::scrollToAnchor(const QString& link)
{
    // default implementation does nothing
    Q_UNUSED(link)
}

/*!
    Allows the viewer object to add any relevant actions to the application \a menu supplied.
*/
void QMailViewerInterface::addActions( QMenu* menu ) const
{
    // default implementation does nothing
    Q_UNUSED(menu)
}

/*! 
    Supplies the viewer object with a resource that may be referenced by a mail message.  The resource 
    identified as \a name will be displayed as the object \a value.  
*/
void QMailViewerInterface::setResource(const QUrl& name, QVariant value)
{
    // default implementation does nothing
    Q_UNUSED(name)
    Q_UNUSED(value)
}


/*!
    \class QMailViewerFactory
    \mainclass
    \brief The QMailViewerFactory class creates objects implementing the QMailViewerInterface interface.
    \ingroup messaginglibrary

    The QMailViewerFactory class creates objects that are able to display mail messages, and 
    implement the \c QMailViewerInterface interface.  The factory chooses an implementation
    based on the type of message to be displayed.

    The QMailViewerInterface class describes the interface supported by classes that can be created
    by the QMailViewerFactory class.  To create a new class that can be created via the QMailViewerFactory,
    implement a plug-in that derives from QMailViewerPlugin.

    \sa QMailViewerInterface, QMailViewerPlugin
*/

/*!
    \enum QMailViewerFactory::ContentType

    This enum defines the types of mail content that viewer objects may be able to display.

    \value StaticContent Mail content that is not animated, such as plain text and HTML.
    \value SmilContent Content marked up via Synchronized Multimedia Integration Language.
    \value AnyContent Do not specify the type of content to be viewed.
*/

/*!
    Returns a list of keys identifying classes that can display a message containing \a type content.

    \sa QMailViewerFactory::ContentType
*/
QStringList QMailViewerFactory::keys( ContentType type )
{
    QStringList in;

    foreach (PluginMap::mapped_type plugin, pluginMap())
        if (plugin->isSupported(type))
            in << plugin->key();

    return in;
}

/*!
    Returns the key identifying the first class found that can display message containing \a type content.

    \sa QMailViewerFactory::ContentType
*/
QString QMailViewerFactory::defaultKey( ContentType type )
{
    QStringList list(QMailViewerFactory::keys(type));
    return (list.isEmpty() ? QString() : list.first());
}

/*!
    Creates a viewer object of the class identified by \a key, setting the returned object to 
    have the parent widget \a parent.
*/
QMailViewerInterface *QMailViewerFactory::create( const QString &key, QWidget *parent )
{
    return mapping(key)->create(parent);
}

