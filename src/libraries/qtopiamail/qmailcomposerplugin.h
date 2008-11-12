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

#ifndef QMAILCOMPOSERPLUGIN_H
#define QMAILCOMPOSERPLUGIN_H

#include <QString>
#include <QIcon>
#include <qfactoryinterface.h>

#include <qmailmessage.h>
#include <qtopiaglobal.h>

class QMailComposerInterface;

struct QTOPIAMAIL_EXPORT QMailComposerPluginInterface : public QFactoryInterface
{
    virtual bool isSupported( QMailMessage::MessageType type ) const = 0;
    virtual QMailComposerInterface* create( QWidget* parent ) = 0;

    virtual QString key() const = 0;
    virtual QMailMessage::MessageType messageType() const = 0;

    virtual QString name() const = 0;
    virtual QString displayName() const = 0;
    virtual QIcon displayIcon() const = 0;
};

#define QMailComposerPluginInterface_iid "com.trolltech.Qtopia.Qtopiamail.QMailComposerPluginInterface"
Q_DECLARE_INTERFACE(QMailComposerPluginInterface, QMailComposerPluginInterface_iid)

class QTOPIAMAIL_EXPORT QMailComposerPlugin : public QObject, public QMailComposerPluginInterface
{
    Q_OBJECT
    Q_INTERFACES(QMailComposerPluginInterface:QFactoryInterface)

public:
    QMailComposerPlugin();
    ~QMailComposerPlugin();

    virtual QStringList keys() const;

    virtual bool isSupported( QMailMessage::MessageType type ) const;
};

#endif /* QMAILCOMPOSERPLUGIN_H */
