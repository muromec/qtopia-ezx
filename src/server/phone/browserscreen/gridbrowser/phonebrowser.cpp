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

#include "phonebrowser.h"
#include <QSettings>
#include <QVBoxLayout>
#include <qcontent.h>
#include <qtopialog.h>
#include <qsoftmenubar.h>
#include <QtopiaChannel>
#include <QDataStream>
#include <QExpressionEvaluator>
//#include <QtopiaServiceHistoryModel>
#include "launcherview.h"
#include <QAction>
#include <QMenu>
#include <QDesktopWidget>
#include <qspeeddial.h>
#include <qtopiaapplication.h>
#include <qtopiaservices.h>
#include <qtopiaipcenvelope.h>
#include "applicationmonitor.h"
#include "contentserver.h"
#include "messagebox.h"
#include "qtopiaserverapplication.h"
#include "contentsetlauncherview.h"
#include <QPerformanceLog>
#include "documentview.h"
#include "hierarchicaldocumentview.h"
#include "taskmanagerlauncherview.h"

// define LazyContentStack
LazyContentStack::LazyContentStack(Flags lcsFlags, QWidget *parent,
                                   Qt::WFlags wflags)
: QWidget(parent, wflags), m_flags(lcsFlags)
{
    connect( &monitor, SIGNAL(applicationStateChanged(QString,UIApplicationMonitor::ApplicationState)), this, SLOT(appStateChanged(QString)) );
}

void LazyContentStack::reset()
{
    m_viewStack.clear();
    emit done();
}

void LazyContentStack::resetToView(const QString &view)
{
    m_viewStack.clear();
    addView(view, true);
}

void LazyContentStack::showView(const QString &view)
{
    if(m_flags && NoStack) {
        m_viewStack.clear();
        addView(view, false);
    } else {
        addView(view, false);
    }
}

QString LazyContentStack::currentView() const
{
    return m_viewStack.top();
}

void LazyContentStack::back()
{
    if(!m_viewStack.isEmpty())
        m_viewStack.pop();

    if(m_viewStack.isEmpty()) {
        emit done();
    } else {
        raiseView(m_viewStack.top(), false);
    }
    notBusy();
}

bool LazyContentStack::isDone() const
{
    return m_viewStack.isEmpty();
}

void LazyContentStack::noView(const QString &)
{
}

void LazyContentStack::busy(const QContent &)
{
}

void LazyContentStack::notBusy()
{
}

void LazyContentStack::addView(const QString &view, bool reset)
{
    if(m_views.contains(view)) {
        m_viewStack.append(view);
        raiseView(view, reset);
    } else {
        QObject *newView = createView(view);
        if(newView) {
            m_views.insert(view);
            m_viewStack.append(view);

            QObject::connect(newView, SIGNAL(clicked(QContent)),
                             this, SLOT(execContent(QContent)));

            raiseView(view, reset);
        } else {
            noView(view);
            if(m_viewStack.isEmpty())
                emit done();
        }
    }
}

void LazyContentStack::execContent(QContent content)
{
    if (!content.type().startsWith("Service/")
     && !content.type().startsWith("Ipc/")
     && !content.type().startsWith("Folder/")
     && content.id() == QContent::InvalidId)
    {
        qLog(DocAPI) << "Attempting to execute an invalid content link:" << content;
        return;
    }

    QString ltype = content.type();

    if (m_views.contains(ltype)) {
        busyApp = QString();
        busy(content);
        showView(ltype);
        notBusy();
    } else if (!content.executableName().isNull() ) {
        content.execute();
        QString app = content.executableName();
        if(UIApplicationMonitor::Starting != (monitor.applicationState(app) && UIApplicationMonitor::StateMask)) {
            busyApp = app;
            busy(content);
        }
    } else {
        QRegExp qrs("Service/([^:]*)::(.*)");
        QRegExp ipc("Ipc/([^:]*)::(.*)");

        busyApp = QString();
        busy(content);

        if (qrs.exactMatch(content.type())) {
            QtopiaServiceRequest req(qrs.cap(1),qrs.cap(2));
            req.send();
        } else if (ipc.exactMatch(content.type())) {
            QtopiaIpcEnvelope env(ipc.cap(1),ipc.cap(2));
        } else {
            QObject *newView = createView(ltype);
            if (!newView) {
                noView(ltype);
            } else {
                m_views.insert(ltype);

                QObject::connect(newView, SIGNAL(clicked(QContent)),
                                 this, SLOT(execContent(QContent)));

                showView(ltype);
            }
        }

        notBusy();
    }
}

