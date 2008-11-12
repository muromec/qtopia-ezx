/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the Qt Linguist of the Qt Toolkit.
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

/*  TRANSLATOR TrWindow

  This is the application's main window.
*/

#include "trwindow.h"
#include "finddialog.h"
#include "translatedialog.h"
#include "batchtranslationdialog.h"
#include "translationsettingsdialog.h"
#include "msgedit.h"
#include "phrasebookbox.h"
#include "printout.h"
#include "statistics.h"
#include "messagemodel.h"
#include "phrasemodel.h"
#include "translator.h"
#include "previewtool/trpreviewtool.h"

#include <QAction>
#include <QApplication>
#include <QBitmap>
#include <QDockWidget>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QLabel>
#include <QLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QMenu>
#include <QRegExp>
#include <QSettings>
#include <QStatusBar>
#include <QToolBar>
#include <QWhatsThis>
#include <QAssistantClient>
#include <QDesktopWidget>
#include <QPrintDialog>
#include <QLibraryInfo>
#include <QUiLoader>
#include <QTranslator>

#define pagecurl_mask_width 53
#define pagecurl_mask_height 51
static const uchar pagecurl_mask_bits[] = {
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0x0f, 0x80, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x00, 0xfe, 0xff,
   0xff, 0xff, 0xff, 0x0f, 0x00, 0xf0, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x00,
   0xc0, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x00, 0x80, 0xff, 0xff, 0xff, 0xff,
   0x0f, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x00, 0x00, 0xfe, 0xff,
   0xff, 0xff, 0x0f, 0x00, 0x00, 0xfc, 0xff, 0xff, 0xff, 0x0f, 0x00, 0x00,
   0xfc, 0xff, 0xff, 0xff, 0x0f, 0x00, 0x00, 0xf8, 0xff, 0xff, 0xff, 0x0f,
   0x00, 0x00, 0xf0, 0xff, 0xff, 0xff, 0x0f, 0x00, 0x00, 0xf0, 0xff, 0xff,
   0xff, 0x0f, 0x00, 0x00, 0xf0, 0xff, 0xff, 0xff, 0x0f, 0x00, 0x00, 0xf0,
   0xff, 0xff, 0xff, 0x0f, 0x00, 0x00, 0xe0, 0xff, 0xff, 0xff, 0x0f, 0x00,
   0x00, 0xe0, 0xff, 0xff, 0xff, 0x0f, 0x00, 0x00, 0xe0, 0xff, 0xff, 0xff,
   0x0f, 0x00, 0x00, 0xe0, 0xff, 0xff, 0xff, 0x0f, 0x00, 0x00, 0xe0, 0xff,
   0xff, 0xff, 0x0f, 0x00, 0x00, 0xe0, 0xff, 0xff, 0xff, 0x0f, 0x00, 0x00,
   0xe0, 0xff, 0xff, 0xff, 0x0f, 0x00, 0x00, 0xe0, 0xff, 0xff, 0xff, 0x0f,
   0x00, 0x00, 0xe0, 0xff, 0xff, 0xff, 0x0f, 0x00, 0x00, 0xe0, 0xff, 0xff,
   0xff, 0x0f, 0x00, 0x00, 0xe0, 0xff, 0xff, 0xff, 0x0f, 0x00, 0x00, 0xe0,
   0xff, 0xff, 0xff, 0x0f, 0x00, 0x00, 0xe0, 0xff, 0xff, 0xff, 0x0f, 0x00,
   0x00, 0xe0, 0xff, 0xff, 0xff, 0x0f, 0x00, 0x00, 0xc0, 0xff, 0xff, 0xff,
   0x0f, 0x00, 0x00, 0xc0, 0xff, 0xff, 0xff, 0x0f, 0x00, 0x00, 0x00, 0xfc,
   0xff, 0xff, 0x0f, 0x00, 0x00, 0x00, 0x00, 0xf8, 0xff, 0x0f, 0x00, 0x00,
   0x00, 0x00, 0x00, 0xfc, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x0f,
   0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00,
   0xe0, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x0f, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08 };

static const int ErrorMS = 600000; // for error messages
static const int MessageMS = 2500;

QPixmap *TrWindow::pxOn = 0;
QPixmap *TrWindow::pxOff = 0;
QPixmap *TrWindow::pxObsolete = 0;
QPixmap *TrWindow::pxDanger = 0;
QPixmap *TrWindow::pxWarning = 0;
QPixmap *TrWindow::pxEmpty = 0;

enum Ending {End_None, End_FullStop, End_Interrobang, End_Colon,
              End_Ellipsis};

static Ending ending(QString str, QLocale::Language lang)
{
    str = str.simplified();
    int ch = 0;
    if (!str.isEmpty())
        ch = str.right(1)[0].unicode();

    switch (ch) {
    case 0x002e: // full stop
        if (str.endsWith(QString(QLatin1String("..."))))
            return End_Ellipsis;
        else
            return End_FullStop;
    case 0x0589: // armenian full stop
    case 0x06d4: // arabic full stop
    case 0x3002: // ideographic full stop
        return End_FullStop;
    case 0x0021: // exclamation mark
    case 0x003f: // question mark
    case 0x00a1: // inverted exclamation mark
    case 0x00bf: // inverted question mark
    case 0x01c3: // latin letter retroflex click
    case 0x037e: // greek question mark
    case 0x061f: // arabic question mark
    case 0x203c: // double exclamation mark
    case 0x203d: // interrobang
    case 0x2048: // question exclamation mark
    case 0x2049: // exclamation question mark
    case 0x2762: // heavy exclamation mark ornament
        return End_Interrobang;
    case 0x003b: // greek 'compatibility' questionmark
        return lang == QLocale::Greek ? End_Interrobang : End_None;
    case 0x003a: // colon
        return End_Colon;
    case 0x2026: // horizontal ellipsis
        return End_Ellipsis;
    default:
        return End_None;
    }
}

const QPixmap TrWindow::pageCurl()
{
    QPixmap pixmap;
    pixmap.load(QLatin1String(":/images/pagecurl.png") );
    if ( !pixmap.isNull() ) {
        QBitmap pageCurlMask = QBitmap::fromData(QSize(pagecurl_mask_width, pagecurl_mask_height),
                                                 pagecurl_mask_bits, QImage::Format_MonoLSB);
        pixmap.setMask(pageCurlMask);
    }

    return pixmap;
}

#ifdef Q_WS_MAC
const QString rsrcString = QLatin1String(":/images/mac");
#else
const QString rsrcString = QLatin1String(":/images/win");
#endif

TrWindow::TrWindow()
    : QMainWindow(0, Qt::Window)
{
    ac = 0;

#ifndef Q_WS_MAC
    setWindowIcon(QPixmap(QLatin1String(":/images/appicon.png") ));
#endif

    m_previewTool = 0;

    // Create the application global listview symbols
    pxOn  = new QPixmap(QLatin1String(":/images/s_check_on.png"));
    pxOff = new QPixmap(QLatin1String(":/images/s_check_off.png"));
    pxObsolete = new QPixmap(QLatin1String(":/images/s_check_obsolete.png"));
    pxDanger = new QPixmap(QLatin1String(":/images/s_check_danger.png"));
    pxWarning = new QPixmap(QLatin1String(":/images/s_check_warning.png"));
    pxEmpty = new QPixmap(QLatin1String(":/images/s_check_empty.png"));

    setCorner(Qt::TopLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::TopRightCorner, Qt::RightDockWidgetArea);
    setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
    setCorner(Qt::BottomRightCorner, Qt::RightDockWidgetArea);

    // Create the ausiliary translator
    altTranslatorModel = new MessageModel(this);

    // Set up the Scope dock window
    dwScope = new QDockWidget(this);
    dwScope->setObjectName(QLatin1String("ContextDockWidget"));
    dwScope->setAllowedAreas(Qt::AllDockWidgetAreas);
    dwScope->setFeatures(QDockWidget::AllDockWidgetFeatures);
    dwScope->setWindowTitle(tr("Context"));

    tv = new MessagesTreeView(dwScope);
    cmdl = new MessageModel(dwScope);
    tv->setModel(cmdl);
    tv->setAllColumnsShowFocus(true);
    dwScope->setWidget(tv);
    addDockWidget(Qt::LeftDockWidgetArea, dwScope);

    me = new MessageEditor(cmdl, altTranslatorModel, this);
    ptv = me->phraseView();
    pmdl = qobject_cast<PhraseModel *>(ptv->model());

    connect(tv->selectionModel(), SIGNAL(currentChanged(QModelIndex,QModelIndex)),
             this, SLOT(showNewCurrent(QModelIndex,QModelIndex)));

    connect(cmdl, SIGNAL(languageChanged(QLocale::Language)),
        this, SLOT(updateLanguage(QLocale::Language)));

    m_translatedlg = new TranslateDialog(this);
    m_batchTranslateDlg = new BatchTranslationDialog(cmdl, this);
    m_translationSettingsDialog = 0;
    finddlg = new FindDialog(this);
    findMatchCase = false;
    findWhere = 0;
    foundWhere = 0;
    foundOffset = 0;

    setupMenuBar();
    setupToolBars();
    // We can't call setCentralWidget(me), since it is already called in m_ui.setupUi()
    QBoxLayout *lout = new QBoxLayout(QBoxLayout::TopToBottom, m_ui.centralwidget);
    lout->addWidget(me);
    lout->setMargin(0);
    m_ui.centralwidget->setLayout(lout);

    progress = new QLabel(statusBar());
    statusBar()->addPermanentWidget(progress);
    modified = new QLabel(QString::fromLatin1(" %1 ").arg(tr("MOD")), statusBar());
    statusBar()->addPermanentWidget(modified);

    updateProgress();
    updateCaption();


    connect(tv, SIGNAL(clicked(QModelIndex)),
        this, SLOT(toggleFinished(QModelIndex)));
    connect(me, SIGNAL(translationChanged(QStringList)),
        this, SLOT(updateTranslation(QStringList)));
    connect(me, SIGNAL(finished(bool)), this, SLOT(updateFinished(bool)));
    connect(me, SIGNAL(prevUnfinished()), this, SLOT(prevUnfinished()));
    connect(me, SIGNAL(nextUnfinished()), this, SLOT(nextUnfinished()));
    connect(me, SIGNAL(focusSourceList()), this, SLOT(focusSourceList()));
    connect(me, SIGNAL(focusPhraseList()), this, SLOT(focusPhraseList()));
    connect(finddlg, SIGNAL(findNext(QString,int,bool)),
        this, SLOT(findNext(QString,int,bool)));
    connect(m_translatedlg, SIGNAL( translateAndFindNext(QString,QString,int,int,bool) ),
        this, SLOT( translateAndFindNext(QString,QString,int,int,bool) ));

    connect(tv->header(), SIGNAL(sectionClicked(int)),
        tv, SLOT(clearSelection()));

    tv->setWhatsThis(tr("This panel lists the source contexts."));

    QSize as( qApp->desktop()->size() );
    as -= QSize( 30, 30 );
    resize( QSize( 1000, 800 ).boundedTo( as ) );
    readConfig();
    stats = 0;

    QWidget::setTabOrder(ptv, tv);
}

