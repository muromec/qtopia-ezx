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

#include <contactsource.h>
#include <qcontactmodel.h>
#include <qpimsourcemodel.h>

#include <qsoftmenubar.h>

#include <QListView>
#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QVBoxLayout>

ContactSourceDialog::ContactSourceDialog(QWidget *parent)
    : QPimSourceDialog(parent), contactModel(0)
{
    setWindowTitle(tr("Show Contacts From", "e.g. Show Contacts From Phone/SIM Card"));

    QMenu* contextMenu = QSoftMenuBar::menuFor(this);
    QAction *actionCopyFromSim = new QAction(QIcon(":icon/sync"), tr("Import from SIM"), this);
    actionCopyFromSim->setWhatsThis(tr("Copy all entries from the SIM card to the Phone"));

    connect(actionCopyFromSim, SIGNAL(triggered()), this, SLOT(importActiveSim()));

    QAction *actionCopyToSim = new QAction(QIcon(":icon/sync"), tr("Export to SIM"), this);
    actionCopyToSim->setWhatsThis(tr("<qt>Copy all currently shown entries from the Phone to the SIM card. "
                "Entries not shown due to category filtering will not be copied.</qt>"));
    connect(actionCopyToSim, SIGNAL(triggered()), this, SLOT(exportActiveSim()));

    contextMenu->addAction(actionCopyFromSim);
    contextMenu->addAction(actionCopyToSim);
}

ContactSourceDialog::~ContactSourceDialog()
{
}

void ContactSourceDialog::setPimModel(QPimModel *m)
{
    contactModel = qobject_cast<QContactModel*>(m);
    Q_ASSERT(contactModel);
    QPimSourceDialog::setPimModel(m);
}

void ContactSourceDialog::importActiveSim()
{
    if(contactModel)
        contactModel->mirrorAll(contactModel->simSource(), contactModel->defaultSource());
}

void ContactSourceDialog::exportActiveSim()
{
    if (contactModel) {
        if (!contactModel->mirrorAll(contactModel->defaultSource(), contactModel->simSource())) {
            QMessageBox::warning(this, tr("Contacts"),
                tr("<qt>Could not export contacts to SIM Card.  Please ensure sufficient"
                    " space is available on SIM Card.</qt>"));
        }
    }
}