void LazyContentStack::appStateChanged(const QString &app)
{
    if(app == busyApp && UIApplicationMonitor::Starting != (monitor.applicationState(app) && UIApplicationMonitor::StateMask)) {
        notBusy();
        busyApp = QString();
    }
}

// define PhoneBrowserStack
PhoneBrowserStack::PhoneBrowserStack(QWidget *parent)
: LazyContentStack(NoFlags, parent), menuIdx(-1),
    appCategories("Applications"),
    phoneLauncher(0)
{
    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(0);
    setLayout(layout);
    stack = new QStackedWidget(this);
    layout->addWidget(stack);
}

QObject* PhoneBrowserStack::createView(const QString &view)
{
    if("Main" == view) {
        Q_ASSERT(!phoneLauncher);
        QSettings cfg(Qtopia::defaultButtonsFile(),
                      QSettings::IniFormat); // No tr
        phoneLauncher = new PhoneMainMenu(cfg, stack);
        stack->addWidget(phoneLauncher);
        return phoneLauncher;
    } else if("Folder/Running" == view) {
        return addType("Running", tr("Running", "application is running"),
                       QPixmap(":image/Generic"));
    } else if("Folder/Documents" == view) {
        return addType("Documents", tr("Documents"),
                       QIcon(QPixmap(":image/qpe/DocsIcon")));
    } else if("Folder/ContentSet" == view ) {
        return createContentSetView();
    } else if(view.startsWith("Folder/")) {
        return createAppView(view.mid(7));
    }

    return 0;
}

void PhoneBrowserStack::raiseView(const QString &view, bool reset)
{
    if(reset) {
        QMap<QString, TypeView>::Iterator it;
        for (it = map.begin(); it != map.end(); ++it)
            (*it).view->resetSelection();
    }

    if("Main" == view) {
        Q_ASSERT(phoneLauncher);
        stack->setCurrentWidget(phoneLauncher);

        if(reset)
            phoneLauncher->showDefaultSelection();

        QContent *cur = phoneLauncher->currentItem();
        if ( cur )
            setWindowTitle( cur->name() );
        setObjectName(QLatin1String("main-menu"));
    } else {
        Q_ASSERT(map.contains(view));
        TypeView tv = map[view];

        QString name = tv.name;
        if (name.isNull())
            name = tv.view->windowTitle();
        setWindowTitle(name);

        // ensure top-level window title is correct if opening a non-app window
        // from the launcher in touchscreen mode
        topLevelWidget()->setWindowTitle(name);

        stack->setCurrentWidget(tv.view);
    }
}

void PhoneBrowserStack::show()
{
    emit visibilityChanged();
}

void PhoneBrowserStack::hide()
{
    emit visibilityChanged();
}

void PhoneBrowserStack::back()
{
    LazyContentStack::back();
}

/*!
  \internal
  */
void PhoneBrowserScreen::closeEvent(QCloseEvent *e)
{
    m_stack->back();

    if(m_stack->isDone()) {
        e->accept();
    } else {
        e->ignore();
    }
}

void PhoneBrowserStack::closeEvent(QCloseEvent *e)
{
    emit visibilityChanged();
    LazyContentStack::closeEvent(e);
}