void TrWindow::updateLanguage(QLocale::Language lang)
{
    QStringList forms;
    QByteArray rules;

    getNumerusInfo(lang, cmdl->country(), &rules, &forms);
    me->setNumerusForms(tr("Translation"), forms);
}

TrWindow::~TrWindow()
{
    writeConfig();
    cmdl->clearContextList();
    delete stats;
    delete m_previewTool;
}

void TrWindow::openFile( const QString& name )
{
    if (name.isEmpty())
        return;

    statusBar()->showMessage(tr("Loading..."));
    qApp->processEvents();

    if (!cmdl->load(name)) {
        statusBar()->clearMessage();
        QMessageBox::warning(this, tr("Qt Linguist"), tr("Cannot open '%1'.").arg(name));
        return;
    }

    QString lang = QLocale::languageToString(cmdl->language());
    me->setTranslationLabel(QString(tr("%1 translation")).arg(lang));

    MessageItem *m;
    for (MessageModel::iterator it = cmdl->begin() ; (m = it.current()) ; ++it) {
        updateDanger(m);
    }

    tv->clearSelection();

    setEnabled(true);
    updateProgress();
    filename = name;
    
    updateCaption();

    me->showNothing();

    m_ui.actionDoneAndNext->setEnabled(false);
    m_ui.actionPreviewForm->setEnabled(true);

    statusBar()->showMessage(tr("%n source phrase(s) loaded.", 0, cmdl->getMessageCount()), MessageMS);
    foundWhere = 0;
    foundOffset = 0;

    if (cmdl->contextsInList() > 0) {
        m_ui.actionFind->setEnabled(true);
        m_ui.actionFindNext->setEnabled(false);
        m_ui.actionTranslationFileSettings->setEnabled(true);
        m_ui.actionBatchTranslation->setEnabled(true);
        m_ui.actionSearchAndTranslate->setEnabled(true);
    }

    addRecentlyOpenedFile(name, recentFiles);
    updateStatistics();
}

void TrWindow::open()
{
    if (maybeSave()) {
        QString newFilename = QFileDialog::getOpenFileName( this, QString(), filename,
            tr("Qt translation source (*.ts)\nXLIFF localization file (*.xlf)\nAll files (*)"));
        openFile(newFilename);
    }
}

void TrWindow::openAltSource()
{
    QString newFilename = QFileDialog::getOpenFileName( this, QString(), filename,
            tr("Qt alternative translation source (*.ts)\nXLIFF localization file (*.xlf)\nAll files (*)"));
    openAltSource(newFilename);
}

void TrWindow::resetAltSource()
{
    altTranslatorModel->init();
    altTranslatorModel->clearContextList();
    altTranslatorModel->updateAll();
    m_ui.actionResetAltSource->setEnabled(false);
    showMessages(tv->currentIndex());
}

void TrWindow::openAltSource( const QString& name )
{
    if (name.isEmpty())
        return;
    statusBar()->showMessage(tr("Loading alternative translation..."));
    qApp->processEvents();

    if (!altTranslatorModel->load(name)) {
        statusBar()->clearMessage();
        QMessageBox::warning(this, tr("Qt Linguist"), tr("Cannot open '%1'.").arg(name));
        return;
    }
    statusBar()->showMessage(tr("%n alternative translation(s) loaded.", 0,
        altTranslatorModel->getMessageCount()), MessageMS);
    m_ui.actionResetAltSource->setEnabled(true);
    QString lang = QLocale::languageToString(altTranslatorModel->language());
    me->setAltTextLabel(QString(tr("Existing %1 translation")).arg(lang));
    showMessages(tv->currentIndex());
}

void TrWindow::save()
{
    if (filename.isEmpty())
        return;

    if (cmdl->save(filename)) {
        updateCaption();
        statusBar()->showMessage(tr("File saved."), MessageMS);
    } else {
        QMessageBox::warning(this, tr("Qt Linguist"), tr("Cannot save '%1'.").arg(filename));
    }
}

void TrWindow::saveAs()
{
    QString newFilename = QFileDialog::getSaveFileName(this, QString(), filename,
        tr( "Qt translation source (*.ts)\nXLIFF localization file (*.xlf)\nAll files (*)"));
    if (!newFilename.isEmpty()) {
        filename = newFilename;
        save();
        updateCaption();
    }
}

void TrWindow::releaseAs()
{
    QString newFilename = filename;
    newFilename.replace(QRegExp(QLatin1String(".ts$")), QLatin1String(""));
    newFilename += QString(QLatin1String(".qm"));

    newFilename = QFileDialog::getSaveFileName(this, tr("Release"), newFilename,
        tr("Qt message files for released applications (*.qm)\nAll files (*)"));
    if (!newFilename.isEmpty()) {
        if (cmdl->release(newFilename, false, false, Translator::Everything))
            statusBar()->showMessage(tr("File created."), MessageMS);
        else
            QMessageBox::warning(this, tr("Qt Linguist"), tr("Cannot save '%1'.").arg(newFilename));
    }
}

// No-question
void TrWindow::release()
{
    QString newFilename = filename;
    newFilename.replace(QRegExp(QLatin1String(".ts$")), QLatin1String(""));
    newFilename += QString(QLatin1String(".qm"));

    if (!newFilename.isEmpty()) {
        if (cmdl->release(newFilename, false, false, Translator::Everything))
            statusBar()->showMessage(tr("File created."), MessageMS);
        else
            QMessageBox::warning(this, tr("Qt Linguist"), tr("Cannot save '%1'.").arg(newFilename));
    }
}

void TrWindow::print()
{
    int pageNum = 0;
    QList <ContextItem *> ctxtList;
    QList <MessageItem *> msgList;
    const MessageItem *m;
    ContextItem *c;

    QPrintDialog dlg(&printer, this);
    if (dlg.exec()) {
        QApplication::setOverrideCursor(Qt::WaitCursor);
        printer.setDocName(filename);
        statusBar()->showMessage(tr("Printing..."));
        PrintOut pout(&printer);
        ctxtList = cmdl->contextList();

        for (int i=0; i<ctxtList.count(); i++) {
            c = ctxtList.at(i);
            pout.vskip();
            pout.setRule(PrintOut::ThickRule);
            pout.setGuide(c->context());
            pout.addBox(100, tr("Context: %1").arg(c->context()),
                PrintOut::Strong);
            pout.flushLine();
            pout.addBox(4);
            pout.addBox(92, c->comment(), PrintOut::Emphasis);
            pout.flushLine();
            pout.setRule(PrintOut::ThickRule);

            msgList = c->messageItemList();
            for (int j=0; j<msgList.count(); j++) {
                m = msgList.at(j);
                pout.setRule(PrintOut::ThinRule);

                QString type;
                switch (m->message().type()) {
                case MetaTranslatorMessage::Finished:
                    type = tr("finished");
                    break;
                case MetaTranslatorMessage::Unfinished:
                    type = m->danger() ? tr("unresolved") : QLatin1String("unfinished");
                    break;
                case MetaTranslatorMessage::Obsolete:
                    type = tr("obsolete");
                    break;
                default:
                    type = QString(QLatin1String(""));
                }
                pout.addBox(40, m->sourceText());
                pout.addBox(4);
                if (m->message().isPlural() && cmdl->language() != QLocale::C) {
                    QStringList transls = m->translations();
                    pout.addBox(40, transls.join(QLatin1String("\n")));
                } else {
                    pout.addBox(40, m->translation());
                }
                pout.addBox(4);
                pout.addBox(12, type, PrintOut::Normal, Qt::AlignRight);
                if (!m->comment().isEmpty()) {
                    pout.flushLine();
                    pout.addBox(4);
                    pout.addBox(92, m->comment(), PrintOut::Emphasis);
                }
                pout.flushLine(true);

                if (pout.pageNum() != pageNum) {
                    pageNum = pout.pageNum();
                    statusBar()->showMessage(tr("Printing... (page %1)")
                        .arg(pageNum));
                }
            }
        }
        pout.flushLine(true);
        QApplication::restoreOverrideCursor();
        statusBar()->showMessage(tr("Printing completed"), MessageMS);
    } else {
        statusBar()->showMessage(tr("Printing aborted"), MessageMS);
    }
}

