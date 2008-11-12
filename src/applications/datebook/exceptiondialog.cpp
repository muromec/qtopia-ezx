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

#include "exceptiondialog.h"

#include <qtopiaapplication.h>

/* TODO may want to add code that prevents all three checkboxes being
   deselected */
ExceptionDialog::ExceptionDialog( QWidget *parent, Qt::WFlags f )
    : QDialog( parent, f )
{
    setupUi( this );
}

int ExceptionDialog::exec(bool editMode)
{
    int series = 0;
    checkEarlier->setChecked(false);
    checkSelected->setChecked(true);
    checkLater->setChecked(true);

    if (editMode) {
        lblMessage->setText(tr("<qt>This appointment is part of a series. Select the part of the series you want to change below.</qt>"));
    } else {
        lblMessage->setText(tr("<qt>This appointment is part of a series. Select the part of the series you want to delete below.</qt>"));
    }
    if (!QtopiaApplication::execDialog( this ))
        return 0;

    if (checkSelected->isChecked())
        series |= Selected;
    if (checkLater->isChecked())
        series |= Later;
    if (checkEarlier->isChecked())
        series |= Earlier;
    return series;
}

int ExceptionDialog::result() const
{
    int series = 0;
    if (checkSelected->isChecked())
        series |= Selected;
    if (checkLater->isChecked())
        series |= Later;
    if (checkEarlier->isChecked())
        series |= Earlier;
    return series;
}
