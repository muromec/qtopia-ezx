/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include "qpagesetupdialog.h"

#include "qcombobox.h"
#include "qcoreapplication.h"
#include "qlabel.h"
#include "qlayout.h"
#include "qprinter.h"
#include "qpushbutton.h"
#include "qprintdialog.h"

#include <private/qabstractpagesetupdialog_p.h>

#ifndef QT_NO_PRINTDIALOG

// Disabled untill we have support for papersources on unix
// #define PSD_ENABLE_PAPERSOURCE

static const char * const pageSizeNames[] = {
    QT_TRANSLATE_NOOP(QPrintDialog, "A4 (210 x 297 mm, 8.26 x 11.7 inches)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "B5 (176 x 250 mm, 6.93 x 9.84 inches)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "Letter (8.5 x 11 inches, 216 x 279 mm)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "Legal (8.5 x 14 inches, 216 x 356 mm)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "Executive (7.5 x 10 inches, 191 x 254 mm)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "A0 (841 x 1189 mm)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "A1 (594 x 841 mm)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "A2 (420 x 594 mm)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "A3 (297 x 420 mm)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "A5 (148 x 210 mm)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "A6 (105 x 148 mm)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "A7 (74 x 105 mm)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "A8 (52 x 74 mm)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "A9 (37 x 52 mm)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "B0 (1000 x 1414 mm)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "B1 (707 x 1000 mm)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "B2 (500 x 707 mm)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "B3 (353 x 500 mm)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "B4 (250 x 353 mm)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "B6 (125 x 176 mm)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "B7 (88 x 125 mm)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "B8 (62 x 88 mm)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "B9 (44 x 62 mm)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "B10 (31 x 44 mm)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "C5E (163 x 229 mm)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "US Common #10 Envelope (105 x 241 mm)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "DLE (110 x 220 mm)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "Folio (210 x 330 mm)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "Ledger (432 x 279 mm)"),
    QT_TRANSLATE_NOOP(QPrintDialog, "Tabloid (279 x 432 mm)"),
    0
};

#ifdef PSD_ENABLE_PAPERSOURCE
static const char *paperSourceNames[] = {
    "Only One",
    "Lower",
    "Middle",
    "Manual",
    "Envelope",
    "Envelope manual",
    "Auto",
    "Tractor",
    "Small format",
    "Large format",
    "Large capacity",
    "Cassette",
    "Form source",
    0
};

struct PaperSourceNames
{
    PaperSourceNames(const char *nam, QPrinter::PaperSource ps)
        : paperSource(ps), name(nam) {}
    QPrinter::PaperSource paperSource;
    const char *name;
};
#endif


class QPageSetupDialogPrivate : public QAbstractPageSetupDialogPrivate
{
public:
    QComboBox *pageSize;
#ifdef PSD_ENABLE_PAPERSOURCE
    QComboBox *paperSource;
#endif
    QComboBox *orientation;
};

/*!
    \class QPageSetupDialog

    \brief The QPageSetupDialog class provides a configuration dialog
    for the page-related options on a printer.

    On Windows and Mac OS X the page setup dialog is implemented using
    the native page setup dialogs.

    \sa QPrinter, QPrintDialog
*/

/*!
    Constructs a page setup dialog that configures \a printer with \a
    parent as the parent widget.
*/
QPageSetupDialog::QPageSetupDialog(QPrinter *printer, QWidget *parent)
    : QAbstractPageSetupDialog(*(new QPageSetupDialogPrivate), printer, parent)
{
    Q_D(QPageSetupDialog);
    QVBoxLayout *layout = new QVBoxLayout(this);

    QFrame *frame = new QFrame(this);
    frame->setFrameShape(QFrame::StyledPanel);
    frame->setFrameShadow(QFrame::Sunken);
    QGridLayout *frameLayout = new QGridLayout(frame);

    QLabel *pageSizeLabel = new QLabel(QPrintDialog::tr("Page size:"), frame);
    d->pageSize = new QComboBox(frame);
    frameLayout->addWidget(pageSizeLabel, 0, 0);
    frameLayout->addWidget(d->pageSize, 0, 1);

    QLabel *orientationLabel = new QLabel(QPrintDialog::tr("Orientation:"), frame);
    d->orientation = new QComboBox(frame);
    frameLayout->addWidget(orientationLabel, 2, 0);
    frameLayout->addWidget(d->orientation, 2, 1);

#ifdef PSD_ENABLE_PAPERSOURCE
    QLabel *paperSourceLabel = new QLabel(QPrintDialog::tr("Paper source:"), frame);
    d->paperSource = new QComboBox(frame);
    frameLayout->addWidget(paperSourceLabel, 1, 0);
    frameLayout->addWidget(d->paperSource, 1, 1);
#endif

    layout->addWidget(frame);

    QSpacerItem* spacer = new QSpacerItem(20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding);
    layout->addItem(spacer);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    QSpacerItem *buttonSpacer = new QSpacerItem(71, 20, QSizePolicy::Expanding,
                                                QSizePolicy::Minimum);
    QPushButton *okButton = new QPushButton(QPrintDialog::tr("OK"), this);
    QPushButton *cancelButton = new QPushButton(QPrintDialog::tr("Cancel"), this);

    buttonLayout->addItem(buttonSpacer);
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    layout->addLayout(buttonLayout);

    setAttribute(Qt::WA_WState_Polished, false);

    for (int i = 0; pageSizeNames[i]; ++i)
        d->pageSize->addItem(QCoreApplication::translate("QPrintDialog", pageSizeNames[i]));
    d->pageSize->setCurrentIndex(printer->pageSize());

#ifdef PSD_ENABLE_PAPERSOURCE
    for (int i=0; paperSourceNames[i]; ++i)
        d->paperSource->insertItem(paperSourceNames[i]);
    d->paperSource->setCurrentItem(printer->paperSource());
#endif

    d->orientation->addItem(QPrintDialog::tr("Portrait"));
    d->orientation->addItem(QPrintDialog::tr("Landscape"));
    d->orientation->setCurrentIndex(printer->orientation());

    connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
    connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
}

/*!
    \internal
*/
int QPageSetupDialog::exec()
{
    Q_D(QPageSetupDialog);

    if (d->printer->outputFormat() != QPrinter::NativeFormat) 
        return Rejected;

    int ret = QDialog::exec();
    if (ret == Accepted) {
        // Read out the data
        d->printer->setPageSize((QPrinter::PageSize)d->pageSize->currentIndex());
        d->printer->setOrientation((QPrinter::Orientation)d->orientation->currentIndex());
#ifdef PSD_ENABLE_PAPERSOURCE
        d->printer->setPaperSource((QPrinter::PaperSource)d->paperSource->currentIndex());
#endif
    }
    return ret;
}

/*!
    \fn QPrinter *QPageSetupDialog::printer()

    Returns the printer that was passed to the QPageSetupDialog
    constructor.
*/

#endif // QT_NO_PRINTDIALOG
