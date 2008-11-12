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

#ifndef MSGEDIT_H
#define MSGEDIT_H

#include "trwindow.h"
#include "phrase.h"
#include "messagemodel.h"
#include <QString>
#include <QPixmap>
#include <QBitmap>
#include <QToolTip>
#include <QtEvents>
#include <QShortcut>
#include <QFrame>
#include <QTextEdit>
#include <QScrollArea>
#include <QTextCharFormat>

template <typename T> class QList;
class QSplitter;
class QDockWidget;
class QLabel;
class QTreeView;
class QVBoxLayout;
class EditorPage;
class MetaTranslator;
class QMenu;
class MessageEditor;


class TransEditor : public QWidget
{
    Q_OBJECT
public:
    TransEditor(QWidget *parent = 0);

    void setLabel(const QString &text);
    QTextEdit *editor() const { return m_editor; }
    QLabel *label() const { return m_label; }

    void calculateFieldHeight();
    QString translation() const;

    void gotFocusInEvent( QFocusEvent * e);

public slots:
    void handleTranslationChanges();

signals:
    void heightUpdated(int height);
    void gotFocusIn();

private:
    QTextEdit *m_editor;
    QLabel *m_label;
};

class SourceTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    SourceTextEdit(QWidget *parent = 0);

public slots:
    void copySelection();

protected:
    void contextMenuEvent(QContextMenuEvent *e);

private:
    QAction *actCopy;
    QAction *actSelect;
    QMenu *srcmenu;
};

class GuessShortcut : public QShortcut
{
    Q_OBJECT
public:
    GuessShortcut(int nkey, QWidget *parent, const char *member)
        : QShortcut(parent), nrkey(nkey)
    {
        setKey(Qt::CTRL + (Qt::Key_1 + nrkey));
        connect(this, SIGNAL(activated()), this, SLOT(keyActivated()));
        connect(this, SIGNAL(activated(int)), parent, member);
    }

private slots:
    void keyActivated()
    {
        emit activated(nrkey);
    }

signals:
    void activated(int nkey);

private:
    int nrkey;
};

class PageCurl : public QWidget
{
    Q_OBJECT
public:
    PageCurl(QWidget *parent = 0)
        : QWidget(parent)
    {
        QPixmap px = TrWindow::pageCurl();
        if ( px.hasAlphaChannel() ) {
            setMask( px.mask() );
        }
        QPalette pal = palette();
        pal.setBrush(backgroundRole(), px);
        setPalette(pal);
        setFixedSize( px.size() );
    }

protected:
    bool event(QEvent *e)
    {
        if (e->type() == QEvent::ToolTip && toolTip().isEmpty()) {
            QRect r(34, 0, width()-34, 19);

            QPoint pt = static_cast<QHelpEvent*>(e)->pos();

            if (r.contains(pt)) {
                QToolTip::showText(static_cast<QHelpEvent*>(e)->globalPos(),
                    tr("Next unfinished phrase"), this);
            }

            r.setSize(QSize(width()-34, height()-20));
            r.setX(0);
            r.setY(20);

            if (r.contains(pt)) {
                QToolTip::showText(static_cast<QHelpEvent*>(e)->globalPos(),
                    tr("Previous unfinished phrase"), this);
            }
        }

        return QWidget::event(e);
    }
    void mouseReleaseEvent(QMouseEvent *e)
    {
        int x = e->pos().x()-10;
        int y = e->pos().y();

        if (y < x)
            emit nextPage();
        else
            emit prevPage();
    }

signals:
    void prevPage();
    void nextPage();
};

class ShadowWidget : public QWidget
{
public:
    ShadowWidget(QWidget *parent = 0);
    ShadowWidget(QWidget *child, QWidget *parent = 0);

    void setShadowWidth(int width) {sWidth = width;}
    int  shadowWidth() const {return sWidth;}
    void setMargin(int margin) {wMargin = margin;}
    int  margin() const {return wMargin;}
    void setWidget(QWidget *child);

protected:
    void resizeEvent(QResizeEvent *e);
    void paintEvent(QPaintEvent *e);

private:
    int sWidth;
    int wMargin;
    QWidget *childWgt;
};

