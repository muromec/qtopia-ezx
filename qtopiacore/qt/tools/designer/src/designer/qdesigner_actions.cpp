/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the Qt Designer of the Qt Toolkit.
**
** This file may be used under the terms of the GNU General Public
** License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the files LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file.  Alternatively you may (at
** your option) use any later version of the GNU General Public
** License if such license has been publicly approved by Trolltech ASA
** (or its successors, if any) and the KDE Free Qt Foundation. In
** addition, as a special exception, Trolltech gives you certain
** additional rights. These rights are described in the Trolltech GPL
** Exception version 1.2, which can be found at
** http://www.trolltech.com/products/qt/gplexception/ and in the file
** GPL_EXCEPTION.txt in this package.
**
** Please review the following information to ensure GNU General
** Public Licensing requirements will be met:
** http://trolltech.com/products/qt/licenses/licensing/opensource/. If
** you are unsure which license is appropriate for your use, please
** review the following information:
** http://trolltech.com/products/qt/licenses/licensing/licensingoverview
** or contact the sales department at sales@trolltech.com.
**
** In addition, as a special exception, Trolltech, as the sole
** copyright holder for Qt Designer, grants users of the Qt/Eclipse
** Integration plug-in the right for the Qt/Eclipse Integration to
** link to functionality provided by Qt Designer and its related
** libraries.
**
** This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
** INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE. Trolltech reserves all rights not expressly
** granted herein.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
****************************************************************************/

#include "qdesigner_actions.h"
#include "designer_enums.h"
#include "qdesigner.h"
#include "qdesigner_workbench.h"
#include "qdesigner_formwindow.h"
#include "qdesigner_settings.h"
#include "newform.h"
#include "versiondialog.h"
#include "oublietteview.h"
#include "saveformastemplate.h"
#include "plugindialog.h"
#include "formwindowsettings.h"
#include "qdesigner_toolwindow.h"
#include "preferencesdialog.h"
#include "preferences.h"

#include <pluginmanager_p.h>
#include <qdesigner_formbuilder_p.h>
#include <qdesigner_utils_p.h>
#include <iconloader_p.h>
#include <qsimpleresource_p.h>
// sdk
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerFormWindowInterface>
#include <QtDesigner/QDesignerLanguageExtension>
#include <QtDesigner/QDesignerWidgetDataBaseInterface>
#include <QtDesigner/QDesignerMetaDataBaseInterface>
#include <QtDesigner/QDesignerFormWindowManagerInterface>
#include <QtDesigner/QDesignerFormWindowCursorInterface>
#include <QtDesigner/QDesignerPropertySheetExtension>
#include <QtDesigner/QDesignerPropertyEditorInterface>
#include <QtDesigner/QDesignerFormEditorPluginInterface>
#include <QtDesigner/QExtensionManager>

#include <QtAssistant/QAssistantClient>

#include <QtGui/QAction>
#include <QtGui/QActionGroup>
#include <QtGui/QStyleFactory>
#include <QtGui/QCloseEvent>
#include <QtGui/QFileDialog>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>
#include <QtGui/QIcon>
#include <QtGui/QMdiSubWindow>

#include <QtCore/QLibraryInfo>
#include <QtCore/QBuffer>
#include <QtCore/QPluginLoader>
#include <QtCore/qdebug.h>
#include <QtCore/QTimer>
#include <QtCore/QMetaObject>
#include <QtCore/QFileInfo>
#include <QtGui/QStatusBar>
#include <QtGui/QDesktopWidget>
#include <QtXml/QDomDocument>

#ifdef Q_WS_MAC
#  define NONMODAL_PREVIEW
#endif

static QString getFileExtension(QDesignerFormEditorInterface *core)
{
    QDesignerLanguageExtension *lang
        = qt_extension<QDesignerLanguageExtension *>(core->extensionManager(), core);
    if (lang)
        return lang->uiExtension();
    return QLatin1String("ui");
}

static QAction *createSeparator(QObject *parent) {
    QAction * rc = new QAction(parent);
    rc->setSeparator(true);
    return rc;
}

static QActionGroup *createActionGroup(QObject *parent, bool exclusive = false) {
    QActionGroup * rc = new QActionGroup(parent);
    rc->setExclusive(exclusive);
    return rc;
}

static inline QString savedMessage(const QString &fileName)
{
    return QDesignerActions::tr("Saved %1.").arg(fileName);
}