void PhoneBrowserStack::keyPressEvent(QKeyEvent *ke)
{
    if ( ke->key() == Qt::Key_Hangup || ke->key() == Qt::Key_Flip )
        reset();
    ke->ignore();   //always ignore (Key_Hangup and Key_Flip still need to be processed by the system)
}

void PhoneBrowserStack::busy(const QContent &content)
{
    if(currentLauncherView())
        currentLauncherView()->setBusy(true);
    else if(stack->currentWidget() == phoneLauncher)
        phoneLauncher->setBusy(true);

    emit applicationLaunched(content.fileName());
}

void PhoneBrowserStack::notBusy()
{
    LauncherView *launcherView = currentLauncherView();
    if ( launcherView ) {
        launcherView->setBusy(false);
    }

    // If the PhoneLauncher is still 'busy', it, too, must have its busy flag switched off.
    if ( phoneLauncher ) {
        phoneLauncher->setBusy(false);
    }
}

void PhoneBrowserStack::noView(const QString &view)
{
    QString text;
    if (view.toLower().startsWith("folder/"))
        text = tr("<qt>No applications of this type are installed.</qt>");
    else
        text = tr("<qt>No application is defined for this document.</qt>");
    showMessageBox(tr("No application"), text);
}

LauncherView *PhoneBrowserStack::currentLauncherView()
{
    if (stack->currentWidget()->inherits("LauncherView")) // No tr
        return qobject_cast<LauncherView*>(stack->currentWidget());
    return 0;
}

LauncherView *PhoneBrowserStack::createAppView(const QString &category)
{
    //QString categoryId = QString("_apps_%1").arg( category ); // Don't format the id.
    QString categoryId = category;

    TypeView tv;
    tv.view = new ApplicationLauncherView(categoryId, stack);
    tv.view->setObjectName(category);

    QFont f(font());
    f.setWeight(QFont::Bold);
    tv.view->setFont(f);
    tv.view->setViewMode(QListView::ListMode);

    tv.name = appCategories.label(categoryId);
    tv.icon = appCategories.icon(categoryId);

    map["Folder/"+category] = tv;
    stack->addWidget(tv.view);

    return tv.view;
}

LauncherView * PhoneBrowserStack::addType(const QString& type, const QString& name, const QIcon &icon)
{
    TypeView tv;
    if("Documents" == type) {
#ifdef ENABLE_HIERARCHICAL_DOCUMENT_VIEW
        tv.view = new HierarchicalDocumentLauncherView (this, 0);
#else
        tv.view = new DocumentLauncherView(this, 0);
#endif
        tv.view->setObjectName(type);
    } else if("Running" == type) {
        tv.view = new TaskManagerLauncherView(this);
    } else {
        Q_ASSERT(!"Unknown view");
        return 0;
    }

    QFont f(font());
    f.setWeight(QFont::Bold);
    tv.view->setFont(f);
    tv.view->setViewMode(QListView::ListMode);

    tv.name = name;
    tv.icon = icon;

    map["Folder/"+type] = tv;
    stack->addWidget( tv.view );
    return tv.view;
}

LauncherView * PhoneBrowserStack::createContentSetView()
{
    TypeView tv;

    ContentSetLauncherView *view = new ContentSetLauncherView( this );

    tv.view = view;
    tv.name = tr( "Content" );

    QFont f(font());
    f.setWeight(QFont::Bold);
    tv.view->setFont(f);
    tv.view->setViewMode(QListView::ListMode);
    tv.view->setObjectName(tv.name);


    map["Folder/ContentSet"] = tv;
    stack->addWidget( tv.view );
    return tv.view;
}

void PhoneBrowserStack::showMessageBox(const QString& title, const QString& text, QAbstractMessageBox::Icon icon)
{
    if (!warningBox) {
        warningBox = QAbstractMessageBox::messageBox(this, title, text, icon);
        warningBox->setAttribute(Qt::WA_DeleteOnClose); // It's a QPointer<> so safe.
    }
    warningBox->setText(text);
    QtopiaApplication::showDialog(warningBox);
}

