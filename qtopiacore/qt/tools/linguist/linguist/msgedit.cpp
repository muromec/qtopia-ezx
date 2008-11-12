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

/*  TRANSLATOR MsgEdit

  This is the right panel of the main window.
*/

#include "msgedit.h"
#include "trwindow.h"
#include "simtexth.h"
#include "messagemodel.h"
#include "phrasemodel.h"

#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QLabel>
#include <QLayout>
#include <QTextEdit>
#include <QPalette>
#include <QString>
#include <QPainter>
#include <QHeaderView>
#include <QDockWidget>
#include <QFont>
#include <QTreeView>
#include <QScrollArea>
#include <QtGui/QTextDocumentFragment>
#include <QtGui/QTextCursor>
#include <QtGui/QTextBlock>
#include <QtGui/QTextFragment>
#include <QtGui/QTextImageFormat>
#include <QtGui/QPainter>
#include <QtGui/QImage>
#include <QtCore/QUrl>
#include <QAbstractTextDocumentLayout>

static const int MaxCandidates = 5;

const char MessageEditor::backTab[] = "\a\b\f\n\r\t";
const char * const MessageEditor::friendlyBackTab[] = {
        QT_TRANSLATE_NOOP("MessageEditor", "bell"),
        QT_TRANSLATE_NOOP("MessageEditor", "backspace"),
        QT_TRANSLATE_NOOP("MessageEditor", "new page"),
        QT_TRANSLATE_NOOP("MessageEditor", "new line"),
        QT_TRANSLATE_NOOP("MessageEditor", "carriage return"),
        QT_TRANSLATE_NOOP("MessageEditor", "tab")
    };

const char *bellImageName = "trolltech/bellImage";
const char *backspaceImageName = "trolltech/bsImage";
const char *newpageImageName = "trolltech/newpageImage";
const char *newlineImageName = "trolltech/newlineImage";
const char *crImageName = "trolltech/crImage";
const char *tabImageName = "trolltech/tabImage";
const char *backTabImages[] = {
    bellImageName, 
    backspaceImageName, 
    newpageImageName, 
    newlineImageName, 
    crImageName, 
    tabImageName};

class BackTabTextEdit : public QTextEdit
{
public:
    BackTabTextEdit(QWidget *parent = 0) : QTextEdit(parent) { }

    virtual QVariant loadResource ( int type, const QUrl & name );

    virtual void keyPressEvent ( QKeyEvent * e);
    virtual void focusInEvent ( QFocusEvent * e);

    QMap<QUrl, QImage> m_backTabOmages;
    QImage m_tabImg;
    QImage m_newlineImg;
};

QVariant BackTabTextEdit::loadResource ( int type, const QUrl & name )
{
    QImage img;
    if (type == QTextDocument::ImageResource) {
        img = m_backTabOmages.value(name);
        if (img.isNull()) {
            for (uint i = 0; i < qstrlen(MessageEditor::backTab); ++i) {
                if (backTabImages[i] && name == QUrl(QLatin1String(backTabImages[i]))) {

                    QFont fnt = font();
                    fnt.setItalic(true);
                    QFontMetrics fm(fnt);
                    int h = fm.height();
                    
                    QString str = QString::fromAscii("(%1)").arg(QLatin1String(MessageEditor::friendlyBackTab[i]));
                    int w = fm.boundingRect(str).width() + 1;   //###
                    QImage textimg(w, h, QImage::Format_RGB32);
                    textimg.fill(qRgb(255,255,255));

                    QPainter p(&textimg);
                    p.setPen(QColor(Qt::blue));
                    p.setFont(fnt);
                    p.drawText(0, fm.ascent(), str);            //###
                    document()->addResource(QTextDocument::ImageResource, QUrl(QLatin1String(backTabImages[i])), textimg);

                    m_backTabOmages.insert(name, textimg);
                    return textimg;
                }
            }                
        }
    }
    return img;
}

void BackTabTextEdit::focusInEvent ( QFocusEvent * e)
{
    TransEditor *te = qobject_cast<TransEditor*>(parent());
    te->gotFocusInEvent(e);
    QTextEdit::focusInEvent(e);
}

void BackTabTextEdit::keyPressEvent ( QKeyEvent * e )
{
    bool eatevent = false;
    QTextCursor tc = textCursor();
    if (e->modifiers() == Qt::NoModifier) {
        switch (e->key()) {
        case Qt::Key_Tab: {
            tc = textCursor();
            tc.insertImage(QLatin1String(tabImageName));
            eatevent = true;
            break; }
        case Qt::Key_Return: {
            tc = textCursor();
            document()->blockSignals(true);
            tc.beginEditBlock();
            tc.insertImage(QLatin1String(newlineImageName));
            document()->blockSignals(false);
            tc.insertBlock();
            tc.endEditBlock();
            eatevent = true;
            break; }
        case Qt::Key_Backspace: 
            if (tc.anchor() == tc.position()) {
                QTextCursor tc = textCursor();
                if (!tc.atStart() && tc.atBlockStart()) {
                    tc.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, 1);
                    QTextCharFormat fmt = tc.charFormat();
                    if (fmt.isImageFormat()) {
                        tc.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, 1);
                    }
                    tc.removeSelectedText();
                    eatevent = true;
                }
            }
            break;
        case Qt::Key_Delete:
            if (tc.anchor() == tc.position()) {
                QTextCursor tc = textCursor();
                tc.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 1);
                QTextCharFormat fmt = tc.charFormat();
                if (fmt.isImageFormat()) {
                    if (!tc.atEnd() && tc.atBlockEnd()) {
                        tc.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 1);
                    }
                    tc.removeSelectedText();
                    eatevent = true;
                }
            }
            break;
        }
    }
    // Also accept Key_Enter on the numpad
    if (e->modifiers() == Qt::KeypadModifier && e->key() == Qt::Key_Enter) {
        tc = textCursor();
        document()->blockSignals(true);
        tc.beginEditBlock();
        tc.insertImage(QLatin1String(newlineImageName));
        document()->blockSignals(false);
        tc.insertBlock();
        tc.endEditBlock();
        eatevent = true;
    }


    if (eatevent) e->accept();
    else QTextEdit::keyPressEvent(e);
}