void TrWindow::find()
{
    finddlg->show();
    finddlg->activateWindow();
    finddlg->raise();
}

void TrWindow::findAgain()
{
    if (cmdl->contextsInList() <= 0)
        return;

    int pass = 0;
    int scopeNo = 0;
    int itemNo = 0;


    QModelIndex indxItem = tv->currentIndex();
    if (indxItem.isValid())
        itemNo = indxItem.row();
    QModelIndex indxScope = tv->currentIndex().parent();
    if (indxScope.isValid())
        scopeNo = indxScope.row();

    QString delayedMsg;

    //scopeNo = foundScope;
    ContextItem *c = cmdl->contextItem(cmdl->index(scopeNo, 1));
    MessageItem *m; // = c->messageItem(foundItem);

#if 1
    /*
      As long as we don't implement highlighting of the text in the QTextView,
      we may have only one match per message.
    */
    foundOffset = (int) 0x7fffffff;
#else
    foundOffset++;
#endif

    // We want to search the scope we started from *again*, since we did not necessarily search that *completely* when we started.
    // (Problaby we started somewhere in the middle of it.)
    // Therefore, "pass <=" and not "pass < " 
    while (pass <= cmdl->contextsInList()) {
        for (int mit = itemNo; mit < c->messageItemsInList() ; ++mit) {
            m = c->messageItem(mit);
            switch (foundWhere) {
                case 0:
                    foundWhere = FindDialog::SourceText;
                    foundOffset = 0;
                    // fall-through
                case FindDialog::SourceText:
                    if (searchItem(m->sourceText(), scopeNo, mit)) {
                        finddlg->hide();
                        if (!delayedMsg.isEmpty())
                            statusBar()->showMessage(delayedMsg, MessageMS);
                        return;
                    }
                    foundWhere = FindDialog::Translations;
                    foundOffset = 0;
                    // fall-through
                case FindDialog::Translations:
                    if (searchItem(m->translation(), scopeNo, mit)) {
                        finddlg->hide();
                        if (!delayedMsg.isEmpty())
                            statusBar()->showMessage(delayedMsg, MessageMS);
                        return;
                    }
                    foundWhere = FindDialog::Comments;
                    foundOffset = 0;
                    // fall-through
                case FindDialog::Comments:
                    if (searchItem(m->comment(), scopeNo, mit)) {
                        finddlg->hide();
                        if (!delayedMsg.isEmpty())
                            statusBar()->showMessage(delayedMsg, MessageMS);
                        return;
                    }
                    foundWhere = 0;
                    foundOffset = 0;
            }
        }
        itemNo = 0;
        ++pass;

        ++scopeNo;
        if (scopeNo >= cmdl->contextsInList()) {
            scopeNo = 0;
            delayedMsg = tr("Search wrapped.");
        }

        c = cmdl->contextItem(cmdl->index(scopeNo, 1));
    }

    qApp->beep();
    QMessageBox::warning( finddlg, tr("Qt Linguist"),
                          QString( tr("Cannot find the string '%1'.") ).arg(findText));
//    foundItem   = 0;
    foundWhere  = 0;
    foundOffset = 0;
}

void TrWindow::showTranslateDialog()
{
    m_translatedlg->show();
    m_translatedlg->activateWindow();
    m_translatedlg->raise();
}

void TrWindow::showBatchTranslateDialog()
{
    QList<PhraseBook> frasebooks = phraseBooks[PhraseEditMenu].values();
    m_batchTranslateDlg->setPhraseBooks(frasebooks);
    m_batchTranslateDlg->exec();
    
}

void TrWindow::translateAndFindNext(const QString& findWhat, const QString &translateTo, 
                                    int matchOption, int mode, bool markFinished)
{
    findText = findWhat;
    m_translateTo = translateTo;
    findMatchCase = matchOption & TranslateDialog::MatchCase;
    m_markFinished = markFinished;
    m_findMatchSubstring = false;

    translate(mode);
}

void TrWindow::translate(int mode)
{
    int itemNo = 0;
    int scopeNo = 0;
    QModelIndex indxItem = tv->currentIndex();
    if (indxItem.isValid())
        itemNo = indxItem.row();      // The for-loop condition for the ContextItem will rule this potential overflow on itemNo
    QModelIndex indxScope = tv->currentIndex().parent();
    if (indxScope.isValid()) {
        scopeNo = indxScope.row();
    }else{
        scopeNo = itemNo;
        itemNo = 0;
    }

    int translatedCount = 0;
    bool found = false;


    MessageModel::iterator it = cmdl->Iterator(scopeNo, itemNo);
    switch (mode) {
    case TranslateDialog::TranslateAll:
        {
            int passes = cmdl->getMessageCount();
            while (passes > 0) {
                MessageItem *m = it.current();
                if (!m) {
                    it.reset();
                    m = it.current();
                }
                if (m && m->compare(findText, m_findMatchSubstring, findMatchCase ? Qt::CaseSensitive : Qt::CaseInsensitive)) {
                    cmdl->setTranslation(it, m_translateTo);
                    if (indxScope.isValid() && it.contextNo() == scopeNo && it.messageNo() == itemNo)
                        me->setTranslation(m_translateTo, 0, false);
                    updateFinished(it.contextNo(), it.messageNo(), m_markFinished);
                    ++translatedCount;
                }
                ++it;
                --passes;
            }
            found = translatedCount == 0 ? false : true;
            if (found) {
                QMessageBox::warning( m_translatedlg, tr("Translate"),
                                  QString( tr("Translated %n entries to '%1'", 0, translatedCount).arg(m_translateTo) ));
            }
        }
        break;
    case TranslateDialog::Translate:
        {
            MessageItem *m = it.current();
            if (m && m->compare(findText, m_findMatchSubstring, findMatchCase ? Qt::CaseSensitive : Qt::CaseInsensitive)) {
                cmdl->setTranslation(it, m_translateTo);
                updateFinished(it.contextNo(), it.messageNo(), m_markFinished);
                ++translatedCount;
            }
        }
    case TranslateDialog::Skip:
        {
            ++it;
            int passes = cmdl->getMessageCount() - 1;
            while (passes > 0) {
                MessageItem *m = it.current();
                if (!m) {
                    it.reset();
                    m = it.current();
                }
                if (m && m->compare(findText, m_findMatchSubstring, findMatchCase ? Qt::CaseSensitive : Qt::CaseInsensitive)) {
                    found = true;
                    break;
                }
                ++it;
                --passes;
            }
        }
        break;
    }

    if (found) {
        QModelIndex cidx = cmdl->index(it.contextNo(),0);
        if (cidx.isValid()) {
            setCurrentMessage(cmdl->index(it.messageNo(),0, cidx));
        }
    } else {
        qApp->beep();
        QMessageBox::warning( m_translatedlg, tr("Qt Linguist"),
                              QString( tr("Cannot find the string '%1'.") ).arg(findText) );
    }
}

bool TrWindow::searchItem(const QString &searchWhat, int c, int m)
{
    if ((findWhere & foundWhere) != 0) {
        foundOffset = searchWhat.indexOf(findText, foundOffset,
            findMatchCase ? Qt::CaseSensitive : Qt::CaseInsensitive);
        if (foundOffset >= 0) {
            QModelIndex cidx = cmdl->index(c,0);
            setCurrentMessage(cmdl->index(m,0, cidx));
            return true;
        }
    }
    foundOffset = 0;
    return false;
}

void TrWindow::newPhraseBook()
{
    QString name;
    for (;;) {
        name = QFileDialog::getSaveFileName(this, tr("Create New Phrase Book"),
            QString(), tr("Qt phrase books (*.qph)\nAll files (*)"));
        if (name.isEmpty())
            break;
        else if (!QFile::exists(name)) {
            break;
            QMessageBox::warning(this, tr("Qt Linguist"),
                tr("A file called '%1' already exists."
                "  Please choose another name.").arg(name));
        }
    }
    if (!name.isEmpty()) {
        PhraseBook pb;
        if (savePhraseBook(name, pb)) {
            if (openPhraseBook(name))
                statusBar()->showMessage(tr("Phrase book created."), MessageMS);
        }
    }
}

