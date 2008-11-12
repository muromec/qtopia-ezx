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

#ifndef TRWINDOW_H
#define TRWINDOW_H

#include "phrase.h"
#include "messagestreeview.h"
#include "ui_mainwindow.h"
#include <QMainWindow>
#include <QHash>
#include <QPrinter>
#include <QtCore/QPointer>
#include <QtCore/QLocale>
class QModelIndex;
class QStringList;
class QPixmap;
class QAction;
class QDialog;
class QLabel;
class QMenu;
class QAssistantClient;
class QIcon;

class TrPreviewTool;

class QTreeView;
class PhraseModel;
class PhraseItem;
class MessageModel;
class MessageItem;
class ContextItem;
class FindDialog;
class TranslateDialog;
class BatchTranslationDialog;
class TranslationSettingsDialog;
class MessageEditor;
class Statistics;

class TrWindow : public QMainWindow
{
    Q_OBJECT
public:
    enum {PhraseCloseMenu, PhraseEditMenu, PhrasePrintMenu};

    static QPixmap *pxOn;
    static QPixmap *pxOff;
    static QPixmap *pxObsolete;
    static QPixmap *pxDanger;
    static QPixmap *pxWarning;
    static QPixmap *pxEmpty;
    static const QPixmap pageCurl();

    TrWindow();
    ~TrWindow();

    void openFile(const QString &name);

protected:
    void readConfig();
    void writeConfig();
    void closeEvent(QCloseEvent *);

private slots:
    void doneAndNext();
    void prev();
    void next();
    void recentFileActivated(QAction *action);
    void setupRecentFilesMenu();
    void open();
    void save();
    void saveAs();
    void release();
    void releaseAs();
    void print();
    void find();
    void findAgain();
    void showTranslateDialog();
    void showBatchTranslateDialog();
    void showTranslationSettings();
    void translateAndFindNext(const QString& findWhat, const QString &translateTo, int matchOption, int mode, bool markFinished);
    void translate(int mode);
    void newPhraseBook();
    void openPhraseBook();
    void closePhraseBook(QAction *action);
    void editPhraseBook(QAction *action);
    void printPhraseBook(QAction *action);
    void openAltSource();
    void resetAltSource();
    void manual();
    void revertSorting();
    void about();
    void aboutQt();
    void updateViewMenu();

    void setupPhrase();
    bool maybeSave();
    void updateCaption();
    void showNewCurrent(const QModelIndex &current, const QModelIndex &old);
    void showMessages(const QModelIndex &index);
    
    // To synchronize from the contextmodel to the MetaTranslator...
    // Operates on the selected item
    void updateTranslation(const QStringList &translations);
    void updateFinished(bool finished);
    // Operates on the given item
    void updateTranslation(int context, int message, const QString &translation);
    void updateFinished(int context, int message, bool finished);

    void toggleFinished(const QModelIndex &index);
    void prevUnfinished();
    void nextUnfinished();
    void findNext(const QString &text, int where, bool matchCase);
    void revalidate();
    void toggleGuessing();
    void focusSourceList();
    void focusPhraseList();
    void toggleStatistics();
    void updateStatistics();
    void onWhatsThis();
    void finishedBatchTranslation();
    void previewForm();
    void updateLanguage(QLocale::Language);
private:

    typedef QHash<QString, PhraseBook> PBD;

    static QString friendlyString(const QString &str);

    int findCurrentContextRow();
    //bool setNextMessage(int *currentrow, bool checkUnfinished);
    bool setNextMessage(QModelIndex *currentIndex, bool checkUnfinished);
    bool setPrevMessage(QModelIndex *currentIndex, bool checkUnfinished);
    bool setNextContext(int *currentrow, bool checkUnfinished);
    bool setPrevContext(int *currentrow, bool checkUnfinished);
    bool next(bool checkUnfinished);
    bool prev(bool checkUnfinished);
    QStringList findFormFilesInCurrentTranslationFile();

    void addRecentlyOpenedFile(const QString &fn, QStringList &lst);
    void setupMenuBar();
    void setupToolBars();
    void setCurrentContextRow(int row);
    void setCurrentContext(const QModelIndex &indx);
    void setCurrentMessage(const QModelIndex &indx);
    PhraseBook phraseBookFromFileName(QString name) const;
    bool openPhraseBook(const QString &name);
    bool phraseBooksContains(QString name);
    bool savePhraseBook(QString &name, const PhraseBook &pb);
    void openAltSource(const QString &name);
    void updateProgress();
    void updatePhraseDict();
    PhraseBook getPhrases(const QString &source);
    bool danger(const MessageItem *message, bool verbose = false);

    void printDanger(MessageItem *m);
    bool updateDanger(MessageItem *m, bool verbose = false);

    bool searchItem(const QString &searchWhat, int c, int m);

    QAssistantClient *ac;
    MessagesTreeView *tv;
    MessageModel *cmdl;
    MessageModel *altTranslatorModel;
    QTreeView *stv;
    QTreeView *ptv;
    PhraseModel *pmdl;
    MessageEditor * me;
    QLabel        * progress;
    QLabel        * modified;
    QStringList recentFiles;
    QString     filename;
    PBD phraseDict;
    QMap<QAction *, PhraseBook> phraseBooks[3];
    QPrinter printer;

    FindDialog *finddlg;
    QString findText;
    int findWhere;
    bool findMatchCase;
    int foundWhere;
    int foundOffset;
    TranslateDialog *m_translatedlg;
    BatchTranslationDialog *m_batchTranslateDlg;
    TranslationSettingsDialog *m_translationSettingsDialog;
    QString m_translateTo;
    bool m_findMatchSubstring;
    bool m_markFinished;
    
    // used by the preview tool
    QPointer<TrPreviewTool> m_previewTool;

    QDockWidget *dwScope;
    Ui::MainWindow m_ui;    // menus and actions
    Statistics *stats;
};

#endif