TransEditor::TransEditor(QWidget *parent /*= 0*/) 
: QWidget(parent)
{
    QVBoxLayout *lout = new QVBoxLayout(this);
    lout->setSpacing(2);
    lout->setMargin(0);
    m_label = new QLabel(this);
    lout->addWidget(m_label);
    m_editor = new BackTabTextEdit(this);
    m_label->setFocusProxy(m_editor );
    setFocusProxy(m_editor);
    lout->addWidget(m_editor);
    setLayout(lout);

    connect(m_editor->document(), SIGNAL(contentsChanged()),
             this, SLOT(handleTranslationChanges()));

    if (parent) {
        QFont fnt = parent->font();
        fnt.setBold(true);
        m_label->setFont(fnt);
    }
}

void TransEditor::gotFocusInEvent ( QFocusEvent * e)
{
    Q_UNUSED(e);
    emit gotFocusIn();
}

void TransEditor::setLabel(const QString &text)
{
    m_label->setText(text);
}

void TransEditor::handleTranslationChanges()
{
    calculateFieldHeight();
}


void TransEditor::calculateFieldHeight()
{
    QTextEdit *field = m_editor;
    int contentsHeight = qRound(field->document()->documentLayout()->documentSize().height());
    if (contentsHeight != field->height()) {
        int oldHeight = field->height();
        if (contentsHeight < 30)
            contentsHeight = 30;

        resize(width(), m_label->height() + 6 + 2 + contentsHeight);
        emit heightUpdated(height() + (field->height() - oldHeight));
    }

}


QString TransEditor::translation() const 
{ 
    QString plain;
    QTextBlock tb = m_editor->document()->begin();
    for (int b = 0; b < m_editor->document()->blockCount(); ++b) {
        QTextBlock::iterator it = tb.begin();
        if (it.atEnd()) {
            plain += tb.text();
        } else {
            while ( !it.atEnd() ) {
                QTextCharFormat fmt = it.fragment().charFormat();
                if (fmt.isImageFormat()) {
                    QTextImageFormat tif = fmt.toImageFormat();
                    if (tif.name() == QLatin1String(tabImageName)) plain += QLatin1Char('\t');
                    else if (tif.name() == QLatin1String(newlineImageName)) plain += QLatin1Char('\n');
                } else {
                    plain += it.fragment().text();
                }
                ++it;
            }
        }
        tb = tb.next();
    }
    return plain;
}

void MessageEditor::visualizeBackTabs(const QString &text, QTextEdit *te)
{
    te->clear();
    QTextCursor tc(te->textCursor());
    QTextCharFormat blueFormat = defFormat;
    blueFormat.setForeground(QBrush(Qt::blue));
    blueFormat.setFontItalic(true);
    blueFormat.setProperty(QTextFormat::UserProperty, -1);

    QString plainText;
    for (int i = 0; i < (int) text.length(); ++i)
    {
        int ch = text[i].unicode();
        if (ch < 0x20)
        {
            if (!plainText.isEmpty())
            {
                tc.insertText(plainText, defFormat);
                plainText.clear();
            }
            const char *p = strchr(backTab, ch);
            // store the character in the user format property
            // in the first '(' in the phrase
            blueFormat.setProperty(QTextFormat::UserProperty, ch);
            tc.insertText(QString(QLatin1String("(")), blueFormat);
            blueFormat.setProperty(QTextFormat::UserProperty, -1);
            if (p == 0)
            {
                tc.insertText(QString::number(ch, 16) + QLatin1String(")"), blueFormat);
            }
            else
            {
                tc.insertText(MessageEditor::tr(friendlyBackTab[p - backTab]) + QLatin1String(")"),
                    blueFormat);
                if (backTab[p - backTab] == '\n')
                    tc.insertBlock();
            }
        }
        // if a space is by itself, at the end, or beside other spaces
        else if (ch == ' ')
        {
            if (i == 0 || i == text.length() - 1 || text[i - 1].isSpace() ||
                text[i + 1].isSpace())
            {
                tc.insertText(plainText, defFormat);
                plainText.clear();
                blueFormat.setProperty(QTextFormat::UserProperty, ch);
                tc.insertText(QString(QLatin1String("(")), blueFormat);
                blueFormat.setProperty(QTextFormat::UserProperty, -1);
                tc.insertText(MessageEditor::tr("sp)"), blueFormat);
            }
            else
            {
                plainText += QLatin1Char(' ');
            }
        }
        else
        {
            plainText += QString(ch);
        }
    }
    tc.insertText(plainText, defFormat);
}

SourceTextEdit::SourceTextEdit(QWidget *parent) : QTextEdit(parent)
{
    srcmenu = 0;
    actCopy = new QAction(tr("&Copy"), this);
    actCopy->setShortcut(QKeySequence(tr("Ctrl+C")));
    actSelect = new QAction(tr("Select &All"), this);
    actSelect->setShortcut(QKeySequence(tr("Ctrl+A")));
    connect(actCopy, SIGNAL(triggered()), this, SLOT(copySelection()));
    connect(actSelect, SIGNAL(triggered()), this, SLOT(selectAll()));
}

