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
TRANSLATOR qdesigner_internal::StyleSheetEditorDialog
*/

#include "stylesheeteditor_p.h"
#include "csshighlighter_p.h"

#include <QtDesigner/QDesignerFormWindowInterface>
#include <QtDesigner/QDesignerFormWindowCursorInterface>
#include <QtDesigner/QDesignerFormEditorInterface>
#include <QtDesigner/QDesignerPropertySheetExtension>
#include <QtDesigner/QExtensionManager>

#include <QtGui/QDialogButtonBox>
#include <QtGui/QPushButton>
#include <QtGui/QVBoxLayout>
#include <QtGui/QMessageBox>
#include "private/qcssparser_p.h"

namespace qdesigner_internal {

StyleSheetEditor::StyleSheetEditor(QWidget *parent)
    : QTextEdit(parent)
{
    setTabStopWidth(fontMetrics().width(QLatin1Char(' '))*4);
}

StyleSheetEditorDialog::StyleSheetEditorDialog(QWidget *fw, QWidget *widget)
    : QDialog(fw), m_widget(widget)
{
    m_fw = qobject_cast<QDesignerFormWindowInterface *>(fw);
    Q_ASSERT(m_fw != 0);
    setWindowTitle(tr("Edit Style Sheet"));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    QGridLayout *layout = new QGridLayout;
    m_editor = new StyleSheetEditor;
    new CssHighlighter(m_editor->document());
    QDialogButtonBox *buttonBox = new QDialogButtonBox;
    buttonBox->setStandardButtons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply);
    QObject::connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    QObject::connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    QPushButton *apply = buttonBox->button(QDialogButtonBox::Apply);
    QObject::connect((const QObject *)apply, SIGNAL(clicked()), this, SLOT(applyStyleSheet()));
    QObject::connect(buttonBox, SIGNAL(accepted()), this, SLOT(applyStyleSheet()));

    validityLabel = new QLabel(tr("Valid Style Sheet"));
    QObject::connect(m_editor, SIGNAL(textChanged()), this, SLOT(validateStyleSheet()));

    layout->addWidget(m_editor, 0, 0, 1, 2);;
    layout->addWidget(validityLabel, 1, 0, 1, 1);
    layout->addWidget(buttonBox, 1, 1, 1, 1);
    setLayout(layout);

    QDesignerPropertySheetExtension *sheet = qt_extension<QDesignerPropertySheetExtension*>(m_fw->core()->extensionManager(), m_widget);
    Q_ASSERT(sheet != 0);
    m_editor->setText(sheet->property(sheet->indexOf(QLatin1String("styleSheet"))).toString());

    m_editor->setFocus();
    resize(430, 330);
}

StyleSheetEditor *StyleSheetEditorDialog::editor() const
{
    return m_editor;
}

void StyleSheetEditorDialog::applyStyleSheet()
{
    QString text = m_editor->toPlainText();
    m_fw->cursor()->setWidgetProperty(m_widget, QLatin1String("styleSheet"), QVariant(text));
}

bool StyleSheetEditorDialog::isStyleSheetValid(const QString &styleSheet)
{
    QCss::Parser parser(styleSheet);
    QCss::StyleSheet sheet;
    if (parser.parse(&sheet))
        return true;
    QString fullSheet = QLatin1String("* { ");
    fullSheet += styleSheet;
    fullSheet += QLatin1Char('}');
    QCss::Parser parser2(fullSheet);
    return parser2.parse(&sheet);
}

void StyleSheetEditorDialog::validateStyleSheet()
{
    QString text = m_editor->toPlainText();
    if (!isStyleSheetValid(text)) {
        validityLabel->setText(tr("Invalid Style Sheet"));
        validityLabel->setStyleSheet(QLatin1String("color: red"));
    } else {
        validityLabel->setText(tr("Valid Style Sheet"));
        validityLabel->setStyleSheet(QLatin1String("color: green"));
    }
}

} // namespace qdesigner_internal
