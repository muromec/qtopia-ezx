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

#ifndef SMILVIEWER_H
#define SMILVIEWER_H

#include <QEvent>
#include <QMap>
#include <QString>
#include <QVariant>

#include <qtopia/mail/qmailviewer.h>
#include <qtopia/mail/qmailviewerplugin.h>

class QIODevice;
class QUrl;
class QWidget;

class QMailMessage;
class SmilDataSource;
class SmilView;

// A viewer able to playback a SMIL MMS mail
class SmilViewer : public QMailViewerInterface
{
    Q_OBJECT

public:
    SmilViewer(QWidget* parent = 0);
    virtual ~SmilViewer();

    virtual QWidget *widget() const;

public slots:
    virtual bool setMessage(const QMailMessage& mail);
    virtual void clear();

protected:
    virtual bool eventFilter(QObject* watched, QEvent* event);

private slots:
    void requestTransfer(SmilDataSource* dataSource, const QString &src);
    void cancelTransfer(SmilDataSource *dataSource, const QString &src);

private:
    void tweakView();
    void advanceSlide();

    SmilView* view;
    QMap<SmilDataSource*,QIODevice*> transfers;
    int menuKey;
    const QMailMessage* mail;
};

class SmilViewerPlugin : public QMailViewerPlugin
{
    Q_OBJECT

public:
    SmilViewerPlugin();

    virtual QString key() const;
    virtual bool isSupported( QMailViewerFactory::ContentType type ) const;

    QMailViewerInterface* create( QWidget* parent );
};

#endif // SMILVIEWER_H

