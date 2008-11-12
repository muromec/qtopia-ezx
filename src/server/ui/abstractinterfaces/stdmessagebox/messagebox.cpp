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

#include "messagebox.h"

#include <qtopiaapplication.h>
#include <qsoftmenubar.h>
#include <qtopialog.h>

#include <qmessagebox.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qevent.h>
#include <QPixmap>

// declare PhoneMessageBoxPrivate
class PhoneMessageBoxPrivate
{
public:
    PhoneMessageBoxPrivate()
    : iconLabel(0),
    msg(0),
    btn0(PhoneMessageBox::NoButton),
    btn1(PhoneMessageBox::NoButton),
    btn2(PhoneMessageBox::NoButton),
    defaultBtnNum(0),
    escapeBtnNum(-1),
    customButton(false),
    icon(PhoneMessageBox::NoIcon),
    yesKey(0),
    vbox(0) {}

    QString title;

    QLabel *iconLabel;
    QLabel *msg;

    PhoneMessageBox::Button btn0;
    PhoneMessageBox::Button btn1;
    PhoneMessageBox::Button btn2;
    int defaultBtnNum;
    int escapeBtnNum;
    bool customButton;

    PhoneMessageBox::Icon icon;

    int yesKey;

    QVBoxLayout *vbox;
};


/*!
  \class PhoneMessageBox
  \brief The PhoneMessageBox class implements the Qtopia Phone message box.
  \ingroup QtopiaServer::PhoneUI
  
  This class is a Qtopia \l{QtopiaServerApplication#qtopia-server-widgets}{server widget}. 
  It is part of the Qtopia server and cannot be used by other Qtopia applications.

  \sa QAbstractServerInterface, QAbstractMessageBox
  */

/*!
  Constructs a new PhoneMessageBox instance with the specified \a parent
  and widget \a flags.
  */
PhoneMessageBox::PhoneMessageBox(QWidget *parent, Qt::WFlags flags)
: QAbstractMessageBox(parent, flags)
{
    d = new PhoneMessageBoxPrivate;

    d->vbox = new QVBoxLayout(this);
    QWidget *messageArea = new QWidget(this);
    d->vbox->addWidget(messageArea);
    QHBoxLayout *hb = new QHBoxLayout(messageArea);
    hb->setMargin(6);
    hb->setSpacing(6);
    d->iconLabel = new QLabel(messageArea);
    d->iconLabel->setAlignment(Qt::AlignTop | Qt::AlignHCenter);
    hb->addWidget(d->iconLabel);
    d->msg = new QLabel(messageArea);
    d->msg->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    d->msg->setWordWrap(true);
    hb->addWidget(d->msg, 100);

    if(parent)
        parent->installEventFilter(this);
}

/*!
  \reimp
  */
void PhoneMessageBox::setButtons(Button button0, Button button1)
{
    d->btn0 = button0;
    d->btn1 = button1;

    if(d->btn1 != NoButton) {
        if(d->btn0 == Yes || d->btn0 == Ok) {
            const QList<int> &cbtns = QSoftMenuBar::keys();
            if (cbtns.count()) {
                if (cbtns[0] != Qt::Key_Back)
                    d->yesKey = cbtns[0];
                else if (cbtns.count() > 1)
                    d->yesKey = cbtns[cbtns.count()-1];
            }
            if(d->yesKey)
                QSoftMenuBar::setLabel(this, d->yesKey, "", d->btn0 == Yes ? tr("Yes") : tr("OK"));
        }
        if (d->btn1 == No || d->btn1 == Cancel) {
            QSoftMenuBar::setLabel(this, Qt::Key_Back, "", d->btn1 == No ? tr("No") : tr("Cancel"));
        }
    }
}

/*!
  \reimp
  */
void PhoneMessageBox::setButtons(const QString &button0Text, const QString &button1Text, const QString &button2Text,
        int defaultButtonNumber, int escapeButtonNumber)
{
    d->customButton = true;

    d->btn0 = button0Text.isEmpty() ? (Button)-2 : (Button)0;
    d->btn1 = button1Text.isEmpty() ? (Button)-2 : (Button)1;
    d->btn2 = button2Text.isEmpty() ? (Button)-2 : (Button)2;

    d->defaultBtnNum = defaultButtonNumber;
    d->escapeBtnNum = escapeButtonNumber;

    if (button1Text.isEmpty() && button2Text.isEmpty()) {
        QSoftMenuBar::setLabel(this, Qt::Key_Back, "", button0Text.isEmpty() ? QString() : button0Text);
        return;
    }

    if (Qtopia::hasKey(Qt::Key_Context1))
        QSoftMenuBar::setLabel(this, Qt::Key_Context1, "", button0Text.isEmpty() ? QString() : button0Text);
    else if (Qtopia::hasKey(Qt::Key_Menu))
        QSoftMenuBar::setLabel(this, Qt::Key_Menu, "", button0Text.isEmpty() ? QString() : button0Text);
    else
        qLog(UI) << "Cannot set context label" << button0Text;

    if (!button1Text.isEmpty() && button2Text.isEmpty()) {
        QSoftMenuBar::setLabel(this, Qt::Key_Back, "", button1Text);
    } else if (button1Text.isEmpty() && !button2Text.isEmpty()) {
        QSoftMenuBar::setLabel(this, Qt::Key_Back, "", button2Text);
    } else {
        QSoftMenuBar::setLabel(this, Qt::Key_Select, "", button1Text);
        QSoftMenuBar::setLabel(this, Qt::Key_Back, "", button2Text);
    }
}