bool TrWindow::phraseBooksContains(QString name)
{
    foreach(PhraseBook pb, phraseBooks[PhraseCloseMenu]) {
        if (pb.fileName() == name)
            return true;
    }

    return false;
}

PhraseBook TrWindow::phraseBookFromFileName(QString name) const
{
    foreach(PhraseBook pb, phraseBooks[PhraseCloseMenu]) {
        if (pb.fileName() == name)
            return pb;
    }

    return PhraseBook(); // empty phrasebook
}

void TrWindow::openPhraseBook()
{
    QString phrasebooks(QLibraryInfo::location(QLibraryInfo::DataPath));
    QString name = QFileDialog::getOpenFileName(this, tr("Open Phrase Book"),
        phrasebooks + QLatin1String("/phrasebooks"), tr("Qt phrase books (*.qph)\nAll files (*)"));
    //### The phrasebooks are not stored here!!
    if (!name.isEmpty() && !phraseBooksContains(name)) {
        if (openPhraseBook(name)) {
            int n = phraseBookFromFileName(name).count();
            statusBar()->showMessage(tr("%n phrase(s) loaded.", 0, n), MessageMS);
        }
    }
}

void TrWindow::closePhraseBook(QAction *action)
{
    PhraseBook pb = phraseBooks[PhraseCloseMenu].value(action);
    phraseBooks[PhraseCloseMenu].remove(action);
    m_ui.menuClosePhraseBook->removeAction(action);

    QAction *act = phraseBooks[PhraseEditMenu].key(pb);
    phraseBooks[PhraseEditMenu].remove(act);
    m_ui.menuEditPhraseBook->removeAction(act);

    act = phraseBooks[PhrasePrintMenu].key(pb);
    m_ui.menuPrintPhraseBook->removeAction(act);

    updatePhraseDict();
    m_ui.actionBatchTranslation->setEnabled(phraseBooks[PhraseCloseMenu].count() > 0);
}

void TrWindow::editPhraseBook(QAction *action)
{
    PhraseBook pb = phraseBooks[PhraseEditMenu].value(action);
    PhraseBookBox box(pb.fileName(), pb, this);
    box.setWindowTitle(tr("%1 - %2").arg(tr("Qt Linguist"))
        .arg(pb.friendlyPhraseBookName()));
    box.resize(500, 300);
    box.exec();

    // delete phrasebook from all menus before changing
    // this avoids detachment
    phraseBooks[PhraseEditMenu].remove(action);
    QAction *closeact = phraseBooks[PhraseCloseMenu].key(pb);
    phraseBooks[PhraseCloseMenu].remove(closeact);
    QAction *printact = phraseBooks[PhrasePrintMenu].key(pb);
    phraseBooks[PhrasePrintMenu].remove(printact);

    phraseBooks[PhraseEditMenu].insert(action, box.phraseBook());
    phraseBooks[PhraseCloseMenu].insert(closeact, box.phraseBook());
    phraseBooks[PhrasePrintMenu].insert(printact, box.phraseBook());

    updatePhraseDict();
}

void TrWindow::printPhraseBook(QAction *action)
{
    PhraseBook phraseBook = phraseBooks[PhrasePrintMenu].value(action);

    int pageNum = 0;

    QPrintDialog dlg(&printer, this);
    if (dlg.exec()) {
        printer.setDocName(phraseBook.fileName());
        statusBar()->showMessage(tr("Printing..."));
        PrintOut pout(&printer);
        pout.setRule(PrintOut::ThinRule);
        foreach (Phrase p, phraseBook) {
            pout.setGuide(p.source());
            pout.addBox(29, p.source());
            pout.addBox(4);
            pout.addBox(29, p.target());
            pout.addBox(4);
            pout.addBox(34, p.definition(), PrintOut::Emphasis);

            if (pout.pageNum() != pageNum) {
                pageNum = pout.pageNum();
                statusBar()->showMessage(tr("Printing... (page %1)")
                    .arg(pageNum));
            }
            pout.setRule(PrintOut::NoRule);
            pout.flushLine(true);
        }
        pout.flushLine(true);
        statusBar()->showMessage(tr("Printing completed"), MessageMS);
    } else {
        statusBar()->showMessage(tr("Printing aborted"), MessageMS);
    }
}

void TrWindow::revertSorting()
{
    if (cmdl->contextsInList() < 0)
        return;

    tv->clearSelection();
    tv->header()->setSortIndicator(1, Qt::AscendingOrder);
    tv->header()->setSortIndicatorShown(true);
    cmdl->sort(1, Qt::AscendingOrder);
    //mmdl->setContextItem(0);

    foreach(ContextItem *c, cmdl->contextList()) {
        c->sortMessages(1, Qt::AscendingOrder);
    }
}

void TrWindow::manual()
{
    if (!ac)
        ac = new QAssistantClient(QLibraryInfo::location(QLibraryInfo::BinariesPath), this);
    ac->showPage(QLibraryInfo::location(QLibraryInfo::DocumentationPath) +
                 QLatin1String("/html/linguist-manual.html"));
}

void TrWindow::about()
{

    QMessageBox box(this);
    box.setTextFormat(Qt::RichText);
    QString version = tr("Version %1");
#if QT_EDITION == QT_EDITION_OPENSOURCE
    QString open = tr(" Open Source Edition");
    version.append(open);
#endif
    version = version.arg(QLatin1String(QT_VERSION_STR));

    QString edition =
#if QT_EDITION == QT_EDITION_OPENSOURCE
                    tr("This version of Qt Linguist is part of the Qt Open Source Edition, for use "
                    "in the development of Open Source applications. "
                    "Qt is a comprehensive C++ framework for cross-platform application "
                    "development.<br/><br/>"
                    "You need a commercial Qt license for development of proprietary (closed "
                    "source) applications. Please see <tt>http://www.trolltech.com/company/model"
                    ".html</tt> for an overview of Qt licensing.");
#else
                    tr("This program is licensed to you under the terms of the "
                    "Qt Commercial License Agreement. For details, see the file LICENSE "
                    "that came with this software distribution.");
#endif
    
    box.setText(tr("<center><img src=\":/images/splash.png\"/></img><p>%1</p></center>"
                    "<p>Qt Linguist is a tool for adding translations to Qt "
                    "applications.</p>"
                    "<p>%2</p>"
                    "<p>Copyright (C) 2000-2008 Trolltech ASA. All rights reserved."
                    "</p><p>The program is provided AS IS with NO WARRANTY OF ANY KIND,"
                    " INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A"
                    " PARTICULAR PURPOSE.</p>").arg(version).arg(edition));
    
    box.setWindowTitle(QApplication::translate("AboutDialog", "Qt Linguist"));
    box.setIcon(QMessageBox::NoIcon);
    box.exec();
}

void TrWindow::aboutQt()
{
    QMessageBox::aboutQt(this, tr("Qt Linguist"));
}

void TrWindow::setupPhrase()
{
    bool enabled = !phraseBooks[PhraseCloseMenu].isEmpty();
    m_ui.menuClosePhraseBook->setEnabled(enabled);
    m_ui.menuEditPhraseBook->setEnabled(enabled);
    m_ui.menuPrintPhraseBook->setEnabled(enabled);
}

void TrWindow::closeEvent(QCloseEvent *e)
{
    if (maybeSave()) {
        e->accept();
        delete m_previewTool;
        m_previewTool = 0;
    } else {
        e->ignore();
    }
}

bool TrWindow::maybeSave()
{
    if (cmdl->isModified()) {
        switch (QMessageBox::information(this, tr("Qt Linguist"),
            tr("Do you want to save '%1'?").arg(filename),
            QMessageBox::Yes | QMessageBox::Default,
            QMessageBox::No,
            QMessageBox::Cancel | QMessageBox::Escape))
        {
            case QMessageBox::Cancel:
                return false;
            case QMessageBox::Yes:
                save();
                return !cmdl->isModified();
            case QMessageBox::No:
                break;
        }
    }
    return true;
}

void TrWindow::updateCaption()
{
    QString cap;
    bool enable = !filename.isEmpty();
    m_ui.actionSave->setEnabled(enable);
    m_ui.actionSaveAs->setEnabled(enable);
    m_ui.actionRelease->setEnabled(enable);
    m_ui.actionReleaseAs->setEnabled(enable);
    m_ui.actionPrint->setEnabled(enable);
    m_ui.actionAccelerators->setEnabled(enable);
    m_ui.actionEndingPunctuation->setEnabled(enable);
    m_ui.actionPhraseMatches->setEnabled(enable);
    m_ui.actionPlaceMarkerMatches->setEnabled(enable);
    m_ui.actionRevertSorting->setEnabled(enable);

    if (filename.isEmpty())
        cap = tr("Qt Linguist by Trolltech");
    else
        cap = tr("%1 - %2%3").arg( tr("Qt Linguist by Trolltech"))
        .arg(filename).arg(cmdl->isModified() ? QLatin1String("*") : QLatin1String(""));
    setWindowTitle(cap);
    modified->setEnabled(cmdl->isModified());
}