void SourceTextEdit::copySelection()
{
    QTextDocumentFragment tdf = textCursor().selection();
    QTextDocument td;
    QTextCursor tc(&td);
    tc.insertFragment(tdf);
    int ch;

    tc.movePosition(QTextCursor::Start);
    while(!tc.atEnd())
    {
        tc.movePosition(QTextCursor::NextCharacter);
        ch = tc.charFormat().intProperty(QTextFormat::UserProperty);
        if (ch != 0) // if wrong format
        {
            // delete char
            tc.deletePreviousChar();
            if (ch != -1) // insert backtab
                tc.insertText(QString(ch));
        }
    }

    QApplication::clipboard()->setText(td.toPlainText());
}

void SourceTextEdit::contextMenuEvent(QContextMenuEvent *e)
{
    if (!srcmenu)
    {
        srcmenu = new QMenu(this);
        srcmenu->addAction(actCopy);
        srcmenu->addAction(actSelect);
    }

    actCopy->setEnabled(textCursor().hasSelection());
    actSelect->setEnabled(!document()->isEmpty());

    srcmenu->popup(e->globalPos());
}

/*
   ShadowWidget class impl.

   Used to create a shadow like effect for a widget
*/
ShadowWidget::ShadowWidget(QWidget *parent)
    : QWidget(parent), sWidth(10), wMargin(3), childWgt(0)
{

}

ShadowWidget::ShadowWidget(QWidget *child, QWidget *parent)
    : QWidget(parent), sWidth(10), wMargin(3), childWgt(0)
{
    setWidget(child);
}

void ShadowWidget::setWidget(QWidget *child)
{
    childWgt = child;
    if (childWgt && childWgt->parent() != this) {
        childWgt->setParent(this);
        childWgt->move(0,0);
        childWgt->show();
    }
}

void ShadowWidget::resizeEvent(QResizeEvent *)
{
    if (childWgt) {
        childWgt->move(wMargin, wMargin);
        childWgt->resize(width() - sWidth - wMargin, height() - sWidth -
            wMargin);
    }
}

void ShadowWidget::paintEvent(QPaintEvent *e)
{
    QPainter p;
    int w = width() - sWidth;
    int h = height() - sWidth;


    if (!((w > 0) && (h > 0)))
        return;

    if (p.begin(this)) {
        p.setPen(palette().color(QPalette::Shadow));

        p.drawPoint(w + 5, 6);
        p.drawLine(w + 3, 6, w + 5, 8);
        p.drawLine(w + 1, 6, w + 5, 10);
        int i;
        for (i=7; i < h; i += 2)
            p.drawLine( w, i, w + 5, i + 5);
        for (i = w - i + h; i > 6; i -= 2)
            p.drawLine( i, h, i + 5, h + 5);
        for (; i > 0 ; i -= 2)
            p.drawLine( 6, h + 6 - i, i + 5, h + 5);

        p.end();
    }
    QWidget::paintEvent(e);
}

/*
   EditorPage class impl.

   A frame that contains the source text, translated text and any
   source code comments and hints.
*/
EditorPage::EditorPage(MessageEditor *parent, const char *name)
    : QFrame(parent)
{
    setObjectName(QLatin1String(name));
    setLineWidth(1);
    setFrameStyle(QFrame::Box | QFrame::Plain);

    // Use white explicitly as the background color for the editor page.
    QPalette p = palette();
    p.setColor(QPalette::Active, QPalette::Base, QColor(Qt::white));
    p.setColor(QPalette::Inactive, QPalette::Base, QColor(Qt::white));
    p.setColor(QPalette::Disabled, QPalette::Base, QColor(Qt::white));
    p.setColor(QPalette::Active, QPalette::Window,
                p.color(QPalette::Active, QPalette::Base));
    p.setColor(QPalette::Inactive, QPalette::Window,
                p.color(QPalette::Inactive, QPalette::Base));
    p.setColor(QPalette::Disabled, QPalette::Window,
                p.color(QPalette::Disabled, QPalette::Base));

    parent->setPalette(p);

    srcTextLbl = new QLabel(tr("Source text"), this);

    QFont fnt = font();
    fnt.setBold(true);
    srcTextLbl->setFont(fnt);

    srcText = new SourceTextEdit(this);
    srcText->setFrameStyle(QFrame::NoFrame);
    srcText->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding,
        QSizePolicy::Minimum));
    srcText->setAutoFormatting(QTextEdit::AutoNone);
    srcText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    srcText->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    p = srcText->palette();
    p.setColor(QPalette::Disabled, QPalette::Base, p.color(QPalette::Active, QPalette::Base));
    srcText->setPalette( p );
	srcText->setReadOnly(true);
    srcText->setWhatsThis(tr("This area shows the source text.") );
    connect(srcText->document(), SIGNAL(contentsChanged()), SLOT(handleSourceChanges()));
    connect(srcText, SIGNAL(selectionChanged()),
             SLOT(sourceSelectionChanged()));

    altTextLbl = new QLabel(this);
    altTextLbl->setText(tr("Alternative source text"));
    altTextLbl->setFont(fnt);

    altText = new SourceTextEdit(this);
    altText->setFrameStyle(QFrame::NoFrame);
    altText->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding,
        QSizePolicy::Minimum));
    altText->setAutoFormatting(QTextEdit::AutoNone);
    altText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    altText->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    altText->setWhatsThis(tr("This area shows text from an auxillary translation.") );
    p = altText->palette();
    p.setColor(QPalette::Disabled, QPalette::Base, p.color(QPalette::Active, QPalette::Base));
    altText->setPalette( p );
    altText->setReadOnly(true);
    connect(altText->document(), SIGNAL(contentsChanged()), SLOT(handleAltSourceChanges()));
    connect(altText, SIGNAL(selectionChanged()),
             SLOT(sourceSelectionChanged()));

    cmtText = new QTextEdit(this);
    cmtText->setObjectName(QLatin1String("comment/context view"));
    cmtText->setFrameStyle( QFrame::NoFrame );
    cmtText->setSizePolicy( QSizePolicy( QSizePolicy::MinimumExpanding,
                                         QSizePolicy::Minimum ) );
    cmtText->setAutoFormatting(QTextEdit::AutoNone);
    cmtText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    cmtText->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    p = cmtText->palette();
    p.setColor(QPalette::Active, QPalette::Base, QColor(236,245,255));
    p.setColor(QPalette::Inactive, QPalette::Base, QColor(236,245,255));
    cmtText->setPalette(p);
	cmtText->setReadOnly(true);
    connect(cmtText->document(), SIGNAL(contentsChanged()), SLOT(handleCommentChanges()));

    cmtText->setWhatsThis(tr("This area shows a comment that"
                        " may guide you, and the context in which the text"
                        " occurs.") );

    m_pluralEditMode = false;
    addPluralForm(m_invariantForm);

    pageCurl = new PageCurl(this);

    // Focus
    //setFocusPolicy(Qt::StrongFocus);
    //parent->setFocusProxy(transText);
    //transLbl->setFocusProxy(transText);
    //srcTextLbl->setFocusProxy(transText);
    //srcText->setFocusProxy(transText);
    //cmtText->setFocusProxy(transText);
    //setFocusProxy(transText);

    updateCommentField();
    updateAltSourceField();
    layoutWidgets();

}

