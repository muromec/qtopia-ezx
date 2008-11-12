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

#ifndef GENERICVIEWER_H
#define GENERICVIEWER_H

#include <QObject>
#include <QString>

#include <qtopia/mail/qmailviewer.h>
#include <qtopia/mail/qmailviewerplugin.h>

class QAction;
class QMailMessage;
class Browser;

// A generic viewer able to show email, SMS or basic MMS
class GenericViewer : public QMailViewerInterface
{
    Q_OBJECT

public:
    GenericViewer(QWidget* parent = 0);
    virtual ~GenericViewer();

    virtual void scrollToAnchor(const QString& a);

    virtual QWidget *widget() const;

    virtual void addActions(QMenu* menu) const;

signals:
    void finished();

public slots:
    virtual bool setMessage(const QMailMessage& mail);
    virtual void setResource(const QUrl& name, QVariant var);
    virtual void clear();

    virtual void action(QAction* action);

    virtual void linkClicked(const QUrl& link);

protected slots:
    virtual void linkHighlighted(const QUrl& link);

private:
    virtual void setPlainTextMode(bool plainTextMode);
    virtual void print() const;

    Browser* browser;
    QAction* plainTextModeAction;
    QAction* richTextModeAction;
    QAction* printAction;
    QAction* dialAction;
    const QMailMessage* message;
    bool plainTextMode;
    bool containsNumbers;
};

class GenericViewerPlugin : public QMailViewerPlugin
{
    Q_OBJECT

public:
    GenericViewerPlugin();

    virtual QString key() const;
    virtual bool isSupported( QMailViewerFactory::ContentType type ) const;

    QMailViewerInterface* create( QWidget* parent );
};

#endif // GENERICVIEWER_H
