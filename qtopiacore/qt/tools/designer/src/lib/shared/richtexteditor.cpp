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

/*
TRANSLATOR qdesigner_internal::RichTextEditorDialog
*/

#include <QtCore/QPointer>
#include <QtCore/QMap>

#include <QtGui/QToolBar>
#include <QtGui/QComboBox>
#include <QtGui/QAction>
#include <QtGui/QTextCursor>
#include <QtGui/QPainter>
#include <QtGui/QIcon>
#include <QtGui/QMoveEvent>
#include <QtGui/QTextDocument>
#include <QtGui/QTextBlock>
#include <QtGui/QVBoxLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QDialogButtonBox>

#include <QtCore/qdebug.h>

#include "iconloader_p.h"
#include "richtexteditor_p.h"

static bool operator < (const QColor &c1, const QColor &c2)
{
    if (c1.red() != c2.red())
        return c1.red() < c2.red();
    if (c1.green() != c2.green())
        return c1.green() < c2.green();
    return c1.blue() < c2.blue();
}

namespace qdesigner_internal {

class RichTextEditorToolBar : public QToolBar
{
    Q_OBJECT
public:
    RichTextEditorToolBar(RichTextEditor *editor, QWidget *parent = 0);

public slots:
    void updateActions();

private slots:
    void sizeInputActivated(const QString &size);
    void colorInputActivated(const QString &color);

private:
    QAction *m_bold_action;
    QAction *m_italic_action;
    QAction *m_underline_action;
    QComboBox *m_font_size_input;
    QComboBox *m_color_input;

    QPointer<RichTextEditor> m_editor;