void EditorPage::showNothing()
{
    srcText->clear();
    cmtText->clear();
    altText->clear();
    handleChanges();
}

void EditorPage::handleChanges()
{
    handleSourceChanges();
    handleAltSourceChanges();
    handleCommentChanges();
    adjustTranslationFieldHeights();
    updateCommentField();
    updateAltSourceField();
}

void EditorPage::addPluralForm(const QString &label)
{
    TransEditor *te = new TransEditor(this);
    te->setLabel(label);
    if (m_transTexts.count()) {
        te->setVisible(false);
    }
    te->label()->adjustSize();
    
    te->editor()->setObjectName(QLatin1String("translation editor"));
    te->editor()->setFrameStyle(QFrame::NoFrame);
    te->editor()->setSizePolicy(QSizePolicy(QSizePolicy::MinimumExpanding,
                                             QSizePolicy::MinimumExpanding));
    te->editor()->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    te->editor()->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    te->editor()->setAutoFormatting(QTextEdit::AutoNone);
    te->editor()->setLineWrapMode(QTextEdit::WidgetWidth);
    QPalette p = te->editor()->palette();
    p.setColor(QPalette::Disabled, QPalette::Base, p.color(QPalette::Active, QPalette::Base));
    te->editor()->setPalette(p);
    connect(te, SIGNAL(heightUpdated(int)), this, SLOT(updateHeight(int)));
    connect(te->editor(), SIGNAL(selectionChanged()),
             SLOT(translationSelectionChanged()));
    connect(te, SIGNAL(gotFocusIn(void)), this, SIGNAL(currentTranslationEditorChanged(void)));

    m_transTexts << te;
}

int EditorPage::currentTranslationEditor()
{
    for (int i = 0; i < m_transTexts.count(); ++i) {
        QTextEdit *te = m_transTexts[i]->editor();
        if (te->hasFocus()) return i;
    }
    return -1;  //no focus
}

void EditorPage::updateHeight(int /*h*/)
{
    layoutWidgets();
}

/*! internal
    Returns all translations for an item.
    The number of translations is dependent on if we have a plural form or not.
    If we don't have a plural form, then this should only contain one item.
    Otherwise it will contain the number of numerus forms for the particular language.
*/
QStringList EditorPage::translations() const
{
    QStringList translations;
    for (int i = 0; i < m_transTexts.count() && m_transTexts.at(i)->isVisible(); ++i) {
        translations << m_transTexts[i]->translation();
    }
    return translations;
}

/*
   Don't show the comment field if there are no comments.
*/
void EditorPage::updateCommentField()
{
    if (cmtText->toPlainText().isEmpty())
        cmtText->hide();
    else
        cmtText->show();

    layoutWidgets();
}

/*
   Don't show the auxiliary translation field if there is no translation.
*/
void EditorPage::updateAltSourceField()
{
    if (altText->toPlainText().isEmpty()) {
        altTextLbl->hide();
        altText->hide();
    } else {
        altText->show();
        altTextLbl->show();
    }
    layoutWidgets();
}

/*
   Handle the widget layout manually
*/
void EditorPage::layoutWidgets()
 {

    int margin = 6;
    int space  = 2;
    int w = width();

    pageCurl->move(width() - pageCurl->width(), 0);

    QFontMetrics fm(srcTextLbl->font());
    srcTextLbl->move(margin, margin);
    srcTextLbl->resize(fm.width(srcTextLbl->text()), srcTextLbl->height());

    srcText->move(margin, srcTextLbl->y() + srcTextLbl->height() + space);
    srcText->resize(w - margin*2, srcText->height());

    int ypos = srcText->y() + srcText->height() + space;
    if (altText->isVisible()) {
        QFontMetrics altfm(altTextLbl->font());
        altTextLbl->move(margin, ypos);
        altTextLbl->resize(altfm.width(altTextLbl->text()), altTextLbl->height());

        altText->move(margin, altTextLbl->y() + altTextLbl->height() + space);
        altText->resize(w - margin*2, altText->height());

        ypos = altText->y() + altText->height() + space;
    }
    if (cmtText->isVisible()) {
        cmtText->move(margin, ypos);
        cmtText->resize(w - margin*2, cmtText->height());
        ypos+=cmtText->height() + space;
    }

    for (int i = 0; i < m_transTexts.count(); ++i) {
        TransEditor *te = m_transTexts[i];
        te->resize(w - margin * 2, te->height());
        te->move(margin, ypos);
        if (te->isVisible()) {
            ypos += te->height() + space;
        }
    }

    int totHeight = ypos + margin - srcTextLbl->y();

    if (height() != totHeight)
        emit pageHeightUpdated(totHeight);
}