void TrWindow::showMessages(const QModelIndex &index)
{
    if (index.isValid()) {
        MessageItem *m = cmdl->messageItem(index);
        ContextItem *c = cmdl->contextItem(index);
        if (m && c) {
            QStringList translations  = cmdl->normalizedTranslations(*m);
            me->showMessage(m->context(), m->sourceText(), m->comment(), c->fullContext(),
                translations, m->message().type(), getPhrases(m->sourceText()));
            if (m->danger())
                printDanger(m);
            else
                statusBar()->clearMessage();

            m_ui.actionDoneAndNext->setEnabled(m->message().type() !=
                MetaTranslatorMessage::Obsolete);
        } else {
            me->showNothing();
            m_ui.actionDoneAndNext->setEnabled(false);
        }
    }
    else {
        me->showNothing();
        m_ui.actionDoneAndNext->setEnabled(false);
    }
}

void TrWindow::showNewCurrent(const QModelIndex &current, const QModelIndex &old)
{
    showMessages(current);
    m_ui.actionSelectAll->setEnabled(m_ui.actionDoneAndNext->isEnabled());

    Q_UNUSED(old);
}

void TrWindow::updateTranslation(const QStringList &translations)
{
    QModelIndex item = tv->currentIndex();
    if (!item.isValid())
        return;

    MessageItem *m = cmdl->messageItem(item);
    if (m) {
        if (translations != m->translations()) {
            m->setTranslations(translations);
            updateDanger(m, true);
            cmdl->updateItem(item);

            if (m->finished()) {
                updateFinished(false);
            } else {
                cmdl->setModified(true);
                updateCaption();
            }
        }
    }
}

/**
 * Updates the translation in *both* the MessageModel and in the flat MetaTranslator 'model'.
 */
void TrWindow::updateTranslation(int context, int message, const QString &translation)
{
    MessageItem *m = cmdl->messageItem(context, message);

    if (m && translation != m->translation()) {
        m->setTranslation(translation);

        updateDanger(m, true);

        if (m->finished())
            updateFinished(false);
        else
            updateCaption();

        // Notify the view(s)
        QModelIndex idx = cmdl->modelIndex(context, message);
        cmdl->updateItem(idx);
    }
}

void TrWindow::updateFinished(int context, int message, bool finished)
{
    MessageItem *m = cmdl->messageItem(context, message);
    if (finished != m->finished()) {
        m->setFinished(finished);
        updateProgress();
        updateCaption();
        updateStatistics();

        // Notify the view(s)
        QModelIndex idx = cmdl->modelIndex(context, message);
        cmdl->updateItem(idx);
    }
}


void TrWindow::updateFinished(bool finished)
{
    QModelIndex item = tv->currentIndex();
    if (!item.isValid())
        return;
    MessageItem *m = cmdl->messageItem(item);
    if (finished != m->finished()) {
        m->setFinished(finished);
        updateProgress();
        updateCaption();
        updateStatistics();

        cmdl->updateItem(item);
    }
}

void TrWindow::doneAndNext()
{
    if (!tv->currentIndex().isValid())
        return;

    MessageItem *m = cmdl->messageItem(tv->currentIndex());

    if (!m->danger()) {
        updateFinished(true);
        nextUnfinished();
    }
    else {
        qApp->beep();
    }
}

void TrWindow::toggleFinished(const QModelIndex &index)
{
    if ( !index.isValid() || (index.column() != 0) || (index.parent() == QModelIndex()) )
        return;

    MessageItem *m = cmdl->messageItem(index);

    if (m->message().type() == MetaTranslatorMessage::Obsolete)
        return;

    if (m->danger())
        printDanger(m);

    if (!m->danger() && !m->finished())
        updateFinished(true);
    else if (m->finished())
        updateFinished(false);
}

int TrWindow::findCurrentContextRow()
{
    QModelIndex index = tv->selectionModel()->currentIndex();
    if (index.isValid())
        return index.row();

    //if no context is selected
    setCurrentContextRow(0);
    return 0;
}

bool TrWindow::setNextContext(int *currentrow, bool checkUnfinished)
{
    QModelIndex mindx;
    ++(*currentrow);

    for (; *currentrow < cmdl->contextsInList(); ++(*currentrow)) {
        if (!checkUnfinished) {
            setCurrentContextRow(*currentrow);
            return true; //it is one more item
        }

        mindx = cmdl->index(*currentrow, 0);
        if (cmdl->contextItem(mindx)->unFinishedCount() > 0) {
            setCurrentContext(mindx);
            return true; // found a unfinished context
        }
    }

    return false; // done
}

bool TrWindow::setPrevContext(int *currentrow, bool checkUnfinished)
{
    QModelIndex mindx;
    --(*currentrow);

    for (; *currentrow >= 0; --(*currentrow)) {
        if (!checkUnfinished) {
            setCurrentContextRow(*currentrow);
            return true; //it is one more item
        }

        mindx = cmdl->index(*currentrow, 0);
        if (cmdl->contextItem(mindx)->unFinishedCount() > 0) {
            setCurrentContext(mindx);
            return true; // found a unfinished context
        }
    }

    return false; // done
}

bool TrWindow::setNextMessage(QModelIndex *currentIndex, bool checkUnfinished)
{
    bool found = false;
    Q_ASSERT(currentIndex);
    QModelIndex idx = currentIndex->isValid() ? *currentIndex : cmdl->index(0, 0);
    do {
        int row = 0;
        QModelIndex par = idx.parent();
        if (par.isValid()) {
            row = idx.row() + 1;
        } else {        //In case we are located on a top-level node
            par = idx;
        }

        if (row >= cmdl->rowCount(par)) {
            int toprow = par.row() + 1;
            if (toprow >= cmdl->rowCount()) toprow = 0;
            par = cmdl->index(toprow, 0);
            row = 0;
            idx = cmdl->index(row, 1, par);
        } else {
            idx = cmdl->index(row, 1, par);
        }
        MessageItem *m = cmdl->messageItem(idx);
        if (m)
            found = checkUnfinished ? !(m->finished() || m->obsolete()) : true;
        if (idx == *currentIndex) break;
    } while(!found);

    if (found) {
        *currentIndex = idx;
        tv->setCurrentIndex(*currentIndex);
    }
    return found;
}

bool TrWindow::setPrevMessage(QModelIndex *currentIndex, bool checkUnfinished)
{
    bool found = false;
    Q_ASSERT(currentIndex);
    QModelIndex idx = currentIndex->isValid() ? *currentIndex : cmdl->index(0, 0);
    do {
        int row = idx.row() - 1;
        QModelIndex par = idx.parent();
        if (!par.isValid()) {   //In case we are located on a top-level node
            par = idx;
            row = -1;
        }

        if (row < 0) {
            int toprow = par.row() - 1;
            if (toprow < 0) toprow = cmdl->rowCount() - 1;
            par = cmdl->index(toprow, 0);
            row = cmdl->rowCount(par) - 1;
            idx = cmdl->index(row, 1, par);
        } else {
            idx = cmdl->index(row, 1, par);
        }
        MessageItem *m = cmdl->messageItem(idx);
        if (m)
            found = checkUnfinished ? !(m->finished() || m->obsolete()) : true;
        if (idx == *currentIndex) break;
    } while(!found);

    if (found) {
        *currentIndex = idx;
        tv->setCurrentIndex(*currentIndex);
    }
    return found;
}

void TrWindow::nextUnfinished()
{
    if (m_ui.actionNextUnfinished->isEnabled()) {
        if (!next(true)) {
            // If no Unfinished message is left, the user has finished the job.  We
            // congratulate on a job well done with this ringing bell.
            statusBar()->showMessage(tr("No untranslated phrases left."), MessageMS);
            qApp->beep();
        }
    }
}

void TrWindow::prevUnfinished()
{
    if (m_ui.actionNextUnfinished->isEnabled()) {
        if (!prev(true)) {
            // If no Unfinished message is left, the user has finished the job.  We
            // congratulate on a job well done with this ringing bell.
            statusBar()->showMessage(tr("No untranslated phrases left."), MessageMS);
            qApp->beep();
        }
    }
}

void TrWindow::prev()
{
    prev(false);
}

bool TrWindow::prev(bool checkUnfinished)
{
    QModelIndex current = tv->currentIndex();
    bool ok = setPrevMessage(&current, checkUnfinished);    
    me->setEditorFocus();
    return ok;
}

bool TrWindow::next(bool checkUnfinished)
{
    QModelIndex current = tv->currentIndex();
    bool ok = setNextMessage(&current, checkUnfinished);
    me->setEditorFocus();
    return ok;
}

void TrWindow::next()
{
    next(false);
}


void TrWindow::findNext(const QString &text, int where, bool matchCase)
{
    if (text.isEmpty())
        return;
    findText = text;
    findWhere = where;
    findMatchCase = matchCase;
    m_ui.actionFindNext->setEnabled(true);
    findAgain();
}

void TrWindow::revalidate()
{
    if (cmdl->contextsInList() <= 0)
        return;

    ContextItem *c;
    MessageItem *m;

    for (int ci=0; ci<cmdl->contextsInList(); ++ci) {
        c = cmdl->contextItem(cmdl->index(ci, 0));
        for (int mi=0; mi<c->messageItemsInList(); ++mi) {
            m = c->messageItem(mi);
            updateDanger(m);
            //if (cmdl->contextItem() == c)
            //    cmdl->updateItem(cmdl->index(mi, 0));
        }
        cmdl->updateItem(cmdl->index(ci, 0));
    }
}