void PhoneBrowserStack::showType(const QString &type)
{
    resetToView(type);
}

// define PhoneMainMenu
PhoneMainMenu::PhoneMainMenu(QSettings &config, QWidget * parent)
: PhoneLauncherView(config.value("Menu/Rows", 3).toInt(),
                    config.value("Menu/Columns", 3).toInt(),
                    config.value("Menu/Map", "123456789").toString(),
                    config.value("Menu/Animator","").toString(),
                    config.value("Menu/AnimatorBackground","").toString(),
                    parent)
{
    makeLauncherMenu(config);
}

void PhoneMainMenu::showDefaultSelection()
{
    setCurrentItem(defaultSelection);
}

// Make it simple to map between expressions and keys
class PhoneMainMenu::ItemExpression : public QExpressionEvaluator
{
public:
    ItemExpression(const QChar &c, const QByteArray &exp, QObject *parent = 0)
        : QExpressionEvaluator(exp, parent), m_c(c)
    {
    }

    QChar character() const
    {
        return m_c;
    }

private:
    QChar m_c;
};

void PhoneMainMenu::makeLauncherMenu(QSettings &cfg)
{
    cfg.beginGroup("Menu"); // No tr
    const int menur = cfg.value("Rows",3).toInt();
    const int menuc = cfg.value("Columns",3).toInt();
    menuKeyMap = cfg.value("Map","123456789").toString();

    QPerformanceLog("UI") << QPerformanceLog::Begin << "PhoneMainMenu create";

    qLog(UI) << "PhoneMainMenu:";
    qLog(UI) << "    " << menur << "x" << menuc;
    qLog(UI) << "    Mapping keys:" << menuKeyMap;

    // For multitasking, this is always needed.
    for (int i = 0; i < menur*menuc; i++) {
        QChar key = menuKeyMap[i];
        QStringList entries = cfg.value(QString(key)).toStringList();
        if(entries.isEmpty())
            qLog(UI) << "    Key" << QString(key) << ": No mapping";
        else
            qLog(UI) << "    Key" << QString(key) << ":" << entries.join(",");

        for(int jj = 0; jj < entries.count(); ++jj) {
            const QString &entry = entries.at(jj);

            QString lnk;
            QString expr;

            // Check for expression
            int exprIdx = entry.indexOf('{');
            if(-1 != exprIdx) {
                lnk = entry.left(exprIdx);
                int exprEndIdx = entry.indexOf('}', exprIdx);
                expr = entry.mid(exprIdx + 1, (exprEndIdx == -1)?-1:exprEndIdx - exprIdx - 1);
            } else {
                lnk = entry;
            }

            QContent *appLnk = readLauncherMenuItem(lnk);
            if(appLnk) {
                if(expr.isEmpty())
                    qLog(UI) << "        Name:" << appLnk->name()
                             <<         "Icon:" << appLnk->icon()
                             <<         "Exec:" << appLnk->executableName();
                else
                    qLog(UI) << "        Name:" << appLnk->name()
                             <<         "Icon:" << appLnk->icon()
                             <<         "Exec:" << appLnk->executableName()
                             <<         "Expr:" << expr;

                Item item;
                item.lnk = *appLnk;
                item.expr = 0;
                if(!expr.isEmpty()) {
                    item.expr = new ItemExpression(key, expr.toLatin1());
                    QObject::connect(item.expr, SIGNAL(termsChanged()),
                                     this, SLOT(expressionChanged()));
                }

                mainMenuEntries[key].append(item);

                delete appLnk;
            } else {
                qLog(UI) << "        No target: " << lnk;
            }
        }

        QMap<QChar,Items>::Iterator iter = mainMenuEntries.find(key);
        if(iter != mainMenuEntries.end()) {
            // Setup default
            bool setDefault = false;
            for(int jj = 0; !setDefault && jj < iter->count(); ++jj) {
                if((*iter)[jj].exprTrue()) {
                    iter->activeItem = jj;
                    setDefault = true;
                    addItem(new QContent(iter->at(jj).lnk), i);
                }
            }
            if(!setDefault) {
                iter->activeItem = iter->count() - 1;
                addItem(new QContent(iter->at(iter->count() - 1).lnk), i);
            }
        }
    }

    QString d = cfg.value("Default",menuKeyMap.mid(menuKeyMap.length()/2,1)).toString();
    defaultSelection = menuKeyMap.indexOf(d);
    setCurrentItem(defaultSelection);

    // just to get help for the main menu
    (void)QSoftMenuBar::menuFor(this);

    QPerformanceLog("UI") << QPerformanceLog::End << "PhoneMainMenu create";

    cfg.endGroup();
}