void EditorPage::resizeEvent(QResizeEvent *)
{
    adjustTranslationFieldHeights();
    handleSourceChanges();
    handleAltSourceChanges();
    handleCommentChanges();

    layoutWidgets();
}

void EditorPage::adjustTranslationFieldHeights()
{
    for (int i = 0; i < m_transTexts.count(); ++i) {
        m_transTexts[i]->calculateFieldHeight();
    }
    if (srcText->textCursor().hasSelection())
        translationSelectionChanged();
}

void EditorPage::handleSourceChanges()
{
    calculateFieldHeight(srcText);
}

void EditorPage::handleAltSourceChanges()
{
    calculateFieldHeight(altText);
}

void EditorPage::handleCommentChanges()
{
    calculateFieldHeight(cmtText);
}

// makes sure only one of the textedits has a selection
void EditorPage::sourceSelectionChanged()
{
    for (int i = 0; i < m_transTexts.count(); ++i) {
        QTextEdit *te = m_transTexts[i]->editor();
        bool oldBlockState = te->blockSignals(true);
        QTextCursor c = te->textCursor();
        c.clearSelection();
        te->setTextCursor(c);
        te->blockSignals(oldBlockState);
    }
    emit selectionChanged();
}

void EditorPage::translationSelectionChanged()
{
    bool oldBlockState = srcText->blockSignals(true);
    QTextCursor c = srcText->textCursor();
    c.clearSelection();
    srcText->setTextCursor(c);
    srcText->blockSignals(oldBlockState);

    // clear the selection for all except the sender
    QTextEdit *te = qobject_cast<QTextEdit*>(sender());
    for (int i = 0; i < m_transTexts.count(); ++i) {
        QTextEdit *t = m_transTexts[i]->editor();
        if (t != te) {
            oldBlockState = t->blockSignals(true);
            QTextCursor c = t->textCursor();
            c.clearSelection();
            t->setTextCursor(c);
            t->blockSignals(oldBlockState);
        }
    }

    emit selectionChanged();
}

int EditorPage::activeTranslationNumerus() const
{
    for (int i = 0; i < m_transTexts.count(); ++i) {
        if (m_transTexts[i]->editor()->hasFocus()) {
            return i;
        }
    }
    //### hmmm.....
    if (m_transTexts.count()) return 0;
    return -1;
}

void EditorPage::setNumerusForms(const QString &invariantForm, const QStringList &numerusForms)
{
    m_invariantForm = invariantForm;
    m_numerusForms = numerusForms;

    if (!m_pluralEditMode) {
        m_transTexts[0]->setLabel(invariantForm);
    } else {
        m_transTexts[0]->setLabel(tr("Translation (%1)").arg(m_numerusForms[0]));
    }
    int i;
    for (i = 1; i < m_numerusForms.count(); ++i) {
        QString label = tr("Translation (%1)").arg(m_numerusForms[i]);
        if (i >= m_transTexts.count()) {
            addPluralForm(label);
        } else {
            m_transTexts[i]->setLabel(label);
        }
        m_transTexts[i]->setVisible(m_pluralEditMode);
    }
    for (int j = m_transTexts.count() - i; j > 0; --j) {
        TransEditor *te = m_transTexts.takeLast();
        delete te;
        ++i;
    }
    layoutWidgets();
}

QTextEdit *EditorPage::activeTransText() const
{
    int numerus = activeTranslationNumerus();
    if (numerus != -1) {
        return m_transTexts[numerus]->editor();
    }
    return 0;
}

/*
   Check if the translation text field is big enough to show all text
   that has been entered. If it isn't, resize it.
*/
void EditorPage::calculateFieldHeight(QTextEdit *field)
{
    int contentsHeight = qRound(field->document()->documentLayout()->documentSize().height());

    if (contentsHeight != field->height()) {
        int oldHeight = field->height();
        if (contentsHeight < 30)
            contentsHeight = 30;
        field->resize(field->width(), contentsHeight);
        emit pageHeightUpdated(height() + (field->height() - oldHeight));
    }
}

void EditorPage::fontChange(const QFont &)
{
    //keep the labels bold...
    QFont fnt = font();

    fnt.setBold(true);
    QFontMetrics fm(fnt);
    srcTextLbl->setFont(fnt);
    srcTextLbl->resize(fm.width(srcTextLbl->text()), srcTextLbl->height());
    for (int i = 0; i < m_transTexts.count(); ++i) {
        QLabel *transLbl = m_transTexts[i]->label();
        transLbl->setFont(fnt);
        transLbl->resize(fm.width(transLbl->text()), transLbl->height());
    }
    update();
}