/*!
  \reimp
  */
QString PhoneMessageBox::title() const
{
    return d->title;
}

/*!
  \reimp
  */
void PhoneMessageBox::setTitle(const QString &title)
{
    d->title = title;
    setWindowTitle(title);
}

/*!
  \reimp
  */
PhoneMessageBox::Icon PhoneMessageBox::icon() const
{
    return d->icon;
}

/*!
  \reimp
  */
void PhoneMessageBox::setIcon(Icon i)
{
    d->icon = i;
    QPixmap pm;
    switch (d->icon) {
        case Information:
            pm = QPixmap(":image/alert_info");
            break;
        case Warning:
            pm = QPixmap(":image/alert_warning");
            break;
        default:
            pm = QMessageBox::standardIcon((QMessageBox::Icon)d->icon);
            break;
    }
    d->iconLabel->setPixmap(pm);
}

/*!
  \reimp
  */
QString PhoneMessageBox::text() const
{
    return d->msg->text();
}

/*!
  \reimp
  */
void PhoneMessageBox::setText(const QString &text)
{
    d->msg->setText(text);
}

/*!
  \internal
  */
void PhoneMessageBox::keyPressEvent(QKeyEvent *ke)
{
    if (d->customButton) {
        if (ke->key() == Qt::Key_Return || ke->key() == Qt::Key_Enter) {
            done(d->defaultBtnNum);
            ke->accept();
        } else if (ke->key() == Qt::Key_Escape) {
            done(d->escapeBtnNum);
            ke->accept();
        } else if (ke->key() == Qt::Key_Context1) {
            if (d->btn0 != (Button)-2 && (d->btn1 != (Button)-2 || d->btn2 != (Button)-2))
                done(d->btn0);
            ke->accept();
        } else if (ke->key() == Qt::Key_Select) {
            if (d->btn0 != (Button)-2 && d->btn1 != (Button)-2 && d->btn2 != (Button)-2)
                done(d->btn1);
            ke->accept();
        } else if (ke->key() == Qt::Key_Back) {
            if (d->btn0 != (Button)-2 && d->btn1 == (Button)-2 && d->btn2 == (Button)-2)
                done(d->btn0);
            else if (d->btn0 != (Button)-2 && d->btn1 != (Button)-2 && d->btn2 == (Button)-2)
                done(d->btn1);
            else if (d->btn0 == (Button)-2 && d->btn1 != (Button)-2 && d->btn2 == (Button)-2)
                done(d->btn1);
            else if (d->btn0 != (Button)-2 && d->btn1 == (Button)-2 && d->btn2 != (Button)-2)
                done(d->btn2);
            else if (d->btn0 != (Button)-2 && d->btn1 != (Button)-2 && d->btn2 != (Button)-2)
                done(d->btn2);
            else if (d->btn0 == (Button)-2 && d->btn1 == (Button)-2 && d->btn2 != (Button)-2)
                done(d->btn2);
            else if (d->btn0 == (Button)-2 && d->btn1 != (Button)-2 && d->btn2 != (Button)-2)
                done(d->btn2);
            ke->accept();
        } else if (ke->key() == Qt::Key_Hangup) {
            done(d->escapeBtnNum);
            ke->accept();
        }
        return;
    }

    if (ke->key() == Qt::Key_Yes || ke->key() == d->yesKey) {
        done(d->btn0);
        ke->accept();
    } else if (ke->key() == Qt::Key_No || ke->key() == Qt::Key_Back) {
        if (d->btn1 != NoButton)
            done(d->btn1);
        else
            done(d->btn0);
        ke->accept();
    }
}

/*!
  \internal
  */
bool PhoneMessageBox::eventFilter(QObject *, QEvent *e)
{
    if (e->type() == QEvent::WindowActivate && isVisible()) {
        raise();
    }

    return false;
}

/*!
  \internal
  */
void PhoneMessageBox::addContents(QWidget *c)
{
    d->vbox->addWidget(c);
}

QTOPIA_REPLACE_WIDGET(QAbstractMessageBox, PhoneMessageBox);
