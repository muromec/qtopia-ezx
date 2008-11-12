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
TRNASLATOR qdesigner_internal::QtBrushDialog
*/

#include "qtbrushdialog.h"
#include "ui_qtbrushdialog.h"

#include "qdebug.h"

using namespace qdesigner_internal;

namespace qdesigner_internal {

class QtBrushDialogPrivate
{
    QtBrushDialog *q_ptr;
    Q_DECLARE_PUBLIC(QtBrushDialog)
public:
    Ui::QtBrushDialog m_ui;
};

}

QtBrushDialog::QtBrushDialog(QWidget *parent)
    : QDialog(parent)
{
    d_ptr = new QtBrushDialogPrivate();
    d_ptr->q_ptr = this;
    d_ptr->m_ui.setupUi(this);

    connect(d_ptr->m_ui.brushEditor, SIGNAL(textureChooserActivated(QWidget *, const QBrush &)),
            this, SIGNAL(textureChooserActivated(QWidget *, const QBrush &)));
}

QtBrushDialog::~QtBrushDialog()
{
    delete d_ptr;
}

void QtBrushDialog::setBrush(const QBrush &brush)
{
    d_ptr->m_ui.brushEditor->setBrush(brush);
}

QBrush QtBrushDialog::brush() const
{
    return d_ptr->m_ui.brushEditor->brush();
}

void QtBrushDialog::setBrushManager(QDesignerBrushManagerInterface *manager)
{
    d_ptr->m_ui.brushEditor->setBrushManager(manager);
}

#include "moc_qtbrushdialog.cpp"