/*
   MessageEditor class impl.

   Handle layout of dock windows and the editor page.
*/
MessageEditor::MessageEditor(MessageModel *model, MessageModel *altTranslatorModel, QMainWindow *parent)
    : QScrollArea(parent->centralWidget()), m_contextModel(model), m_altTranslatorModel(altTranslatorModel)
{
    altTranslator = m_altTranslatorModel->translator();
    cutAvail = false;
    copyAvail = false;
    doGuesses = true;
    bottomDockWnd = new QDockWidget(parent);
    bottomDockWnd->setObjectName(QLatin1String("PhrasesDockwidget"));
    bottomDockWnd->setAllowedAreas(Qt::AllDockWidgetAreas);
    bottomDockWnd->setFeatures(QDockWidget::AllDockWidgetFeatures);
    bottomDockWnd->setWindowTitle(tr("Phrases"));

    QWidget *w = new QWidget(bottomDockWnd);
    w->setSizePolicy(QSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum));
    QVBoxLayout *vl = new QVBoxLayout(w);
    vl->setSpacing(6);

    phraseLbl = new QLabel( tr("Phrases and guesses:"), w );

    phraseTv = new QTreeView(w);
    phraseTv->setObjectName(QLatin1String("phrase list view"));
    phrMdl = new PhraseModel(w);
    phraseTv->setModel(phrMdl);
    phraseTv->setAlternatingRowColors(true);
    phraseTv->setSelectionBehavior(QAbstractItemView::SelectRows);
    phraseTv->setSelectionMode(QAbstractItemView::SingleSelection);
    phraseTv->setRootIsDecorated(false);
    phraseTv->setItemsExpandable(false);
    QPalette pal = phraseTv->palette();
    phraseTv->setPalette(pal);

    phraseTv->header()->setResizeMode(QHeaderView::Stretch);
    phraseTv->header()->setClickable(true);

    vl->addWidget(phraseLbl);
    vl->addWidget(phraseTv);

    int i;
    for (i = 0; i < 9; ++i) {
        (void) new GuessShortcut(i, this, SLOT(guessActivated(int)));
    }

    bottomDockWnd->setWidget(w);
    parent->addDockWidget(Qt::BottomDockWidgetArea, bottomDockWnd);

    setObjectName(QLatin1String("scroll area"));
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setFrameStyle(QFrame::NoFrame);

    editorPage = new EditorPage(this, "editor page");
    connect(editorPage, SIGNAL(pageHeightUpdated(int)),
             SLOT(updatePageHeight(int)));

    sw = new ShadowWidget(editorPage, this);
    sw->setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding));
    sw->setMinimumSize(QSize(100, 150));

    setWidget(sw);
    defFormat = editorPage->srcText->currentCharFormat();

    // Signals
    connect(editorPage->pageCurl, SIGNAL(nextPage()),
        SIGNAL(nextUnfinished()));
    connect(editorPage->pageCurl, SIGNAL(prevPage()),
        SIGNAL(prevUnfinished()));
    connect(this, SIGNAL(translationChanged(const QStringList &)),
        this, SLOT(updateButtons()));

    connect(this, SIGNAL(translationChanged(const QStringList &)),
        this, SLOT(checkUndoRedo()));
    connect(editorPage, SIGNAL(currentTranslationEditorChanged()),
        this, SLOT(checkUndoRedo()));

    connect(editorPage, SIGNAL(selectionChanged()),
        this, SLOT(updateCutAndCopy()));
    connect(qApp->clipboard(), SIGNAL(dataChanged()),
        this, SLOT(clipboardChanged()));
    connect(phraseTv, SIGNAL(doubleClicked(QModelIndex)),
        this, SLOT(insertPhraseInTranslation(QModelIndex)));

    clipboardChanged();

    phraseTv->installEventFilter(this);

    // What's this
    this->setWhatsThis(tr("This whole panel allows you to view and edit "
                              "the translation of some source text.") );
    showNothing();
}

void MessageEditor::checkUndoRedo()
{
    QTextEdit *te = editorPage->activeTransText();    
    undoAvailable(te->document()->isUndoAvailable());
    redoAvailable(te->document()->isRedoAvailable());
}

bool MessageEditor::eventFilter(QObject *o, QEvent *e)
{
    // handle copying from the source
    if ((e->type() == QEvent::KeyPress) ||
        (e->type() == QEvent::ShortcutOverride))
    {
        QKeyEvent *ke = static_cast<QKeyEvent *>(e);

        // handle return key in phrase list
        if (o == phraseTv
            && e->type() == QEvent::KeyPress
            && ke->modifiers() == Qt::NoModifier
            && ke->key() == Qt::Key_Return
            && phraseTv->currentIndex().isValid())
        {
            insertPhraseInTranslationAndLeave(phraseTv->currentIndex());
            return false;
        }

        if (ke->modifiers() & Qt::ControlModifier)
        {
            if ((ke->key() == Qt::Key_A) &&
                editorPage->srcText->underMouse())
            {
                editorPage->srcText->selectAll();
                return true;
            }
            if ((ke->key() == Qt::Key_C) &&
                editorPage->srcText->textCursor().hasSelection())
            {
                editorPage->srcText->copySelection();
                return true;
            }
        }
    }

    return QScrollArea::eventFilter(o, e);
}

void MessageEditor::updatePageHeight(int height)
{
    sw->resize(sw->width(), height + sw->margin() + sw->shadowWidth());
}

void MessageEditor::resizeEvent(QResizeEvent *e)
{
    sw->resize(viewport()->width(), sw->height());
    QScrollArea::resizeEvent(e);
}

QTreeView *MessageEditor::phraseView() const
{
    return phraseTv;
}

void MessageEditor::showNothing()
{
    setEditionEnabled(false);
    sourceText.clear();
    setTranslation(QString(), 0, false);
    editorPage->showNothing();
}