QString TrWindow::friendlyString(const QString& str)
{
    QString f = str.toLower();
    f.replace(QRegExp(QString(QLatin1String("[.,:;!?()-]"))), QString(QLatin1String(" ")));
    f.replace(QLatin1String("&"), QString(QLatin1String("")));
    f = f.simplified();
    f = f.toLower();
    return f;
}


void TrWindow::setupMenuBar()
{
    m_ui.setupUi(this);
    m_ui.actionAccelerators->setIcon(QIcon(rsrcString + QLatin1String("/accelerator.png")));
    m_ui.actionOpenPhraseBook->setIcon(QIcon(rsrcString + QLatin1String("/book.png")));
    m_ui.actionDoneAndNext->setIcon(QIcon(rsrcString + QLatin1String("/doneandnext.png")));
    m_ui.actionCopy->setIcon(QIcon(rsrcString + QLatin1String("/editcopy.png")));
    m_ui.actionCut->setIcon(QIcon(rsrcString + QLatin1String("/editcut.png")));
    m_ui.actionPaste->setIcon(QIcon(rsrcString + QLatin1String("/editpaste.png")));
    m_ui.actionOpen->setIcon(QIcon(rsrcString + QLatin1String("/fileopen.png")));
    m_ui.actionOpenAltSource->setIcon(QIcon(rsrcString + QLatin1String("/fileopen.png")));
    m_ui.actionSave->setIcon(QIcon(rsrcString + QLatin1String("/filesave.png")));
    m_ui.actionNext->setIcon(QIcon(rsrcString + QLatin1String("/next.png")));
    m_ui.actionNextUnfinished->setIcon(QIcon(rsrcString + QLatin1String("/nextunfinished.png")));
    m_ui.actionPhraseMatches->setIcon(QIcon(rsrcString + QLatin1String("/phrase.png")));
    m_ui.actionEndingPunctuation->setIcon(QIcon(rsrcString + QLatin1String("/punctuation.png")));
    m_ui.actionPrev->setIcon(QIcon(rsrcString + QLatin1String("/prev.png")));
    m_ui.actionPrevUnfinished->setIcon(QIcon(rsrcString + QLatin1String("/prevunfinished.png")));
    m_ui.actionPrint->setIcon(QIcon(rsrcString + QLatin1String("/print.png")));
    m_ui.actionRedo->setIcon(QIcon(rsrcString + QLatin1String("/redo.png")));
    m_ui.actionFind->setIcon(QIcon(rsrcString + QLatin1String("/searchfind.png")));
    m_ui.actionUndo->setIcon(QIcon(rsrcString + QLatin1String("/undo.png")));
    m_ui.actionPlaceMarkerMatches->setIcon(QIcon(rsrcString + QLatin1String("/validateplacemarkers.png")));
    m_ui.actionWhatsThis->setIcon(QIcon(rsrcString + QLatin1String("/whatsthis.png")));
    

    // File menu
    connect(m_ui.actionOpen, SIGNAL(triggered()), this, SLOT(open()));
    connect(m_ui.actionOpenAltSource, SIGNAL(triggered()), this, SLOT(openAltSource()));
    connect(m_ui.actionResetAltSource, SIGNAL(triggered()), this, SLOT(resetAltSource()));
    connect(m_ui.actionSave, SIGNAL(triggered()), this, SLOT(save()));
    connect(m_ui.actionSaveAs, SIGNAL(triggered()), this, SLOT(saveAs()));
    connect(m_ui.actionRelease, SIGNAL(triggered()), this, SLOT(release()));
    connect(m_ui.actionReleaseAs, SIGNAL(triggered()), this, SLOT(releaseAs()));
    connect(m_ui.actionPrint, SIGNAL(triggered()), this, SLOT(print()));
    connect(m_ui.actionExit, SIGNAL(triggered()), this, SLOT(close()));

    // Edit menu
    connect(m_ui.actionUndo, SIGNAL(triggered()), me, SLOT(undo()));
    connect(me, SIGNAL(undoAvailable(bool)), m_ui.actionUndo, SLOT(setEnabled(bool)));

    connect(m_ui.actionRedo, SIGNAL(triggered()), me, SLOT(redo()));
    connect(me, SIGNAL(redoAvailable(bool)), m_ui.actionRedo, SLOT(setEnabled(bool)));

    connect(m_ui.actionCopy, SIGNAL(triggered()), me, SLOT(copy()));
    connect(me, SIGNAL(copyAvailable(bool)), m_ui.actionCopy, SLOT(setEnabled(bool)));

    connect(me, SIGNAL(cutAvailable(bool)), m_ui.actionCut, SLOT(setEnabled(bool)));
    connect(m_ui.actionCut, SIGNAL(triggered()), me, SLOT(cut()));

    connect(me, SIGNAL(pasteAvailable(bool)), m_ui.actionPaste, SLOT(setEnabled(bool)));
    connect(m_ui.actionPaste, SIGNAL(triggered()), me, SLOT(paste()));

    connect(m_ui.actionSelectAll, SIGNAL(triggered()), me, SLOT(selectAll()));
    connect(m_ui.actionFind, SIGNAL(triggered()), this, SLOT(find()));
    connect(m_ui.actionFindNext, SIGNAL(triggered()), this, SLOT(findAgain()));
    connect(m_ui.actionSearchAndTranslate, SIGNAL(triggered()), this, SLOT(showTranslateDialog()));
    connect(m_ui.actionBatchTranslation, SIGNAL(triggered()), this, SLOT(showBatchTranslateDialog()));
    
    connect( m_ui.actionTranslationFileSettings, SIGNAL(triggered()), this, SLOT(showTranslationSettings()) );

    // Translation menu
    // when updating the accelerators, remember the status bar
    connect(m_ui.actionPrevUnfinished, SIGNAL(triggered()), this, SLOT(prevUnfinished()));
    connect(m_ui.actionNextUnfinished, SIGNAL(triggered()), this, SLOT(nextUnfinished()));
    connect(m_ui.actionNext, SIGNAL(triggered()), this, SLOT(next()));
    connect(m_ui.actionPrev, SIGNAL(triggered()), this, SLOT(prev()));
    connect(m_ui.actionDoneAndNext, SIGNAL(triggered()), this, SLOT(doneAndNext()));
    connect(m_ui.actionBeginFromSource, SIGNAL(triggered()), me, SLOT(beginFromSource()));
    connect(me, SIGNAL(updateActions(bool)), m_ui.actionBeginFromSource, SLOT(setEnabled(bool)));

    // Phrasebook menu
    connect(m_ui.actionNewPhraseBook, SIGNAL(triggered()), this, SLOT(newPhraseBook()));
    connect(m_ui.actionOpenPhraseBook, SIGNAL(triggered()), this, SLOT(openPhraseBook()));
    connect(m_ui.menuClosePhraseBook, SIGNAL(triggered(QAction*)),
        this, SLOT(closePhraseBook(QAction*)));
    connect(m_ui.menuEditPhraseBook, SIGNAL(triggered(QAction*)),
        this, SLOT(editPhraseBook(QAction*)));
    connect(m_ui.menuPrintPhraseBook, SIGNAL(triggered(QAction*)),
        this, SLOT(printPhraseBook(QAction*)));

    // Validation menu
    connect(m_ui.actionAccelerators, SIGNAL(triggered()), this, SLOT(revalidate()));
    connect(m_ui.actionEndingPunctuation, SIGNAL(triggered()), this, SLOT(revalidate()));
    connect(m_ui.actionPhraseMatches, SIGNAL(triggered()), this, SLOT(revalidate()));
    connect(m_ui.actionPlaceMarkerMatches, SIGNAL(triggered()), this, SLOT(revalidate()));

    // View menu
    connect(m_ui.actionRevertSorting, SIGNAL(triggered()), this, SLOT(revertSorting()));
    connect(m_ui.actionDisplayGuesses, SIGNAL(triggered()), this, SLOT(toggleGuessing()));
    connect(m_ui.actionStatistics, SIGNAL(triggered()), this, SLOT(toggleStatistics()));
    connect(m_ui.menuView, SIGNAL(aboutToShow()), this, SLOT(updateViewMenu()));
    m_ui.menuViewViews->addAction( dwScope->toggleViewAction() );
    m_ui.menuViewViews->addAction( me->phraseDockWnd()->toggleViewAction() );

    // Tools menu
    connect(m_batchTranslateDlg, SIGNAL(finished()), this, SLOT(finishedBatchTranslation()));
    connect(m_ui.actionPreviewForm, SIGNAL(triggered()), this, SLOT(previewForm()));

#if defined(Q_WS_MAC)
    // Window menu
    QMenu *windowMenu = new QMenu(tr("&Window"), this);
    menuBar()->insertMenu(m_ui.menuHelp->menuAction(), windowMenu);
    windowMenu->addAction(tr("Minimize"), this,
        SLOT(showMinimized()), QKeySequence(tr("Ctrl+M")));
#endif

    // Help
    connect(m_ui.actionManual, SIGNAL(triggered()), this, SLOT(manual()));
    connect(m_ui.actionAbout, SIGNAL(triggered()), this, SLOT(about()));
    connect(m_ui.actionAboutQt, SIGNAL(triggered()), this, SLOT(aboutQt()));
    connect(m_ui.actionWhatsThis, SIGNAL(triggered()), this, SLOT(onWhatsThis()));

    connect(m_ui.menuFile, SIGNAL(aboutToShow()), this,
        SLOT(setupRecentFilesMenu()));
    connect(m_ui.menuRecentlyOpenedFiles, SIGNAL(triggered(QAction*)), this,
        SLOT(recentFileActivated(QAction*)));

    m_ui.actionManual->setWhatsThis(tr("Display the manual for %1.").arg(tr("Qt Linguist")));
    m_ui.actionAbout->setWhatsThis(tr("Display information about %1.").arg(tr("Qt Linguist")));
    m_ui.actionDoneAndNext->setShortcuts(QList<QKeySequence>()
                                            << QKeySequence(QLatin1String("Ctrl+Return"))
                                            << QKeySequence(QLatin1String("Ctrl+Enter")));

    // Disable the Close/Edit/Print phrasebook menuitems if they are not loaded
    connect(m_ui.menuPhrases, SIGNAL(aboutToShow()), this, SLOT(setupPhrase()));
}

