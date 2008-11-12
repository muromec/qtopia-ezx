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

#ifndef CLEANUPWIZARD_H
#define CLEANUPWIZARD_H

#include <qwidget.h>

class QWidgetStack;
class PreselectionWidget;
class DocCleanWidget;
class DocResultWidget;
class DocSummaryWidget;
class MailCleanWidget;
class DatebookCleanWidget;
class FinalCleanupWidget;

class CleanupWizard : public QWidget
{
    Q_OBJECT
public:
    CleanupWizard(QWidget* parent = 0, const char* name = 0, WFlags fl = 0 );
    virtual ~CleanupWizard();
protected:
    void keyPressEvent(QKeyEvent *keyEvent);
    void showEvent(QShowEvent *);
    bool eventFilter(QObject *o, QEvent *e);
private slots:
    void addToFinalSummary(int noDeletedDocs);
    void init();

private:
    enum WizardStyle { Default, NoBack, NoForward};
    void setContextBar(WizardStyle style);

    QWidgetStack * wStack;
    PreselectionWidget *m_PreselectionWidget;
    DocCleanWidget *m_DocCleanWidget;
    DocResultWidget *m_DocResultWidget;
    DocSummaryWidget * m_DocSummaryWidget;
    MailCleanWidget * m_MailCleanWidget;
    DatebookCleanWidget *m_DatebookCleanWidget;
    FinalCleanupWidget *m_FinalCleanupWidget;
};

#endif