static CandidateList similarTextHeuristicCandidates( MessageModel::iterator it,
                        const char *text,
                        int maxCandidates )
{
    QList<int> scores;
    CandidateList candidates;

    StringSimilarityMatcher stringmatcher(QString::fromLatin1(text));

    for (MessageItem *m = 0; (m = it.current()); ++it) {
        MetaTranslatorMessage mtm = m->message();
        if ( mtm.type() == MetaTranslatorMessage::Unfinished ||
             mtm.translation().isEmpty() )
            continue;

        QString s = m->sourceText();

        int score = stringmatcher.getSimilarityScore(s);

        if ( (int) candidates.count() == maxCandidates &&
             score > scores[maxCandidates - 1] )
            candidates.removeAt( candidates.size()-1 );
        if ( (int) candidates.count() < maxCandidates && score >= textSimilarityThreshold ) {
            Candidate cand( s, mtm.translation() );

            int i;
            for ( i = 0; i < (int) candidates.size(); i++ ) {
                if ( score >= scores.at(i) ) {
                    if ( score == scores.at(i) ) {
                        if ( candidates.at(i) == cand )
                            goto continue_outer_loop;
                    } else {
                        break;
                    }
                }
            }
            scores.insert( i, score );
            candidates.insert( i, cand );
        }
        continue_outer_loop:
        ;
    }
    return candidates;
}


void MessageEditor::showMessage(const QString &context,
                                const QString &text,
                                const QString &comment,
                                const QString &fullContext,
                                const QStringList &translations,
                                MetaTranslatorMessage::Type type,
                                const QList<Phrase> &phrases)
{
    phraseTv->clearSelection();

    bool obsolete = (type == MetaTranslatorMessage::Obsolete);
    setEditionEnabled(!obsolete);
    sourceText = text;

    visualizeBackTabs(text, editorPage->srcText);

    visualizeBackTabs(altTranslator->translate(context.simplified().toLatin1(),
                                               text.simplified().toLatin1(),
                                               comment.simplified().toLatin1()),
                                               editorPage->altText);

    if (!fullContext.isEmpty() && !comment.isEmpty())
        visualizeBackTabs(fullContext.simplified() + QLatin1String("\n") +
            comment.simplified(), editorPage->cmtText);
    else if (!fullContext.isEmpty() && comment.isEmpty())
        visualizeBackTabs(fullContext.simplified(), editorPage->cmtText);
    else if (fullContext.isEmpty() && !comment.isEmpty())
        visualizeBackTabs(comment.simplified(), editorPage->cmtText);
    else
        editorPage->cmtText->clear();

    editorPage->m_pluralEditMode = translations.count() > 1;
    if (!editorPage->m_pluralEditMode) {
        editorPage->m_transTexts[0]->setLabel(editorPage->m_invariantForm);
    } else {
        editorPage->m_transTexts[0]->setLabel(tr("Translation (%1)").arg(editorPage->m_numerusForms.first()));
    }
    int i;
    for (i = 0; i < qMax(1, editorPage->m_numerusForms.count()); ++i) {
        bool shouldShow = i < translations.count();
        if (shouldShow) {
            setTranslation(translations[i], i, false);
        } else {
            setTranslation(QString(), i, false);
        }
        if (i >= 1) {
            editorPage->m_transTexts[i]->setVisible(shouldShow);
        }
    }
    phrMdl->removePhrases();

    foreach(Phrase p, phrases) {
        phrMdl->addPhrase(p);
    }

    if (doGuesses && !sourceText.isEmpty()) {
        CandidateList cl = similarTextHeuristicCandidates(m_contextModel->begin(),
            sourceText.toLatin1(), MaxCandidates);
        int n = 0;
        QList<Candidate>::Iterator it = cl.begin();
        while (it != cl.end()) {
            QString def;
            if (n < 9)
                def = tr("Guess (%1)").arg(QString(QKeySequence(Qt::CTRL | (Qt::Key_0 + (n + 1)))));
            else
                def = tr("Guess");
            phrMdl->addPhrase(Phrase((*it).source, (*it).target, def, n));
            ++n;
            ++it;
        }
    }
    phrMdl->resort();
    editorPage->handleChanges();
}

void MessageEditor::setNumerusForms(const QString &invariantForm, const QStringList &numerusForms)
{
    // uninstall the emitTranslationChanged slots and remove event filters
    for (int i = 0; i  < editorPage->m_transTexts.count(); ++i) {
        QTextEdit *transText = editorPage->m_transTexts[i]->editor();
        disconnect( transText->document(), SIGNAL(contentsChanged()), this, SLOT(emitTranslationChanged()) );
        transText->removeEventFilter(this);
    }
    editorPage->setNumerusForms(invariantForm, numerusForms);

    // reinstall event filters and set up the emitTranslationChanged slot
    for (int i = 0; i  < editorPage->m_transTexts.count(); ++i) {
        QTextEdit *transText = editorPage->m_transTexts[i]->editor();
        transText->installEventFilter(this);
        connect(transText->document(), SIGNAL(contentsChanged()),
            this, SLOT(emitTranslationChanged()));
        // What's this
        transText->setWhatsThis(tr("This is where you can enter or modify"
                                " the translation of some source text.") );
    }
}

static void visualizeImages(const QString &text, QTextEdit *te)
{
    te->clear();
    QTextCursor tc(te->textCursor());

    QString plainText;
    for (int i = 0; i < (int) text.length(); ++i)
    {
        int ch = text[i].unicode();
        if (ch < 0x20)
        {
            if (!plainText.isEmpty())
            {
                tc.insertText(plainText);
                plainText.clear();
            }
            const char *p = strchr(MessageEditor::backTab, ch);
            if (p)
            {
                if (backTabImages[p - MessageEditor::backTab]) {
                    tc.insertImage(QLatin1String(backTabImages[p - MessageEditor::backTab]));
                }
                if (MessageEditor::backTab[p - MessageEditor::backTab] == '\n') {
                    tc.insertBlock();
                }
            }
        }
        else
        {
            plainText += QString(ch);
        }
    }
    tc.insertText(plainText);
}