QDesignerActions::QDesignerActions(QDesignerWorkbench *workbench)
    : QObject(workbench),
      m_workbench(workbench),
      m_assistantClient(0),
      m_backupTimer(new QTimer(this)),
      m_fileActions(createActionGroup(this)),
      m_recentFilesActions(createActionGroup(this)),
      m_editActions(createActionGroup(this)),
      m_formActions(createActionGroup(this)),
      m_windowActions(createActionGroup(this)),
      m_toolActions(createActionGroup(this, true)),
      m_helpActions(0),
      m_styleActions(createActionGroup(this, true)),
      m_editWidgetsAction(new QAction(tr("Edit Widgets"), this)),
      m_newFormAction(new QAction(qdesigner_internal::createIconSet(QLatin1String("filenew.png")), tr("&New Form..."), this)),
      m_openFormAction(new QAction(qdesigner_internal::createIconSet(QLatin1String("fileopen.png")), tr("&Open Form..."), this)),
      m_saveFormAction(new QAction(qdesigner_internal::createIconSet(QLatin1String("filesave.png")), tr("&Save Form"), this)),
      m_saveFormAsAction(new QAction(tr("Save Form &As..."), this)),
      m_saveAllFormsAction(new QAction(tr("Save A&ll Forms"), this)),
      m_saveFormAsTemplateAction(new QAction(tr("Save Form As &Template..."), this)),
      m_closeFormAction(new QAction(tr("&Close Form"), this)),
      m_quitAction(new QAction(tr("&Quit"), this)),
      m_previewFormAction(new QAction(tr("&Preview"), this)),
      m_formSettings(new QAction(tr("Form &Settings..."), this)),
      m_minimizeAction(new QAction(tr("&Minimize"), this)),
      m_bringAllToFrontSeparator(createSeparator(this)),
      m_bringAllToFrontAction(new QAction(tr("Bring All to Front"), this)),
      m_windowListSeparatorAction(createSeparator(this)),
      m_preferencesAction(new QAction(tr("Preferences..."), this))
{
    Q_ASSERT(m_workbench != 0);

    m_core = m_workbench->core();
    Q_ASSERT(m_core != 0);

    m_helpActions = createHelpActions();

    QDesignerFormWindowManagerInterface *formWindowManager = m_core->formWindowManager();
    Q_ASSERT(formWindowManager != 0);

    QDesignerSettings settings;
//
// file actions
//
    m_newFormAction->setShortcut(tr("CTRL+N"));
    connect(m_newFormAction, SIGNAL(triggered()), this, SLOT(createForm()));
    m_fileActions->addAction(m_newFormAction);

    m_openFormAction->setShortcut(tr("CTRL+O"));
    connect(m_openFormAction, SIGNAL(triggered()), this, SLOT(slotOpenForm()));
    m_fileActions->addAction(m_openFormAction);

    QAction *act;
    // Need to insert this into the QAction.
    for (int i = 0; i < MaxRecentFiles; ++i) {
        act = new QAction(this);
        act->setVisible(false);
        connect(act, SIGNAL(triggered()), this, SLOT(openRecentForm()));
        m_recentFilesActions->addAction(act);
    }
    updateRecentFileActions();

    m_recentFilesActions->addAction(createSeparator(this));

    act = new QAction(tr("Clear &Menu"), this);
    act->setObjectName(QLatin1String("__qt_action_clear_menu_"));
    connect(act, SIGNAL(triggered()), this, SLOT(clearRecentFiles()));
    m_recentFilesActions->addAction(act);

    m_fileActions->addAction(createSeparator(this));

    m_saveFormAction->setShortcut(tr("CTRL+S"));
    connect(m_saveFormAction, SIGNAL(triggered()), this, SLOT(saveForm()));
    m_fileActions->addAction(m_saveFormAction);

    connect(m_saveFormAsAction, SIGNAL(triggered()), this, SLOT(saveFormAs()));
    m_fileActions->addAction(m_saveFormAsAction);

    m_saveAllFormsAction->setShortcut(tr("CTRL+SHIFT+S"));
    connect(m_saveAllFormsAction, SIGNAL(triggered()), this, SLOT(saveAllForms()));
    m_fileActions->addAction(m_saveAllFormsAction);

    connect(m_saveFormAsTemplateAction, SIGNAL(triggered()), this, SLOT(saveFormAsTemplate()));
    m_fileActions->addAction(m_saveFormAsTemplateAction);

    m_fileActions->addAction(createSeparator(this));

    m_closeFormAction = new QAction(this);
    m_closeFormAction->setShortcut(tr("CTRL+W"));
    connect(m_closeFormAction, SIGNAL(triggered()), this, SLOT(closeForm()));
    m_fileActions->addAction(m_closeFormAction);
    updateCloseAction();

    m_fileActions->addAction(createSeparator(this));

    m_quitAction->setShortcut(tr("CTRL+Q"));
    m_quitAction->setMenuRole(QAction::QuitRole);
    connect(m_quitAction, SIGNAL(triggered()), this, SLOT(shutdown()));
    m_fileActions->addAction(m_quitAction);

//
// edit actions
//
    QAction *undoAction = formWindowManager->actionUndo();
    undoAction->setShortcut(tr("CTRL+Z"));
    m_editActions->addAction(undoAction);

    QAction *redoAction = formWindowManager->actionRedo();
    redoAction->setShortcut(tr("CTRL+SHIFT+Z"));
    m_editActions->addAction(redoAction);

    m_editActions->addAction(createSeparator(this));

    m_editActions->addAction(formWindowManager->actionCut());
    m_editActions->addAction(formWindowManager->actionCopy());
    m_editActions->addAction(formWindowManager->actionPaste());
    m_editActions->addAction(formWindowManager->actionDelete());

    m_editActions->addAction(formWindowManager->actionSelectAll());

    m_editActions->addAction(createSeparator(this));

    m_editActions->addAction(formWindowManager->actionLower());
    m_editActions->addAction(formWindowManager->actionRaise());

//
// edit mode actions
//

    m_editWidgetsAction->setCheckable(true);
    m_editWidgetsAction->setShortcut(tr("F3"));
    m_editWidgetsAction->setIcon(QIcon(m_core->resourceLocation() + QLatin1String("/widgettool.png")));
    connect(m_editWidgetsAction, SIGNAL(triggered()), this, SLOT(editWidgetsSlot()));
    m_editWidgetsAction->setChecked(true);
    m_editWidgetsAction->setEnabled(false);
    m_toolActions->addAction(m_editWidgetsAction);

    connect(formWindowManager, SIGNAL(activeFormWindowChanged(QDesignerFormWindowInterface*)),
                this, SLOT(activeFormWindowChanged(QDesignerFormWindowInterface*)));

    QList<QObject*> builtinPlugins = QPluginLoader::staticInstances();
    builtinPlugins += m_core->pluginManager()->instances();
    foreach (QObject *plugin, builtinPlugins) {
        if (QDesignerFormEditorPluginInterface *formEditorPlugin = qobject_cast<QDesignerFormEditorPluginInterface*>(plugin)) {
            if (QAction *action = formEditorPlugin->action()) {
                m_toolActions->addAction(action);
                action->setCheckable(true);
            }
        }
    }

    connect(m_preferencesAction, SIGNAL(triggered()),  this, SLOT(showPreferencesDialog()));
    m_preferencesAction->setMenuRole(QAction::PreferencesRole);
//
// form actions
//

    m_formActions->addAction(formWindowManager->actionHorizontalLayout());
    m_formActions->addAction(formWindowManager->actionVerticalLayout());
    m_formActions->addAction(formWindowManager->actionSplitHorizontal());
    m_formActions->addAction(formWindowManager->actionSplitVertical());
    m_formActions->addAction(formWindowManager->actionGridLayout());
    m_formActions->addAction(formWindowManager->actionBreakLayout());
    m_formActions->addAction(formWindowManager->actionAdjustSize());
    m_formActions->addAction(createSeparator(this));

    m_previewFormAction->setShortcut(tr("CTRL+R"));
    connect(m_previewFormAction, SIGNAL(triggered()), this, SLOT(previewFormLater()));
    m_formActions->addAction(m_previewFormAction);

    connect(m_styleActions, SIGNAL(triggered(QAction*)), this, SLOT(previewForm(QAction*)));

    m_formActions->addAction(createSeparator(this));

    m_formSettings->setEnabled(false);
    connect(m_formSettings, SIGNAL(triggered()), this, SLOT(showFormSettings()));
    m_formActions->addAction(m_formSettings);

    foreach (QString style,  QStyleFactory::keys()) {
        QAction *a = new QAction(tr("%1 Style").arg(style), this);
        a->setData(style);
        m_styleActions->addAction(a);
    }

//
// window actions
//
    m_minimizeAction->setEnabled(false);
    m_minimizeAction->setCheckable(true);
    m_minimizeAction->setShortcut(tr("CTRL+M"));
    connect(m_minimizeAction, SIGNAL(triggered()), m_workbench, SLOT(toggleFormMinimizationState()));
    m_windowActions->addAction(m_minimizeAction);

    m_windowActions->addAction(m_bringAllToFrontSeparator);
    connect(m_bringAllToFrontAction, SIGNAL(triggered()), m_workbench, SLOT(bringAllToFront()));
    m_windowActions->addAction(m_bringAllToFrontAction);
    m_windowActions->addAction(m_windowListSeparatorAction);

    setWindowListSeparatorVisible(false);

//
// connections
//
    fixActionContext();
    activeFormWindowChanged(core()->formWindowManager()->activeFormWindow());

    m_backupTimer->start(180000); // 3min
    connect(m_backupTimer, SIGNAL(timeout()), this, SLOT(backupForms()));
}