    typedef QMap<QColor, QString> ColorMap;
    ColorMap m_color_map;
};

static QAction *createCheckableAction(const QIcon &icon, const QString &text,
                                    QObject *receiver, const char *slot,
                                    QObject *parent = 0)
{
    QAction *result = new QAction(parent);
    result->setIcon(icon);
    result->setText(text);
    result->setCheckable(true);
    result->setChecked(false);
    QObject::connect(result, SIGNAL(triggered(bool)), receiver, slot);
    return result;
}

static QIcon iconForColor(const QColor &color)
{
    QPixmap result(12, 12);
    QPainter painter(&result);
    painter.setPen(Qt::black);
    painter.setBrush(color);
    painter.drawRect(0, 0, result.width() - 1, result.height() - 1);
    painter.end();
    return QIcon(result);
}

RichTextEditorToolBar::RichTextEditorToolBar(RichTextEditor *editor,
                                                QWidget *parent)
    : QToolBar(parent)
{
    m_editor = editor;

    m_bold_action = createCheckableAction(createIconSet(QLatin1String("textbold.png")),
            tr("Bold"), editor, SLOT(setFontBold(bool)), this);
    m_bold_action->setShortcut(tr("CTRL+B"));
    addAction(m_bold_action);

    m_italic_action = createCheckableAction(createIconSet(QLatin1String("textitalic.png")),
            tr("Italic"), editor, SLOT(setFontItalic(bool)), this);
    m_italic_action->setShortcut(tr("CTRL+I"));

    addAction(m_italic_action);
    m_underline_action = createCheckableAction(createIconSet(QLatin1String("textunder.png")),
            tr("Underline"), editor, SLOT(setFontUnderline(bool)), this);
    m_underline_action->setShortcut(tr("CTRL+U"));
    addAction(m_underline_action);

    m_font_size_input = new QComboBox(this);
    m_font_size_input->setEditable(false);
    for (int i = 4; i < 30; ++i)
        m_font_size_input->addItem(QString::number(i));
    connect(m_font_size_input, SIGNAL(activated(QString)),
                this, SLOT(sizeInputActivated(QString)));
    addWidget(m_font_size_input);

    QStringList color_names = QColor::colorNames();
    color_names.removeAll(QLatin1String("transparent"));
    foreach (QString color, color_names)
        m_color_map.insert(QColor(color), color);

    m_color_input = new QComboBox(this);
    foreach (QString color, color_names)
        m_color_input->addItem(iconForColor(color), color);
    connect(m_color_input, SIGNAL(activated(QString)),
                this, SLOT(colorInputActivated(QString)));
    addWidget(m_color_input);

    connect(editor, SIGNAL(textChanged()), this, SLOT(updateActions()));

    updateActions();
}

void RichTextEditorToolBar::colorInputActivated(const QString &s)
{
    QColor color(s);
    if (!color.isValid())
        return;

    m_editor->setTextColor(color);
    m_editor->setFocus();
}

void RichTextEditorToolBar::sizeInputActivated(const QString &size)
{
    if (m_editor == 0)
        return;

    bool ok;
    int i = size.toInt(&ok);
    if (!ok)
        return;

    m_editor->setFontPointSize(i);
    m_editor->setFocus();
}

void RichTextEditorToolBar::updateActions()
{
    if (m_editor == 0) {
        setEnabled(false);
        return;
    }

    QTextCursor cursor = m_editor->textCursor();

    QTextCharFormat char_format = cursor.charFormat();

    m_bold_action->setChecked(char_format.fontWeight() == QFont::Bold);
    m_italic_action->setChecked(char_format.fontItalic());
    m_underline_action->setChecked(char_format.fontUnderline());

    int size = (int) char_format.fontPointSize();
    if (size == 0) // workaround for a bug in QTextEdit
        size = (int) m_editor->document()->defaultFont().pointSize();
    int idx = m_font_size_input->findText(QString::number(size));
    if (idx != -1)
        m_font_size_input->setCurrentIndex(idx);

    QString color = m_color_map.value(m_editor->textColor());
    idx = m_color_input->findText(color);
    m_color_input->setCurrentIndex(idx);
}

RichTextEditor::RichTextEditor(QWidget *parent)
    : QTextEdit(parent)
{
    connect(this, SIGNAL(currentCharFormatChanged(QTextCharFormat)),
                this, SIGNAL(textChanged()));
}

QToolBar *RichTextEditor::createToolBar(QWidget *parent)
{
    return new RichTextEditorToolBar(this, parent);
}

void RichTextEditor::setFontBold(bool b)
{
    if (b)
        setFontWeight(QFont::Bold);
    else
        setFontWeight(QFont::Normal);
}

void RichTextEditor::setFontPointSize(double d)
{
    QTextEdit::setFontPointSize(qreal(d));
}

void RichTextEditor::setText(const QString &text)
{
    if (Qt::mightBeRichText(text))
        setHtml(text);
    else
        setPlainText(text);
}

void RichTextEditor::setDefaultFont(const QFont &font)
{
    document()->setDefaultFont(font);
    if (font.pointSize() > 0)
        setFontPointSize(font.pointSize());
    else
        setFontPointSize(QFontInfo(font).pointSize());
    emit textChanged();
}

static bool compareFontSizes(const QFont &font1, const QFont &font2)
{
    int ps1 = font1.pointSize();
    if (ps1 == -1 && font1.pointSizeF() > 0)
        ps1 = (int) font1.pointSizeF();

    int ps2 = font2.pointSize();
    if (ps2 == -1 && font2.pointSizeF() > 0)
        ps2 = (int) font2.pointSizeF();

    if (ps1 != -1 || ps2 != -1)
        return ps1 == ps2;

    return font1.pixelSize() == font2.pixelSize();
}

static inline bool compareFonts(const QFont &font1, const QFont &font2)
{
    return font1.family() == font2.family()
            && compareFontSizes(font1, font2)
            && font1.bold() == font2.bold()
            && font1.italic() == font2.italic()
            && font1.overline() == font2.overline()
            && font1.underline() == font2.underline()
            && font1.strikeOut() == font2.strikeOut();
}

Qt::TextFormat RichTextEditor::detectFormat() const
{
    Qt::TextFormat result = Qt::PlainText;

    QFont default_font = document()->defaultFont();
    QTextCursor cursor(document()->begin());
    cursor.movePosition(QTextCursor::End);
    while (!cursor.atStart()) {
        QFont font = cursor.charFormat().font();
        if (!compareFonts(font, default_font)) {
            result = Qt::RichText;
            break;
        }
        cursor.movePosition(QTextCursor::Left);
    }

    return result;
};

QString RichTextEditor::text(Qt::TextFormat format) const
{
    bool richtext = true;

    if (format == Qt::PlainText)
        richtext = false;
    else if (format != Qt::RichText)
        richtext = detectFormat() == Qt::RichText;

    if (richtext)
        return toHtml();
    else
        return toPlainText();
}

RichTextEditorDialog::RichTextEditorDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Edit text"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setMargin(1);
    m_editor = new RichTextEditor(this);
    QToolBar *tool_bar = m_editor->createToolBar(this);
    tool_bar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
    layout->addWidget(tool_bar);
    layout->addWidget(m_editor);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok
                                                        | QDialogButtonBox::Cancel, Qt::Horizontal,
                                                       this);
    QPushButton *ok_button = buttonBox->button(QDialogButtonBox::Ok);
    ok_button->setText(tr("&OK"));
    ok_button->setDefault(true);
    buttonBox->button(QDialogButtonBox::Cancel)->setText(tr("&Cancel"));
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));

    layout->addWidget(buttonBox);
}

RichTextEditor *RichTextEditorDialog::editor()
{
    return m_editor;
}

} // namespace qdesigner_internal

#include "richtexteditor.moc"


