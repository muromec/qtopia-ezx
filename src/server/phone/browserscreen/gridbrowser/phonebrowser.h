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

#ifndef _PHONEBROWSER_H_
#define _PHONEBROWSER_H_

#include "phonelauncherview.h"

#include <QStackedWidget>
#include <QCloseEvent>
#include <QMap>
#include <QSet>
#include <QStack>
#include <QString>
#include <qcontent.h>
#include "launcherview.h"
#include <QPointer>
#include "messagebox.h"
#include <qcategorymanager.h>
#include "qabstractbrowserscreen.h"
#include "applicationmonitor.h"

class LazyContentStack : public QWidget
{
Q_OBJECT
public:
    enum Flags { NoFlags = 0x00000000, NoStack = 0x00000001 };
    Q_FLAGS(Flags);

    LazyContentStack(Flags lcsFlags = NoFlags, QWidget *parent = 0,
                     Qt::WFlags wflags = 0);

    virtual void reset();
    virtual void resetToView(const QString &);
    virtual void showView(const QString &);
    virtual QString currentView() const;

    bool isDone() const;
protected:
    // Lazy creation methods
    virtual QObject* createView(const QString &) = 0;
    virtual void raiseView(const QString &, bool reset) = 0;

    // UI methods
    virtual void noView(const QString &);
    virtual void busy(const QContent &);
    virtual void notBusy();

    // Stack manipulation
    void back();

signals:
    void done();
    void viewContentSet( const QContentSet &set );

private slots:
    void appStateChanged(const QString &);
    void execContent(QContent);

private:
    void addView(const QString &, bool);

    QString busyApp;
    UIApplicationMonitor monitor;
    Flags m_flags;
    QSet<QString> m_views;
    QStack<QString> m_viewStack;
};

class QAction;
class QMenu;
struct TypeView {
    TypeView() : view(0) {};
    LauncherView *view;
    QString name;
    QIcon icon;
};

class QSettings;
class PhoneMainMenu;
class DocumentLauncherView;
class PhoneBrowserStack : public LazyContentStack
{
Q_OBJECT
public:
    PhoneBrowserStack(QWidget *parent = 0);
    virtual void show();
    virtual void hide();
    void insertPhoneMenu(QWidget *m);

    void showType(const QString &);

    QString currentName() const;

    void back();
protected:
    void keyPressEvent(QKeyEvent *ke);

    virtual void busy(const QContent &);
    virtual void notBusy();
    virtual void noView(const QString &);

    virtual QObject* createView(const QString &);
    virtual void raiseView(const QString &, bool);
    void closeEvent(QCloseEvent *e);

signals:
    void visibilityChanged();
    void applicationLaunched(const QString &);

private:
    LauncherView *addType(const QString& type,
                          const QString& name, const QIcon &icon);
    LauncherView *createAppView(const QString &);
    LauncherView *createContentSetView();

    LauncherView *currentLauncherView();
    int menuIdx;
    void showMessageBox(const QString& title, const QString& text, QAbstractMessageBox::Icon icon=QAbstractMessageBox::Information);

    QCategoryManager appCategories;

public:
    QPointer<QAbstractMessageBox> warningBox;
    PhoneMainMenu *phoneLauncher;
    QMap<QString, TypeView> map;
    QStackedWidget *stack;
};

class PhoneMainMenu : public PhoneLauncherView
{
Q_OBJECT
public:
    PhoneMainMenu(QSettings &, QWidget * parent = 0);

    void showDefaultSelection();

private slots:
    void expressionChanged();

private:
    void activateItem(const QChar &, int);

    void setMainMenuItemEnabled(const QString &file, const QString &name, const QString &icon, bool enabled);
    void makeLauncherMenu(QSettings &);
    QContent *readLauncherMenuItem(const QString &);

    class ItemExpression;
    struct Item {
        QContent lnk;
        bool exprTrue();
        ItemExpression *expr;
        bool enabled;
    };
    struct Items : public QList<Item>
    {
        Items() : activeItem(-1) {}
        int activeItem;
    };
    QMap<QChar,Items> mainMenuEntries;
    QString menuKeyMap;
    int defaultSelection;
};

class PhoneBrowserScreen : public QAbstractBrowserScreen
{
Q_OBJECT
public:
    PhoneBrowserScreen(QWidget *parent = 0, Qt::WFlags flags = 0);

    virtual QString currentView() const;
    virtual bool viewAvailable(const QString &) const;
    virtual void resetToView(const QString &);
    virtual void moveToView(const QString &);

protected:
    void closeEvent(QCloseEvent *e);

private:
    PhoneBrowserStack *m_stack;
    QHash<QString, QContent*> m_dynamicallyAddedItems;
};

#endif // _PHONEBROWSER_H_