QActionGroup *QDesignerActions::createHelpActions()
{
    QActionGroup *helpActions = createActionGroup(this);

#ifndef QT_JAMBI_BUILD
    QAction *mainHelpAction = new QAction(tr("Qt Designer &Help"), this);
    connect(mainHelpAction, SIGNAL(triggered()), this, SLOT(showDesignerHelp()));
    mainHelpAction->setShortcut(Qt::CTRL + Qt::Key_Question);
    helpActions->addAction(mainHelpAction);

    helpActions->addAction(createSeparator(this));
    QAction *widgetHelp = new QAction(tr("Current Widget Help"), this);
    widgetHelp->setShortcut(Qt::Key_F1);
    connect(widgetHelp, SIGNAL(triggered()), this, SLOT(showWidgetSpecificHelp()));
    helpActions->addAction(widgetHelp);

    helpActions->addAction(createSeparator(this));
    QAction *whatsNewAction = new QAction(tr("What's New in Qt Designer?"), this);
    connect(whatsNewAction, SIGNAL(triggered()), this, SLOT(showWhatsNew()));
    helpActions->addAction(whatsNewAction);
#endif

    helpActions->addAction(createSeparator(this));
    QAction *aboutPluginsAction = new QAction(tr("About Plugins"), this);
    aboutPluginsAction->setMenuRole(QAction::ApplicationSpecificRole);
    connect(aboutPluginsAction, SIGNAL(triggered()), this, SLOT(aboutPlugins()));
    helpActions->addAction(aboutPluginsAction);

    QAction *aboutDesignerAction = new QAction(tr("About Qt Designer"), this);
    aboutDesignerAction->setMenuRole(QAction::AboutRole);
    connect(aboutDesignerAction, SIGNAL(triggered()), this, SLOT(aboutDesigner()));
    helpActions->addAction(aboutDesignerAction);

    QAction *aboutQtAction = new QAction(tr("About Qt"), this);
    aboutQtAction->setMenuRole(QAction::AboutQtRole);
    connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    helpActions->addAction(aboutQtAction);
    return helpActions;
}


QDesignerActions::~QDesignerActions()
{
}

QActionGroup *QDesignerActions::toolActions() const
{ return m_toolActions; }

QDesignerWorkbench *QDesignerActions::workbench() const
{ return m_workbench; }

QDesignerFormEditorInterface *QDesignerActions::core() const
{ return m_core; }

QActionGroup *QDesignerActions::fileActions() const
{ return m_fileActions; }

QActionGroup *QDesignerActions::editActions() const
{ return m_editActions; }

QActionGroup *QDesignerActions::formActions() const
{ return m_formActions; }

QActionGroup *QDesignerActions::windowActions() const
{ return m_windowActions; }

QActionGroup *QDesignerActions::helpActions() const
{ return m_helpActions; }

QActionGroup *QDesignerActions::styleActions() const
{ return m_styleActions; }


QAction *QDesignerActions::previewFormAction() const
{ return m_previewFormAction; }


void QDesignerActions::editWidgetsSlot()
{
    QDesignerFormWindowManagerInterface *formWindowManager = core()->formWindowManager();
    for (int i=0; i<formWindowManager->formWindowCount(); ++i) {
        QDesignerFormWindowInterface *formWindow = formWindowManager->formWindow(i);
        formWindow->editWidgets();
    }
}

void QDesignerActions::createForm()
{
    showNewFormDialog(QString());
}