void TrWindow::updateViewMenu()
{
    if (stats)
        m_ui.actionStatistics->setChecked(stats->isVisible());
    else
        m_ui.actionStatistics->setChecked(false);
}

void TrWindow::onWhatsThis()
{
    QWhatsThis::enterWhatsThisMode();
}

void TrWindow::setupToolBars()
{
    QToolBar *filet = new QToolBar(this);
    filet->setObjectName(QLatin1String("FileToolbar"));
    filet->setWindowTitle(tr("File"));
	this->addToolBar(filet);
    m_ui.menuToolbars->addAction(filet->toggleViewAction());

    QToolBar *editt = new QToolBar(this);
    editt->setVisible(false);
    editt->setObjectName(QLatin1String("EditToolbar"));
    editt->setWindowTitle(tr("Edit"));
    this->addToolBar(editt);
    m_ui.menuToolbars->addAction(editt->toggleViewAction());

    QToolBar *translationst = new QToolBar(this);
    translationst->setObjectName(QLatin1String("TranslationToolbar"));
    translationst->setWindowTitle(tr("Translation"));
	this->addToolBar(translationst);
    m_ui.menuToolbars->addAction(translationst->toggleViewAction());

    QToolBar *validationt = new QToolBar(this);
    validationt->setObjectName(QLatin1String("ValidationToolbar"));
    validationt->setWindowTitle(tr("Validation"));
	this->addToolBar(validationt);
    m_ui.menuToolbars->addAction(validationt->toggleViewAction());

    QToolBar *helpt = new QToolBar(this);
    helpt->setVisible(false);
    helpt->setObjectName(QLatin1String("HelpToolbar"));
    helpt->setWindowTitle(tr("Help"));
	this->addToolBar(helpt);
    m_ui.menuToolbars->addAction(helpt->toggleViewAction());


    filet->addAction(m_ui.actionOpen);
    filet->addAction(m_ui.actionSave);
    filet->addAction(m_ui.actionPrint);
    filet->addSeparator();
    filet->addAction(m_ui.actionOpenPhraseBook);

    editt->addAction(m_ui.actionUndo);
    editt->addAction(m_ui.actionRedo);
    editt->addSeparator();
    editt->addAction(m_ui.actionCut);
    editt->addAction(m_ui.actionCopy);
    editt->addAction(m_ui.actionPaste);
    editt->addSeparator();
    editt->addAction(m_ui.actionFind);

    translationst->addAction(m_ui.actionPrev);
    translationst->addAction(m_ui.actionNext);
    translationst->addAction(m_ui.actionPrevUnfinished);
    translationst->addAction(m_ui.actionNextUnfinished);
    translationst->addAction(m_ui.actionDoneAndNext);

    validationt->addAction(m_ui.actionAccelerators);
    validationt->addAction(m_ui.actionEndingPunctuation);
    validationt->addAction(m_ui.actionPhraseMatches);
    validationt->addAction(m_ui.actionPlaceMarkerMatches);

    helpt->addAction(m_ui.actionWhatsThis);

}

void TrWindow::setCurrentContext(const QModelIndex &indx)
{
    tv->setCurrentIndex(indx);
    tv->scrollTo(indx);
}

void TrWindow::setCurrentContextRow(int row)
{
    QModelIndex mdlI = cmdl->index(row,1);
    tv->setCurrentIndex(mdlI);
    tv->scrollTo(mdlI);
}

void TrWindow::setCurrentMessage(const QModelIndex &indx)
{
    tv->setCurrentIndex(indx);
    tv->scrollTo(indx);
}

bool TrWindow::openPhraseBook(const QString& name)
{
    PhraseBook pb;
    if (!pb.load(name)) {
        QMessageBox::warning(this, tr("Qt Linguist"),
            tr("Cannot read from phrase book '%1'.").arg(name));
        return false;
    }

    QAction *a = m_ui.menuClosePhraseBook->addAction(pb.friendlyPhraseBookName());
    phraseBooks[PhraseCloseMenu].insert(a, pb);
    a->setWhatsThis(tr("Close this phrase book."));

    a = m_ui.menuEditPhraseBook->addAction(pb.friendlyPhraseBookName());
    phraseBooks[PhraseEditMenu].insert(a, pb);
    a->setWhatsThis(tr("Allow you to add, modify, or delete"
        " phrases of this phrase book."));

    a = m_ui.menuPrintPhraseBook->addAction(pb.friendlyPhraseBookName());
    phraseBooks[PhrasePrintMenu].insert(a, pb);
    a->setWhatsThis(tr("Print the entries of the phrase"
        " book."));
    
    updatePhraseDict();
    m_ui.actionBatchTranslation->setEnabled(phraseBooks[PhraseCloseMenu].count() > 0);
    return true;
}

bool TrWindow::savePhraseBook(QString &name, const PhraseBook &pb)
{
    if (!name.contains(QLatin1String(".qph")) && !name.contains(QLatin1String(".")))
        name += QLatin1String(".qph");

    if (!pb.save(name)) {
        QMessageBox::warning(this, tr("Qt Linguist"),
            tr("Cannot create phrase book '%1'.").arg(name));
        return false;
    }
    return true;
}

void TrWindow::updateProgress()
{
    int numNonobsolete = cmdl->getNumNonobsolete();
    int numFinished = cmdl->getNumFinished();
    if (numNonobsolete == 0)
        progress->setText(QString(QLatin1String("    ")));
    else
        progress->setText(QString(QLatin1String(" %1/%2 ")).arg(numFinished)
        .arg(numNonobsolete));
    m_ui.actionPrevUnfinished->setEnabled(numFinished != numNonobsolete);
    m_ui.actionNextUnfinished->setEnabled(numFinished != numNonobsolete);

    m_ui.actionPrev->setEnabled(cmdl->contextsInList() > 0);
    m_ui.actionNext->setEnabled(cmdl->contextsInList() > 0);
}

void TrWindow::updatePhraseDict()
{
    phraseDict.clear();

    foreach (PhraseBook pb, phraseBooks[PhraseCloseMenu]) {
        foreach (Phrase p, pb) {
            QString f = friendlyString(p.source());
            if ( f.length() > 0 ) {
                f = f.split(QLatin1Char(' ')).first();
                if (!phraseDict.contains(f)) {
                    PhraseBook pbe;
                    phraseDict.insert(f, pbe);
                }
                phraseDict[f].append(p);
            }
        }
    }
    revalidate();
}

PhraseBook TrWindow::getPhrases(const QString &source)
{
    PhraseBook phrases;
    QString f = friendlyString(source);
    QStringList lookupWords = f.split(QLatin1Char(' '));

    foreach (QString s, lookupWords) {
        if (phraseDict.contains(s)) {
            PhraseBook ent = phraseDict.value(s);
            foreach (Phrase p, ent) {
                if (f.indexOf(friendlyString((p).source())) >= 0)
                    phrases.append(p);
            }
        }
    }
    return phrases;
}

void TrWindow::printDanger(MessageItem *m)
{
    danger(m, true);
}

bool TrWindow::updateDanger(MessageItem *m, bool verbose)
{
    bool dngr = danger(m, verbose);

    if (dngr != m->danger())
        m->setDanger(dngr);

    return dngr;
}

