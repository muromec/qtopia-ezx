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

#ifndef _e1_PHONEBROWSER_H_
#define _e1_PHONEBROWSER_H_

#include "e1_bar.h"
#include <QStackedWidget>
#include <qcategorymanager.h>
#include "phonebrowser.h"
#include <QPixmap>
#include <QString>
#include <QStringList>
#include <qvaluespace.h>
#include "launcherview.h"

class E1PhoneBrowserTabs;

class E1PhoneBrowser : public LazyContentStack
{
Q_OBJECT
public:
    E1PhoneBrowser(QWidget * = 0, Qt::WFlags = 0);

    void display();

protected:
    virtual QObject* createView(const QString &);
    virtual void resetToView(const QString &);
    virtual void showView(const QString &);
    virtual void raiseView(const QString &, bool reset);
    virtual void keyPressEvent(QKeyEvent *);

private slots:
    void tabChanged(const QString &);
    void message(const QString &, const QByteArray &);
    void toList();
    void toIcon();

private:
    void primeView(const QString &);
    void stopFocus(QObject *);

    QStackedWidget * m_stack;
    E1PhoneBrowserTabs * m_tabs;
    QContentSet appCategories;
    QListView::ViewMode m_mode;

    QMap<QString, QWidget *> m_views;
};


#endif // _e1_PHONEBROWSER_H_