QContent *PhoneMainMenu::readLauncherMenuItem(const QString &entry)
{
    QContent *applnk = 0;

    if (entry.right(8)==".desktop") {
        // There used to be a quick way to locate a .desktop file
        // Now we have to create a QContentSet and iterate over the items

        // The path to the apps folder (which only exists in the database)
        QString apps = Qtopia::qtopiaDir()+"apps/";
        // We need the full path to the entry to compare against the items we get from QContentSet
        QString entryPath = apps+entry;
        applnk = new QContent( entryPath, false );
        if ( applnk->id() == QContent::InvalidId ) {
            delete applnk;
            applnk = 0;
        }
    } else if (entry == "Documents") { // No tr
        applnk = new QContent();
        applnk->setName(tr("Documents"));
        applnk->setType("Folder/Documents");
        applnk->setIcon("qpe/DocsIcon");
    } else if (entry == "Running") { // No tr
        applnk = new QContent();
        applnk->setName(tr("Running"));
        applnk->setType("Folder/Running");
        applnk->setIcon("Generic");
    } else {
        QCategoryManager catman("Applications");
        if(catman.contains(entry))
        {
            applnk = new QContent();
            applnk->setName(catman.label(entry));
            applnk->setIcon(catman.iconFile(entry));
            applnk->setType("Folder/"+entry);
        }
        else
            applnk = NULL;
    }

    return applnk;
}

bool PhoneMainMenu::Item::exprTrue()
{
    return !expr || (expr->evaluate() && expr->result().toBool());
}

void PhoneMainMenu::activateItem(const QChar &c, int idx)
{
    Items &items = mainMenuEntries[c];
    Q_ASSERT(items.count() > idx && idx >= 0);
    if(items.activeItem != idx) {
        items.activeItem = idx;
        qLog(UI) << "PhoneMainMenu: Activating"
                 << items.at(idx).lnk.name()
                 << "for key" << c;
        addItem(new QContent(items.at(idx).lnk),
                menuKeyMap.indexOf(c));
    }
}

void PhoneMainMenu::expressionChanged()
{
    Q_ASSERT(sender());
    ItemExpression *e = static_cast<ItemExpression *>(sender());

    Items &items = mainMenuEntries[e->character()];
    for(int ii = 0; ii < items.count(); ++ii) {
        Q_ASSERT(ii == items.activeItem || items.at(ii).expr);

        if(ii == items.activeItem && e == items.at(ii).expr) {
            // Hmmm... we're the one that changed.  If we're still true, there's
            // nothing to do, otherwise we need to find the next valid guy or
            // the last if none are true
            bool valid = items[ii].exprTrue();
            if(!valid) {
                for(int jj = ii + 1; jj < items.count(); ++jj) {
                    if(items[jj].exprTrue()) {
                        activateItem(e->character(), jj);
                        return;
                    }
                }
                activateItem(e->character(), items.count() - 1);
            }
            return;

        } else if(ii == items.activeItem) {
            // The changed item must be behind the active item, so there's
            // nothing todo
            return;
        } else if(e == items.at(ii).expr) {
            // We've found the expression that changed and it is infront of the
            // active item.  If it is true, then we should set it as active,
            // otherwise do nothing
            if(e->evaluate() && e->result().toBool()) {
                // Activate this guy
                activateItem(e->character(), ii);
            }
            return;
        }
    }
    Q_ASSERT(!"Invalid code path");
}