void MessageEditor::setTranslation(const QString &translation, int numerus, bool emitt)
{
    if (numerus >= editorPage->m_transTexts.count()) numerus = 0;
    QTextEdit *transText = editorPage->m_transTexts[numerus]->editor();
    // Block signals so that a signal is not emitted when
    // for example a new source text item is selected and *not*
    // the actual translation.
    if (!emitt)
        transText->document()->blockSignals(true);

    if (translation.isNull())
        transText->clear();
    else
        visualizeImages(translation, transText);

    if (!emitt)
    {
        transText->document()->blockSignals(false);

        //don't undo the change
        emit undoAvailable(false);
        emit redoAvailable(false);
        updateButtons();
    }
    emit cutAvailable(false);
    emit copyAvailable(false);
}

void MessageEditor::setEditionEnabled(bool enabled)
{
    for (int i = 0; i < editorPage->m_transTexts.count(); ++i) {
        TransEditor* te = editorPage->m_transTexts[i];
        te->label()->setEnabled(enabled);
        te->editor()->setReadOnly(!enabled);
    }

    phraseLbl->setEnabled(enabled);
    phraseTv->setEnabled(enabled);
    updateCanPaste();
}

void MessageEditor::undo()
{
    editorPage->activeTransText()->document()->undo();
}

void MessageEditor::redo()
{
    editorPage->activeTransText()->document()->redo();
}

void MessageEditor::cut()
{
    if (editorPage->activeTransText()->textCursor().hasSelection())
        editorPage->activeTransText()->cut();
}

void MessageEditor::copy()
{
    if (editorPage->srcText->textCursor().hasSelection()) {
        editorPage->srcText->copySelection();
    } else if (editorPage->activeTransText()->textCursor().hasSelection()) {
        editorPage->activeTransText()->copy();
    }
}

void MessageEditor::paste()
{
    editorPage->activeTransText()->paste();
}

void MessageEditor::selectAll()
{
    // make sure we don't select the selection of a translator textedit, if we really want the
    // source text editor to be selected.
    if (!editorPage->srcText->underMouse())
        editorPage->activeTransText()->selectAll();
}

void MessageEditor::emitTranslationChanged()
{
    emit translationChanged( editorPage->translations());
}

void MessageEditor::guessActivated(int key)
{
    QModelIndex mi;
    Phrase p;

    for (int i=0; i<phrMdl->phraseList().count(); ++i) {
        mi = phrMdl->QAbstractTableModel::index(i, 0);
        p = phrMdl->phrase(mi);
        if (p.shortcut() == key) {
            insertPhraseInTranslation(mi);
            break;
        }
    }
}

void MessageEditor::insertPhraseInTranslation(const QModelIndex &index)
{
    if (!editorPage->activeTransText()->isReadOnly())
    {
        editorPage->activeTransText()->textCursor().insertText(phrMdl->phrase(index).target());
        emitTranslationChanged();
    }
}

void MessageEditor::insertPhraseInTranslationAndLeave(const QModelIndex &index)
{
    if (!editorPage->activeTransText()->isReadOnly())
    {
        editorPage->activeTransText()->textCursor().insertText(phrMdl->phrase(index).target());
        
        emitTranslationChanged();
        editorPage->activeTransText()->setFocus();
    }
}

void MessageEditor::updateButtons()
{
    bool overwrite = (!editorPage->activeTransText()->isReadOnly() &&
             (editorPage->activeTransText()->toPlainText().trimmed().isEmpty() ||
              mayOverwriteTranslation));
    mayOverwriteTranslation = false;
    emit updateActions(overwrite);
}

void MessageEditor::beginFromSource()
{
    mayOverwriteTranslation = true;
    for ( int i = 0; i < editorPage->m_transTexts.count(); ++i) {
        setTranslation(sourceText, i, true);
    }
    setEditorFocus();
}

void MessageEditor::setEditorFocus()
{
    if (!editorPage->hasFocus())
        editorPage->activeTransText()->setFocus();
}

void MessageEditor::updateCutAndCopy()
{
    bool newCopyState = false;
    bool newCutState = false;
    if (editorPage->srcText->textCursor().hasSelection()) {
        newCopyState = true;
    } else if (editorPage->activeTransText()->textCursor().hasSelection()) {
        newCopyState = true;
        newCutState = true;
    }

    if (newCopyState != copyAvail) {
        copyAvail = newCopyState;
        emit copyAvailable(copyAvail);
    }

    if (newCutState != cutAvail) {
        cutAvail = newCutState;
        emit cutAvailable(cutAvail);
    }
}

void MessageEditor::clipboardChanged()
{
    // this is expensive, so move it out of the common path in updateCanPaste
    clipboardEmpty = qApp->clipboard()->text().isNull();
    updateCanPaste();
}

void MessageEditor::updateCanPaste()
{
    emit pasteAvailable(!editorPage->activeTransText()->isReadOnly() && !clipboardEmpty);
}

void MessageEditor::toggleGuessing()
{
    doGuesses = !doGuesses;
    if (!doGuesses) {
        phrMdl->removePhrases();
    }
}

void MessageEditor::setAltTextLabel(const QString &str)
{
    editorPage->altTextLbl->setText(str);
}

void MessageEditor::setTranslationLabel(const QString &str)
{
    for (int i = 0; i < editorPage->m_transTexts.count(); ++i) {
        TransEditor* te = editorPage->m_transTexts[i];
        te->label()->setText(str);
    }
}