bool TrWindow::danger( const MessageItem *m,
                       bool verbose )
{
    QString source = m->sourceText();
    QStringList translations = m->translations();
    if (m_ui.actionAccelerators->isChecked()) {
        bool sk = source.contains(Qt::Key_Ampersand);
        bool tk = true;
        for (int i = 0; i < translations.count() && tk; ++i) {
            tk &= bool(translations[i].contains(Qt::Key_Ampersand));
        }

        if (!sk && tk) {
            if (verbose)
                statusBar()->showMessage(tr("Accelerator possibly superfluous in"
                                         " translation."), ErrorMS);
            return true;
        } else if (sk && !tk) {
            if (verbose)
                statusBar()->showMessage(tr("Accelerator possibly missing in"
                                         " translation."), ErrorMS);
            return true;
        }
    }
    if (m_ui.actionEndingPunctuation->isChecked()) {
        bool endingok = true;
        for (int i = 0; i < translations.count() && endingok; ++i) {
            endingok &= (ending( source, cmdl->language()) == ending(translations[i], cmdl->language() ));
        }

        if (!endingok) {
            if (verbose)
                statusBar()->showMessage(tr("Translation does not end with the"
                    " same punctuation as the source text."), ErrorMS);
            return true;
        }
    }
    if (m_ui.actionPhraseMatches->isChecked()) {
        QString fsource = friendlyString(source);
        QString ftranslation = friendlyString(translations.first());
        QStringList lookupWords = fsource.split(QLatin1Char(' '));

        bool phraseFound;
        foreach (QString s, lookupWords) {
            if (phraseDict.contains(s)) {
                PhraseBook ent = phraseDict.value(s);
                phraseFound = true;
                foreach (Phrase p, ent) {
                    if (fsource == friendlyString(p.source())) {
                        if (ftranslation.indexOf(friendlyString(p.target())) >= 0) {
                            phraseFound = true;
                            break;
                        } else {
                            phraseFound = false;
                        }
                    }
                }
                if (!phraseFound) {
                    if (verbose)
                        statusBar()->showMessage(tr("A phrase book suggestion for"
                            " '%1' was ignored.").arg(s), ErrorMS );
                    return true;
                }
            }
        }
    }

    if (m_ui.actionPlaceMarkerMatches->isChecked()) {
        // Stores the occurence count of the place markers in the vector placeMarkerIndexes.
        // i.e. the occurence count of %1 is stored at placeMarkerIndexes[1], 
        // count of %2 is stored at placeMarkerIndexes[2] etc.
        // In the first pass, it counts all place markers in the sourcetext.
        // In the second pass it (de)counts all place markers in the translation.
        // When finished, all elements should have returned to a count of 0, if not there is a mismatch
        // between place markers in the source text and the translation text.
        QVector<int> placeMarkerIndexes;
        QString translation;
        int numTranslations = translations.count();
        for (int pass = 0; pass < numTranslations + 1; ++pass) {
            const QChar *uc_begin = source.unicode();
            const QChar *uc_end = uc_begin + source.length();
            if (pass >= 1) {
                translation = translations[pass - 1];
                uc_begin = translation.unicode();
                uc_end = uc_begin + translation.length();
            }
            const QChar *c = uc_begin;
            while (c < uc_end) {
                if (c->unicode() == '%') {
                    const QChar *escape_start = ++c;
                    while (c->isDigit()) ++c;
                    const QChar *escape_end = c;
                    bool ok = true;
                    int markerIndex = QString::fromRawData(escape_start, escape_end - escape_start).toInt(&ok);
                    if (ok) {
                        if (markerIndex >= placeMarkerIndexes.size()) {
                            placeMarkerIndexes.resize(markerIndex + 1);
                        }
                        placeMarkerIndexes[markerIndex]+= (pass == 0 ? numTranslations : -1);
                    }
                }
                ++c;
            }
        }

        for (int i = 0; i < placeMarkerIndexes.size(); ++i) {
            if (placeMarkerIndexes.at(i) != 0) {
                if (verbose)
                    statusBar()->showMessage(tr("Translation does not refer"
                        " to the same place markers as in the source text."), ErrorMS);
                return true;

            }
        }        
    }
    if (verbose)
        statusBar()->clearMessage();

    return false;
}

void TrWindow::readConfig()
{
    QString keybase(QString::number((QT_VERSION >> 16) & 0xff)
                    + QLatin1Char('.') + QString::number((QT_VERSION >> 8) & 0xff) + QLatin1Char('/'));
    QSettings config;

    QRect r( pos(), size() );
    recentFiles = config.value(keybase + QLatin1String("RecentlyOpenedFiles")).toStringList();
    restoreGeometry(config.value(keybase + QLatin1String("Geometry/WindowGeometry")).toByteArray());
    restoreState(config.value(keybase + QLatin1String("MainWindowState")).toByteArray());

    m_ui.actionAccelerators->setChecked(config.value(keybase+ QLatin1String("Validators/Accelerator"), true).toBool());
    m_ui.actionEndingPunctuation->setChecked(config.value(keybase+ QLatin1String("Validators/EndingPunctuation"), true).toBool());
    m_ui.actionPhraseMatches->setChecked(config.value(keybase+ QLatin1String("Validators/PhraseMatch"), true).toBool());
    m_ui.actionPlaceMarkerMatches->setChecked(config.value(keybase+ QLatin1String("Validators/PlaceMarkers"), true).toBool());
}

void TrWindow::writeConfig()
{
    QString keybase(QString::number( (QT_VERSION >> 16) & 0xff )
                    + QLatin1Char('.') + QString::number((QT_VERSION >> 8) & 0xff) + QLatin1Char('/'));
    QSettings config;
    config.setValue(keybase + QLatin1String("RecentlyOpenedFiles"), recentFiles);
    config.setValue(keybase + QLatin1String("Geometry/WindowGeometry"), saveGeometry());
    config.setValue(keybase+ QLatin1String("Validators/Accelerator"), m_ui.actionAccelerators->isChecked());
    config.setValue(keybase+ QLatin1String("Validators/EndingPunctuation"), m_ui.actionEndingPunctuation->isChecked());
    config.setValue(keybase+ QLatin1String("Validators/PhraseMatch"), m_ui.actionPhraseMatches->isChecked());
    config.setValue(keybase+ QLatin1String("Validators/PlaceMarkers"), m_ui.actionPlaceMarkerMatches->isChecked());
    config.setValue(keybase + QLatin1String("MainWindowState"), saveState());
}

void TrWindow::setupRecentFilesMenu()
{
    m_ui.menuRecentlyOpenedFiles->clear();

    if (recentFiles.count() > 0) {
        m_ui.menuRecentlyOpenedFiles->setEnabled(true);
        QStringList::Iterator it = recentFiles.begin();
        for (; it != recentFiles.end(); ++it) {
            m_ui.menuRecentlyOpenedFiles->addAction(*it);
        }
    } else {
        m_ui.menuRecentlyOpenedFiles->setEnabled(false);
    }
}

void TrWindow::recentFileActivated(QAction *action)
{
    if (!action->text().isEmpty()) {
        if (maybeSave())
            openFile(action->text());
    }
}

void TrWindow::addRecentlyOpenedFile(const QString &fn, QStringList &lst)
{
    QFileInfo fi(fn);
    if (lst.contains(fi.absoluteFilePath()))
        return;
    if ( lst.count() >= 10 )
        lst.removeLast();

    lst.prepend(fi.absoluteFilePath());
}

void TrWindow::toggleGuessing()
{
    me->toggleGuessing();
}

void TrWindow::focusSourceList()
{
    stv->setFocus();
}

void TrWindow::focusPhraseList()
{
    ptv->setFocus();
}

void TrWindow::toggleStatistics()
{
    if (m_ui.actionStatistics->isChecked()) {
        if (!stats) {
            stats = new Statistics(this);
            connect(cmdl, SIGNAL(statsChanged(int,int,int,int,int,int)), stats,
                SLOT(updateStats(int,int,int,int,int,int)));
        }
        stats->show();
        updateStatistics();
    }
    else if (stats) {
        stats->close();
    }
}

void TrWindow::updateStatistics()
{
    // don't call this if stats dialog is not open
    // because this can be slow...
    if (!stats || !stats->isVisible())
        return;

    cmdl->updateStatistics();
}

void TrWindow::finishedBatchTranslation()
{
    updateStatistics();
    updateProgress();
    updateCaption();
}

QStringList TrWindow::findFormFilesInCurrentTranslationFile()
{
    QStringList ret;
    for (MessageModel::iterator it = cmdl->begin(); it != cmdl->end(); ++it) {
        QString fileName = (*it)->message().fileName();
        if (fileName.endsWith(QLatin1String(".ui"))) {
            if (!ret.contains(fileName)) {
                ret+=fileName;
            }
        }
    }
    return ret;
}

void TrWindow::previewForm()
{
    if (m_previewTool.isNull()) {
        m_previewTool = new TrPreviewTool();
        QStringList fileNames = findFormFilesInCurrentTranslationFile();
        bool ok = true;
        for (QStringList::iterator it = fileNames.begin(); it != fileNames.end(); ++it) {
            QString fileName = *it;
            if (!fileName.isEmpty() && QFileInfo(fileName).exists()) {
                if (!m_previewTool->addFormFile(fileName)) {
                    ok = false;
                    break;
                }
            }
        }
        if (ok) {
            ok = m_previewTool->addTranslator(cmdl->translator(), QFileInfo(filename).fileName());
            if (ok) {
                m_previewTool->cascade();
            } else {
                QMessageBox::warning(this, tr("Qt Linguist"), tr("There was a problem in the preparation of form preview."));
            }
        }
    } else {
        // Only do a refresh if it is already open
        m_previewTool->reloadTranslations();
    }
    m_previewTool->show();
}

void TrWindow::showTranslationSettings()
{
    if (!m_translationSettingsDialog) {
        m_translationSettingsDialog = new TranslationSettingsDialog(this);
    }
    m_translationSettingsDialog->setMessageModel(cmdl);
    m_translationSettingsDialog->exec();
}