void PhoneMainMenu::setMainMenuItemEnabled(const QString &file, const QString &name, const QString &icon, bool enabled)
{
    QMap<QChar,Items>::Iterator it;
    for (it = mainMenuEntries.begin(); it != mainMenuEntries.end(); ++it) {
        Items &itemList = *it;
        bool found = false;
        bool change = false;
        Item *enabledItem = 0;
        for (int i=0; i < (int)itemList.count(); i++) {
            Item &item = itemList[i];
            if (!found && item.lnk.executableName() == file) {
                item.enabled = enabled;
                if (!enabledItem && enabled)
                    change = true;
                found = true;
            }
            if (!enabledItem && item.enabled)
                enabledItem = &item;
        }
        if (found) {
            if (!enabledItem && itemList.count())
                enabledItem = &itemList.last();
            if (enabledItem) {
                QContent *appLnk = new QContent(enabledItem->lnk);
                if (change) {
                    if (!name.isEmpty())
                        appLnk->setName(name);
                    if (!icon.isEmpty())
                        appLnk->setIcon(icon);
                }
                addItem(appLnk, menuKeyMap.indexOf(it.key()));
            }
            break;
        }
    }
}

QString PhoneBrowserStack::currentName() const
{
    LauncherView *v = qobject_cast<LauncherView *>(stack->currentWidget());
    for (QMap<QString,TypeView>::ConstIterator it = map.begin();
            it != map.end(); it++) {
        if ( (*it).view == v )
            return (*it).name;
    }
    return QString();
}

/*!
  \class PhoneBrowserScreen
  \brief The PhoneBrowserScreen class provides the main launcher grid for Qtopia Phone.
  \ingroup QtopiaServer::PhoneUI

  This class is a Qtopia \l{QtopiaServerApplication#qtopia-server-widgets}{server widget}. 
  It is part of the Qtopia server and cannot be used by other Qtopia applications.

  \sa QAbstractServerInterface, QAbstractBrowserScreen

*/

/*!
  Constructs a new PhoneBrowserScreen instance with the specified \a parent
  and widget \a flags
  */
PhoneBrowserScreen::PhoneBrowserScreen(QWidget *parent, Qt::WFlags flags)
: QAbstractBrowserScreen(parent, flags), m_stack(0)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    setLayout(layout);
    layout->setSpacing(0);
    layout->setMargin(0);
    m_stack = new PhoneBrowserStack(this);
    QObject::connect(m_stack, SIGNAL(applicationLaunched(QString)), this,
                     SIGNAL(applicationLaunched(QString)));
    layout->addWidget(m_stack);
    setFocusPolicy(Qt::NoFocus);
//    setFocusProxy(m_stack);
}

/*!
  \reimp
  */
QString PhoneBrowserScreen::currentView() const
{
    return m_stack->currentName();
}

/*!
  \reimp
  */
bool PhoneBrowserScreen::viewAvailable(const QString &) const
{
    return true;
}

/*!
  \reimp
  */
void PhoneBrowserScreen::resetToView(const QString &view)
{
    m_stack->resetToView(view);
}

/*!
  \reimp
  */
void PhoneBrowserScreen::moveToView(const QString &view)
{
    m_stack->showView(view);
}

QTOPIA_REPLACE_WIDGET(QAbstractBrowserScreen, PhoneBrowserScreen);