class EditorPage : public QFrame
{
    Q_OBJECT
public:
    EditorPage(MessageEditor *parent = 0, const char *name = 0);
    QTextEdit *activeTransText() const;
    int activeTranslationNumerus() const;
    QStringList translations() const;
    void setNumerusForms(const QString &invariantForm, const QStringList &numerusForms);
    int currentTranslationEditor();

protected:
    void resizeEvent(QResizeEvent *);
    void layoutWidgets();
    void updateCommentField();
    void updateAltSourceField();
    void calculateFieldHeight(QTextEdit *field);
    void fontChange(const QFont &);

private:
    void addPluralForm(const QString &label);
    void adjustTranslationFieldHeights();
    void handleChanges();
    void showNothing();

    PageCurl *pageCurl;
    QLabel *srcTextLbl;
    SourceTextEdit *srcText;
    QLabel *altTextLbl;
    SourceTextEdit *altText;
    QTextEdit *cmtText;
    QStringList m_numerusForms;
    QString     m_invariantForm;
    bool        m_pluralEditMode;
    QList<TransEditor*> m_transTexts;
    friend class MessageEditor;

private slots:
    void handleSourceChanges();
    void handleAltSourceChanges();
    void handleCommentChanges();
    void sourceSelectionChanged();
    void translationSelectionChanged();
    void updateHeight(int h);

signals:
    void pageHeightUpdated(int height);
    void selectionChanged();
    void currentTranslationEditorChanged();
};

class MessageEditor : public QScrollArea
{
    Q_OBJECT
public:
    MessageEditor(MessageModel *model, MessageModel *altTraslatorModel, QMainWindow *parent = 0);
    QTreeView *phraseView() const;
    inline QDockWidget *phraseDockWnd() const {return bottomDockWnd;}

    void showNothing();
    void showMessage(const QString &context, const QString &text, const QString &comment,
        const QString &fullContext, const QStringList &translation,
        MetaTranslatorMessage::Type type,
        const QList<Phrase> &phrases);
    void setNumerusForms(const QString &invariantForm, const QStringList &numerusForms);
    bool eventFilter(QObject *, QEvent *);
    void setTranslation(const QString &translation, int numerus, bool emitt);
    void setAltTextLabel(const QString &str);
    void setTranslationLabel(const QString &str);
signals:
    void translationChanged(const QStringList &translations);
    void finished(bool finished);
    void prevUnfinished();
    void nextUnfinished();
    void updateActions(bool enable);

    void undoAvailable(bool avail);
    void redoAvailable(bool avail);
    void cutAvailable(bool avail);
    void copyAvailable(bool avail);
    void pasteAvailable(bool avail);

    void focusSourceList();
    void focusPhraseList();

public slots:
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void selectAll();
    void beginFromSource();
    void toggleGuessing();
    void setEditorFocus();
    void checkUndoRedo();

private slots:
    void emitTranslationChanged();
    void guessActivated(int key);
    void insertPhraseInTranslation(const QModelIndex &index);
    void insertPhraseInTranslationAndLeave(const QModelIndex &index);
    void updateButtons();
    void updateCanPaste();
    void clipboardChanged();

    void updatePageHeight(int height);
    void updateCutAndCopy();

protected:
    void resizeEvent(QResizeEvent *);

public:
    static const char backTab[];
    static const char * const friendlyBackTab[];

private:
    void visualizeBackTabs(const QString &text, QTextEdit *te);
    void setEditionEnabled(bool enabled);

    MessageModel *srcMdl;
    QDockWidget *bottomDockWnd;
    EditorPage *editorPage;

    QLabel * phraseLbl;
    QTreeView *phraseTv;
    PhraseModel *phrMdl;
    QTextCharFormat defFormat;

    ShadowWidget *sw;

    MessageModel *m_contextModel;
    MessageModel *m_altTranslatorModel;
    QTranslator *altTranslator;
    QString sourceText;

    bool cutAvail;
    bool copyAvail;

    bool mayOverwriteTranslation;
    bool clipboardEmpty;
    bool doGuesses;
};

#endif