void QDesignerActions::showNewFormDialog(const QString &fileName)
{
#ifdef NONMODAL_PREVIEW
    closePreview();
#endif
    NewForm *dlg = new NewForm(workbench(), workbench()->core()->topLevel(), fileName);
    dlg->setAttribute(Qt::WA_DeleteOnClose);
    dlg->setAttribute(Qt::WA_ShowModal);

    dlg->setGeometry(fixDialogRect(dlg->rect()));
    dlg->exec();
}

void QDesignerActions::slotOpenForm()
{
    openForm(core()->topLevel());
}

bool QDesignerActions::openForm(QWidget *parent)
{
#ifdef NONMODAL_PREVIEW
    closePreview();
#endif
    const QString extension = getFileExtension(core());
    const QStringList fileNames = QFileDialog::getOpenFileNames(parent, tr("Open Form"),
        m_openDirectory, tr("Designer UI files (*.%1);;All Files (*)").arg(extension), 0, QFileDialog::DontUseSheet);

    if (fileNames.isEmpty())
        return false;

    bool atLeastOne = false;
    foreach (QString fileName, fileNames) {
        if (readInForm(fileName) && !atLeastOne)
            atLeastOne = true;
    }

    return atLeastOne;
}

bool QDesignerActions::saveFormAs(QDesignerFormWindowInterface *fw)
{
    const QString extension = getFileExtension(core());

    QString dir = fw->fileName();
    if (dir.isEmpty()) {
        do {
            // Build untitled name
            if (!m_saveDirectory.isEmpty()) {
                dir = m_saveDirectory;
                break;
            }
            if (!m_openDirectory.isEmpty()) {
                dir = m_openDirectory;
                break;
            }
            dir = QDir::current().absolutePath();
        } while (false);
        dir += QDir::separator();
        dir += QLatin1String("untitled.");
        dir += extension;
    }

    QString saveFile;
    while (1) {
        saveFile = QFileDialog::getSaveFileName(fw, tr("Save form as"),
                dir,
                tr("Designer UI files (*.%1);;All Files (*)").arg(extension), 0, QFileDialog::DontConfirmOverwrite);
        if (saveFile.isEmpty())
            return false;

        const QFileInfo fInfo(saveFile);
        if (fInfo.suffix().isEmpty() && !fInfo.fileName().endsWith(QLatin1Char('.')))
            saveFile.append(QLatin1Char('.')).append(extension);

        const QFileInfo fi(saveFile);
        if (!fi.exists())
            break;

        if (QMessageBox::warning(fw, tr("Save"), tr("%1 already exists.\nDo you want to replace it?")
                    .arg(fi.fileName()), QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
            break;

        dir = saveFile;
    }

    fw->setFileName(saveFile);
    return writeOutForm(fw, saveFile);
}

void QDesignerActions::saveForm()
{
    if (QDesignerFormWindowInterface *fw = core()->formWindowManager()->activeFormWindow()) {
        if (saveForm(fw))
            showStatusBarMessage(savedMessage(fw->fileName()));
    }
}

void QDesignerActions::saveAllForms()
{
    QString fileNames;
    QDesignerFormWindowManagerInterface *formWindowManager = core()->formWindowManager();
    if (const int totalWindows = formWindowManager->formWindowCount()) {
        const QString separator = QLatin1String(", ");
        for (int i = 0; i < totalWindows; ++i) {
            QDesignerFormWindowInterface *fw = formWindowManager->formWindow(i);
            if (fw && fw->isDirty()) {
                formWindowManager->setActiveFormWindow(fw);
                if (saveForm(fw)) {
                    if (!fileNames.isEmpty())
                        fileNames += separator;
                    fileNames += QFileInfo(fw->fileName()).fileName();
                } else {
                    break;
                }
            }
        }
    }

    if (!fileNames.isEmpty()) {
        showStatusBarMessage(savedMessage(fileNames));
    }
}

bool QDesignerActions::saveForm(QDesignerFormWindowInterface *fw)
{
    bool ret;
    if (fw->fileName().isEmpty())
        ret = saveFormAs(fw);
    else
        ret =  writeOutForm(fw, fw->fileName());
    return ret;
}

void QDesignerActions::closeForm()
{
#ifdef NONMODAL_PREVIEW
    if (closePreview())
        return;
#endif

    if (QDesignerFormWindowInterface *fw = core()->formWindowManager()->activeFormWindow())
        if (QWidget *parent = fw->parentWidget()) {
            if (QMdiSubWindow *mdiSubWindow = qobject_cast<QMdiSubWindow *>(parent->parentWidget())) {
                mdiSubWindow->close();
            } else {
                parent->close();
            }
        }
}

void QDesignerActions::saveFormAs()
{
    if (QDesignerFormWindowInterface *fw = core()->formWindowManager()->activeFormWindow()) {
        if (saveFormAs(fw))
            showStatusBarMessage(savedMessage(fw->fileName()));
    }
}

void QDesignerActions::saveFormAsTemplate()
{
    if (QDesignerFormWindowInterface *fw = core()->formWindowManager()->activeFormWindow()) {
        SaveFormAsTemplate dlg(fw, fw->window());
        dlg.exec();
    }
}

void QDesignerActions::notImplementedYet()
{
    QMessageBox::information(core()->topLevel(), tr("Designer"), tr("Feature not implemented yet!"));
}

void QDesignerActions::previewFormLater(QAction *action)
{
    qRegisterMetaType<QAction*>("QAction*");
    QMetaObject::invokeMethod(this, "previewForm", Qt::QueuedConnection,
                                Q_ARG(QAction*, action));
}

bool QDesignerActions::closePreview()
{
    if (m_previewWidget) {
        m_previewWidget->close();
        return true;
    }
    return false;
}

void QDesignerActions::previewForm(QAction *action)
{
#ifdef NONMODAL_PREVIEW
    closePreview();
#endif
    QDesignerFormWindowInterface *fw = core()->formWindowManager()->activeFormWindow();
    if (!fw)
        return;

    // Get style stored in action if any
    QString styleName;
    if (action) {
        const QVariant data = action->data();
        if (data.type() == QVariant::String)
            styleName = data.toString();
    }

    // create and have form builder display errors.
    QWidget *widget =  qdesigner_internal::QDesignerFormBuilder::createPreview(fw, styleName);
    if (!widget)
        return;

#ifdef Q_WS_WIN
    Qt::WindowFlags windwowFlags = (widget->windowType() == Qt::Window) ? Qt::Window | Qt::WindowMaximizeButtonHint : Qt::WindowFlags(Qt::Dialog);
#else
    // Only Dialogs have close buttons on mac
    // On linux we don't want additional item on task bar and don't want minimize button, we want the preview to be on top
    Qt::WindowFlags windwowFlags = Qt::Dialog;
#endif
    // Install filter for Escape key
    widget->setParent(fw->window(), windwowFlags);

#ifdef NONMODAL_PREVIEW
    connect(fw, SIGNAL(changed()), widget, SLOT(close()));
#else
    // Cannot do this on the Mac as the dialog would have no close button
    widget->setWindowModality(Qt::ApplicationModal);
#endif

    // Position over form window
    widget->setAttribute(Qt::WA_DeleteOnClose, true);
    widget->move(fw->mapToGlobal(QPoint(10, 10)));

    widget->installEventFilter(this);
    widget->show();
    m_previewWidget = widget;

#ifdef NONMODAL_PREVIEW
    updateCloseAction();
#endif
}

void QDesignerActions::fixActionContext()
{
    QList<QAction*> actions;
    actions += m_fileActions->actions();
    actions += m_editActions->actions();
    actions += m_toolActions->actions();
    actions += m_formActions->actions();
    actions += m_windowActions->actions();
    actions += m_helpActions->actions();

    foreach (QAction *a, actions) {
        a->setShortcutContext(Qt::ApplicationShortcut);
    }
}

bool QDesignerActions::readInForm(const QString &fileName)
{
    QString fn = fileName;

    // First make sure that we don't have this one open already.
    QDesignerFormWindowManagerInterface *formWindowManager = core()->formWindowManager();
    const int totalWindows = formWindowManager->formWindowCount();
    for (int i = 0; i < totalWindows; ++i) {
        QDesignerFormWindowInterface *w = formWindowManager->formWindow(i);
        if (w->fileName() == fn) {
            w->raise();
            formWindowManager->setActiveFormWindow(w);
            addRecentFile(fn);
            return true;
        }
    }

    // Otherwise load it.
    do {
        QString errorMessage;
        if (workbench()->openForm(fn, &errorMessage)) {
            addRecentFile(fn);
            m_openDirectory = QFileInfo(fn).absolutePath();
            return true;
        } else {
            // prompt to reload
            QMessageBox box(QMessageBox::Warning, tr("Read error"),
                            tr("%1\nDo you want to update the file location or generate a new form?").arg(errorMessage),
                            QMessageBox::Cancel, core()->topLevel());

            QPushButton *updateButton = box.addButton(tr("&Update"), QMessageBox::ActionRole);
            QPushButton *newButton    = box.addButton(tr("&New Form"), QMessageBox::ActionRole);
            box.exec();
            if (box.clickedButton() == box.button(QMessageBox::Cancel))
                return false;

            if (box.clickedButton() == updateButton) {
                const QString extension = getFileExtension(core());
                fn = QFileDialog::getOpenFileName(core()->topLevel(),
                                                  tr("Open Form"), m_openDirectory,
                                                  tr("Designer UI files (*.%1);;All Files (*)").arg(extension), 0, QFileDialog::DontUseSheet);

                if (fn.isEmpty())
                    return false;
            } else if (box.clickedButton() == newButton) {
                // If the file does not exist, but its directory, is valid, open the template with the editor file name set to it.
                // (called from command line).
                QString newFormFileName;
                const  QFileInfo fInfo(fn);
                if (!fInfo.exists()) {
                    // Normalize file name
                    const QString directory = fInfo.absolutePath();
                    if (QDir(directory).exists()) {
                        newFormFileName = directory;
                        newFormFileName  += QLatin1Char('/');
                        newFormFileName  += fInfo.fileName();
                    }
                }
                showNewFormDialog(newFormFileName);
                return false;
            }
        }
    } while (true);
    return true;
}

static QString createBackup(const QString &fileName)
{
    const QString suffix = QLatin1String(".bak");
    QString backupFile = fileName + suffix;
    QFileInfo fi(backupFile);
    int i = 0;
    while (fi.exists()) {
        backupFile = fileName + suffix + QString::number(++i);
        fi.setFile(backupFile);
    }

    if (QFile::copy(fileName, backupFile))
        return backupFile;
    return QString();
}

static void removeBackup(const QString &backupFile)
{
    if (!backupFile.isEmpty())
        QFile::remove(backupFile);
}

bool QDesignerActions::writeOutForm(QDesignerFormWindowInterface *fw, const QString &saveFile)
{
    Q_ASSERT(fw && !saveFile.isEmpty());

    QString backupFile;
    QFileInfo fi(saveFile);
    if (fi.exists())
        backupFile = createBackup(saveFile);

    const QByteArray utf8Array = fw->contents().toUtf8();
    m_workbench->updateBackup(fw);

    QFile f(saveFile);
    while (!f.open(QFile::WriteOnly)) {
        QMessageBox box(QMessageBox::Warning,
                        tr("Save Form?"),
                        tr("Could not open file"),
                        QMessageBox::NoButton, fw);

        box.setWindowModality(Qt::WindowModal);
        box.setInformativeText(tr("The file, %1, could not be opened"
                               "\nReason: %2"
                               "\nWould you like to retry or change your file?")
                                .arg(f.fileName()).arg(f.errorString()));
        QPushButton *retryButton = box.addButton(QMessageBox::Retry);
        retryButton->setDefault(true);
        QPushButton *switchButton = box.addButton(tr("Select New File"), QMessageBox::AcceptRole);
        QPushButton *cancelButton = box.addButton(QMessageBox::Cancel);
        box.exec();

        if (box.clickedButton() == cancelButton) {
            removeBackup(backupFile);
            return false;
        } else if (box.clickedButton() == switchButton) {
            QString extension = getFileExtension(core());
            const QString fileName = QFileDialog::getSaveFileName(fw, tr("Save form as"),
                                                                  QDir::current().absolutePath(),
                                                                  QLatin1String("*.") + extension);
            if (fileName.isEmpty()) {
                removeBackup(backupFile);
                return false;
            }
            if (f.fileName() != fileName) {
                removeBackup(backupFile);
                fi.setFile(fileName);
                backupFile = QString();
                if (fi.exists())
                    backupFile = createBackup(fileName);
            }
            f.setFileName(fileName);
            fw->setFileName(fileName);
        }
        // loop back around...
    }
    while (f.write(utf8Array, utf8Array.size()) != utf8Array.size()) {
        QMessageBox box(QMessageBox::Warning, tr("Save Form?"),
                        tr("Could not write file"),
                        QMessageBox::Retry|QMessageBox::Cancel, fw);
        box.setWindowModality(Qt::WindowModal);
        box.setInformativeText(tr("It was not possible to write the entire file, %1, to disk."
                                "\nReason:%2\nWould you like to retry?")
                                .arg(f.fileName()).arg(f.errorString()));
        box.setDefaultButton(QMessageBox::Retry);
        switch (box.exec()) {
        case QMessageBox::Retry:
            f.resize(0);
            break;
        default:
            return false;
        }
    }
    f.close();
    removeBackup(backupFile);
    addRecentFile(saveFile);
    m_saveDirectory = QFileInfo(f).absolutePath();

    fw->setDirty(false);
    fw->parentWidget()->setWindowModified(false);
    return true;
}

void QDesignerActions::shutdown()
{

    // Follow the idea from the Mac, i.e. send the Application a close event
    // and if it's accepted, quit.
    QCloseEvent ev;
    QApplication::sendEvent(qDesigner, &ev);
    if (ev.isAccepted())
        qDesigner->quit();
}

void QDesignerActions::activeFormWindowChanged(QDesignerFormWindowInterface *formWindow)
{
    const bool enable = formWindow != 0;
#ifdef NONMODAL_PREVIEW
    closePreview();
#endif

    m_saveFormAction->setEnabled(enable);
    m_saveFormAsAction->setEnabled(enable);
    m_saveAllFormsAction->setEnabled(enable);
    m_saveFormAsTemplateAction->setEnabled(enable);
    m_closeFormAction->setEnabled(enable);
    m_closeFormAction->setEnabled(enable);

    m_editWidgetsAction->setEnabled(enable);
    m_formSettings->setEnabled(enable);

    m_previewFormAction->setEnabled(enable);
    m_styleActions->setEnabled(enable);
}

void QDesignerActions::updateRecentFileActions()
{
    QDesignerSettings settings;
    QStringList files = settings.recentFilesList();
    const int originalSize = files.size();
    int numRecentFiles = qMin(files.size(), int(MaxRecentFiles));
    const QList<QAction *> recentFilesActs = m_recentFilesActions->actions();

    for (int i = 0; i < numRecentFiles; ++i) {
        const QFileInfo fi(files[i]);
        // If the file doesn't exist anymore, just remove it from the list so
        // people don't get confused.
        if (!fi.exists()) {
            files.removeAt(i);
            --i;
            numRecentFiles = qMin(files.size(), int(MaxRecentFiles));
            continue;
        }
        const QString text = fi.fileName();
        recentFilesActs[i]->setText(text);
        recentFilesActs[i]->setIconText(files[i]);
        recentFilesActs[i]->setVisible(true);
    }

    for (int j = numRecentFiles; j < MaxRecentFiles; ++j)
        recentFilesActs[j]->setVisible(false);

    // If there's been a change, right it back
    if (originalSize != files.size())
        settings.setRecentFilesList(files);
}

void QDesignerActions::openRecentForm()
{
    if (const QAction *action = qobject_cast<const QAction *>(sender())) {
        if (!readInForm(action->iconText()))
            updateRecentFileActions(); // File doesn't exist, remove it from settings
    }
}

void QDesignerActions::clearRecentFiles()
{
    QDesignerSettings settings;
    settings.setRecentFilesList(QStringList());
    updateRecentFileActions();
}

QActionGroup *QDesignerActions::recentFilesActions() const
{
    return m_recentFilesActions;
}

void QDesignerActions::addRecentFile(const QString &fileName)
{
    QDesignerSettings settings;
    QStringList files = settings.recentFilesList();
    files.removeAll(fileName);
    files.prepend(fileName);
    while (files.size() > MaxRecentFiles)
        files.removeLast();

    settings.setRecentFilesList(files);
    updateRecentFileActions();
}

QAction *QDesignerActions::closeFormAction() const
{
    return m_closeFormAction;
}

QAction *QDesignerActions::minimizeAction() const
{
    return m_minimizeAction;
}

void QDesignerActions::showDesignerHelp()
{
    showHelp(QLatin1String("designer-manual.html"));
}

void QDesignerActions::showWhatsNew()
{
    showHelp(QLatin1String("qt4-designer.html"));
}

void QDesignerActions::showHelp(const QString &url)
{
    if (!m_assistantClient)
        m_assistantClient
            = new QAssistantClient(QLibraryInfo::location(QLibraryInfo::BinariesPath), this);

    QString filePath = QLibraryInfo::location(QLibraryInfo::DocumentationPath);
    filePath += QLatin1String("/html/");
    filePath += url;

    QString cleanFilePath(filePath);
    const int index = cleanFilePath.lastIndexOf(QLatin1Char('#'));
    if (index != -1)
         cleanFilePath.remove(index, cleanFilePath.size() - index);

    if (!QFile::exists(cleanFilePath)) {
        filePath = QLibraryInfo::location(QLibraryInfo::DocumentationPath);
        filePath += QLatin1String("/html/designer-manual.html");
    }

    m_assistantClient->showPage(filePath);
}

void QDesignerActions::aboutDesigner()
{
    VersionDialog mb(core()->topLevel());
    mb.setWindowTitle(tr("About Qt Designer"));
    if (mb.exec()) {
        OublietteView *oubliette = new OublietteView;
        oubliette->setAttribute(Qt::WA_DeleteOnClose);
        oubliette->setMinimumSize(800, 600);
        oubliette->show();
    }
}

QAction *QDesignerActions::preferencesAction() const
{
    return m_preferencesAction;
}

QAction *QDesignerActions::editWidgets() const
{
    return m_editWidgetsAction;
}

void QDesignerActions::showWidgetSpecificHelp()
{
    QDesignerFormWindowInterface *fw = core()->formWindowManager()->activeFormWindow();
    if (!fw) {
        showDesignerHelp();
        return;
    }

    QString className;
    const QString currentPropertyName = core()->propertyEditor()->currentPropertyName();
    if (!currentPropertyName.isEmpty()) {
        QDesignerPropertySheetExtension *ps
            = qt_extension<QDesignerPropertySheetExtension *>(core()->extensionManager(),
                                                            core()->propertyEditor()->object());
        if (!ps)
            ps = qt_extension<QDesignerPropertySheetExtension *>(core()->extensionManager(),
                                                            fw->cursor()->selectedWidget(0));
        Q_ASSERT(ps);
        className = ps->propertyGroup(ps->indexOf(currentPropertyName));
    } else {
        QDesignerWidgetDataBaseInterface *db = core()->widgetDataBase();
        QDesignerWidgetDataBaseItemInterface *dbi = db->item(db->indexOfObject(fw->cursor()->selectedWidget(0), true));
        className = dbi->name();
    }

    // ### generalize using the Widget Data Base
    if (className == QLatin1String("Line"))
        className = QLatin1String("QFrame");
    else if (className == QLatin1String("Spacer"))
        className = QLatin1String("QSpacerItem");
    else if (className == QLatin1String("QLayoutWidget"))
        className = QLatin1String("QLayout");

    QString url = className.toLower();

    // special case
    url += QLatin1String(".html");

    if (!currentPropertyName.isEmpty()) {
        url += QLatin1Char('#');
        url += currentPropertyName;
    }

    showHelp(url);
}

void QDesignerActions::aboutPlugins()
{
    PluginDialog dlg(core(), core()->topLevel());
    dlg.exec();
}

void QDesignerActions::showFormSettings()
{
    QDesignerFormWindowInterface *formWindow = core()->formWindowManager()->activeFormWindow();
    QDesignerFormWindow *window = m_workbench->findFormWindow(formWindow);

    QExtensionManager *mgr = core()->extensionManager();

    QDialog *settingsDialog = 0;

    if (QDesignerLanguageExtension *lang = qt_extension<QDesignerLanguageExtension*>(mgr, core()))
        settingsDialog = lang->createFormWindowSettingsDialog(formWindow, /*parent=*/ 0);

    if (! settingsDialog)
        settingsDialog = new FormWindowSettings(formWindow);

    QString title = QFileInfo(formWindow->fileName()).fileName();
    if (title.isEmpty())
        title = window->windowTitle();

    settingsDialog->setWindowTitle(tr("Form Settings - %1").arg(title));
    if (settingsDialog->exec() && window) {
        formWindow->setDirty(true);
        window->updateChanged();
    }

    delete settingsDialog;
}

bool QDesignerActions::eventFilter(QObject *watched, QEvent *event)
{
    do {
        if (!watched->isWidgetType())
            break;
        QWidget *previewWindow = qobject_cast<QWidget *>(watched);
        if (!previewWindow || !previewWindow->isWindow())
            break;

        switch (event->type()) {
        case QEvent::KeyPress:
        case QEvent::ShortcutOverride:        {
            const  QKeyEvent *keyEvent = static_cast<const QKeyEvent *>(event);
            const int key = keyEvent->key();
            if ((key == Qt::Key_Escape
#ifdef Q_WS_MAC
                 || (keyEvent->modifiers() == Qt::ControlModifier && key == Qt::Key_Period)
#endif
                 )) {
                previewWindow->close();
                return true;
            }
        }
            break;
#ifdef NONMODAL_PREVIEW
        case QEvent::Close:
            previewWindow->removeEventFilter(this);
            m_previewWidget = 0;
            updateCloseAction();
            break;
#endif
        default:
            break;
        }
    } while(false);
    return QObject::eventFilter(watched, event);
}

void QDesignerActions::updateCloseAction()
{
    if (m_previewWidget) {
        m_closeFormAction->setText(tr("&Close Preview"));
    } else {
        m_closeFormAction->setText(tr("&Close Form"));
    }
}

void QDesignerActions::backupForms()
{
    const int count = m_workbench->formWindowCount();
    if (!count || !ensureBackupDirectories())
        return;


    QStringList tmpFiles;
    QMap<QString, QString> backupMap;
    QDir backupDir(m_backupPath);
    const bool warningsEnabled = qdesigner_internal::QSimpleResource::setWarningsEnabled(false);
    for (int i = 0; i < count; ++i) {
        QDesignerFormWindow *fw = m_workbench->formWindow(i);
        QDesignerFormWindowInterface *fwi = fw->editor();

        QString formBackupName;
        QTextStream(&formBackupName) << m_backupPath << QDir::separator()
                                     << QLatin1String("backup") << i << QLatin1String(".bak");

        QString fwn = QDir::convertSeparators(fwi->fileName());
        if (fwn.isEmpty())
            fwn = fw->windowTitle();

        backupMap.insert(fwn, formBackupName);

        QFile file(formBackupName.replace(m_backupPath, m_backupTmpPath));
        if (file.open(QFile::WriteOnly)){
            const QByteArray utf8Array = fixResourceFileBackupPath(fwi, backupDir).toUtf8();
            if (file.write(utf8Array, utf8Array.size()) != utf8Array.size()) {
                backupMap.remove(fwn);
                qdesigner_internal::designerWarning(QObject::tr("The backup file %1 could not be written.").arg(file.fileName()));
            } else
                tmpFiles.append(formBackupName);

            file.close();
        }
    }
    qdesigner_internal::QSimpleResource::setWarningsEnabled(warningsEnabled);
    if(!tmpFiles.isEmpty()) {
        const QStringList backupFiles = backupDir.entryList(QDir::Files);
        if(!backupFiles.isEmpty()) {
            QStringListIterator it(backupFiles);
            while (it.hasNext())
                backupDir.remove(it.next());
        }

        QStringListIterator it(tmpFiles);
        while (it.hasNext()) {
            const QString tmpName = it.next();
            QString name(tmpName);
            name.replace(m_backupTmpPath, m_backupPath);
            QFile tmpFile(tmpName);
            if (!tmpFile.copy(name))
                qdesigner_internal::designerWarning(QObject::tr("The backup file %1 could not be written.").arg(name));
            tmpFile.remove();
        }

        QDesignerSettings().setBackup(backupMap);
    }
}

QString QDesignerActions::fixResourceFileBackupPath(QDesignerFormWindowInterface *fwi, const QDir& backupDir)
{
    const QString content = fwi->contents();
    QDomDocument domDoc(QLatin1String("backup"));
    if(!domDoc.setContent(content))
        return content;

    const QDomNodeList list = domDoc.elementsByTagName(QLatin1String("resources"));
    if (list.isEmpty())
        return content;

    for (int i = 0; i < list.count(); i++) {
        const QDomNode node = list.at(i);
        if (!node.isNull()) {
            const QDomElement element = node.toElement();
            if(!element.isNull() && element.tagName() == QLatin1String("resources")) {
                QDomNode childNode = element.firstChild();
                while (!childNode.isNull()) {
                    QDomElement childElement = childNode.toElement();
                    if(!childElement.isNull() && childElement.tagName() == QLatin1String("include")) {
                        const QString attr = childElement.attribute(QLatin1String("location"));
                        const QString path = fwi->absoluteDir().absoluteFilePath(attr);
                        childElement.setAttribute(QLatin1String("location"), backupDir.relativeFilePath(path));
                    }
                    childNode = childNode.nextSibling();
                }
            }
        }
    }

    return domDoc.toString();
}

QRect QDesignerActions::fixDialogRect(const QRect &rect) const
{
    QRect frameGeometry;
    const QRect availableGeometry = QApplication::desktop()->availableGeometry(core()->topLevel());

    if (workbench()->mode() == DockedMode) {
        frameGeometry = core()->topLevel()->frameGeometry();
    } else
        frameGeometry = availableGeometry;

    QRect dlgRect = rect;
    dlgRect.moveCenter(frameGeometry.center());

    // make sure that parts of the dialog are not outside of screen
    dlgRect.moveBottom(qMin(dlgRect.bottom(), availableGeometry.bottom()));
    dlgRect.moveRight(qMin(dlgRect.right(), availableGeometry.right()));
    dlgRect.moveLeft(qMax(dlgRect.left(), availableGeometry.left()));
    dlgRect.moveTop(qMax(dlgRect.top(), availableGeometry.top()));

    return dlgRect;
}

void QDesignerActions::showStatusBarMessage(const QString &message) const
{
    if (workbench()->mode() == DockedMode) {
        QStatusBar *bar = qDesigner->mainWindow()->statusBar();
        if (bar && !bar->isHidden())
            bar->showMessage(message, 3000);
    }
}

void QDesignerActions::setBringAllToFrontVisible(bool visible)
{
      m_bringAllToFrontSeparator->setVisible(visible);
      m_bringAllToFrontAction->setVisible(visible);
}

void QDesignerActions::setWindowListSeparatorVisible(bool visible)
{
    m_windowListSeparatorAction->setVisible(visible);
}

bool QDesignerActions::ensureBackupDirectories() {

    if (m_backupPath.isEmpty()) {
        // create names
        m_backupPath = QDir::homePath();
        m_backupPath += QDir::separator();
        m_backupPath += QLatin1String(".designer");
        m_backupPath += QDir::separator();
        m_backupPath += QLatin1String("backup");
        m_backupPath = QDir::convertSeparators(m_backupPath );

        m_backupTmpPath = m_backupPath;
        m_backupTmpPath += QDir::separator();
        m_backupTmpPath += QLatin1String("tmp");
        m_backupTmpPath = QDir::convertSeparators(m_backupTmpPath);
    }

    // ensure directories
    const QDir backupDir(m_backupPath);
    const QDir backupTmpDir(m_backupTmpPath);

    if (!backupDir.exists()) {
        if (!backupDir.mkpath(m_backupPath)) {
            qdesigner_internal::designerWarning(QObject::tr("The backup directory %1 could not be created.").arg(m_backupPath));
            return false;
        }
    }
    if (!backupTmpDir.exists()) {
        if (!backupTmpDir.mkpath(m_backupTmpPath)) {
            qdesigner_internal::designerWarning(QObject::tr("The temporary backup directory %1 could not be created.").arg(m_backupTmpPath));
            return false;
        }
    }
    return true;
}

void QDesignerActions::showPreferencesDialog()
{
    QDesignerSettings settings;
    Preferences preferences = settings.preferences();

    { // It is important that the dialog be deleted before UI mode changes.
        PreferencesDialog preferencesDialog(workbench()->core()->topLevel());
        if (!preferencesDialog.showDialog(preferences))  {
            return;
        }
    }

    settings.setPreferences(preferences);
    m_workbench->applyPreferences(preferences);
}
