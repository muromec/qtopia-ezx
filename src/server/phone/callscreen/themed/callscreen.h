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

#ifndef CALLSCREEN_H
#define CALLSCREEN_H

#include <themedview.h>

#include <QListWidget>
#include "serverthemeview.h"
#include <QPhoneCall>
#ifdef QTOPIA_CELL
#include <QSimControlEvent>
#endif

class DialerControl;
class QMenu;
class CallItemEntry;
class QAction;
class QLineEdit;
class QVBoxLayout;
class CallItemModel;
class SecondaryCallScreen;
class ThemeListModel;
class CallAudioHandler;
class QAudioStateConfiguration;
class QPhoneCall;

class QSimToolkit;
class QAbstractMessageBox;
class MouseControlDialog;

class CallScreen : public PhoneThemedView
{
    friend class CallItemListView;
    friend class CallItemEntry;

    Q_OBJECT
public:
    CallScreen(DialerControl *ctrl, QWidget *parent, Qt::WFlags fl = 0);

    QString ringTone();
    int activeCallCount() const { return activeCount; }
    int heldCallCount() const { return holdCount; }
    bool incomingCall() const { return incoming; }
    bool inMultiCall() const { return activeCount > 1 || holdCount > 1; }

signals:
    void acceptIncoming();
    void muteRing();
    void listEmpty();
    void testKeys(const QString&, bool&);
    void filterKeys(const QString&, bool&);
    void filterSelect(const QString&, bool&);

public slots:
    void stateChanged();
    void requestFailed(const QPhoneCall &,QPhoneCall::Request);
    CallItemEntry *findCall(const QPhoneCall &call, CallItemModel *model);

protected slots:
    void manualLayout();

protected:
    virtual void themeLoaded( const QString &theme );
    QWidget *newWidget(ThemeWidgetItem* input, const QString& name);
    void showEvent(QShowEvent *);
    void keyPressEvent(QKeyEvent *);
    void keyReleaseEvent(QKeyEvent *);
    void closeEvent(QCloseEvent *);
    bool eventFilter(QObject *, QEvent *);
    void hideEvent( QHideEvent * );
    void mousePressEvent(QMouseEvent *);

private slots:
    void updateAll();
    void splitCall();
    void callSelected(const QModelIndex&);
    void callClicked(const QModelIndex&);
    void themeItemClicked(ThemeItem*);
    void actionGsmSelected();
    void updateLabels();
    void setVideoWidget();
    void deleteVideoWidget();
    void initializeAudioConf();
#ifdef QTOPIA_CELL
    void simControlEvent(const QSimControlEvent &);
#endif
    void grabMouse();
    void releaseMouse();
    void muteRingSelected();
    void callConnected(const QPhoneCall &);
    void callDropped(const QPhoneCall &);
    void callIncoming(const QPhoneCall &);
    void callDialing(const QPhoneCall &);
    void showProgressDlg();
    void hideProgressDlg();
    void interactionDelayTimeout();
    void rejectModalDialog();
    void initializeMouseControlDialog();

private:
    void clearDtmfDigits(bool clearOneChar = false);
    void appendDtmfDigits(const QString &);
    void setSelectMode(bool);
    int callListHeight() const;
    bool dialNumbers(const QString & numbers);
    void setGsmMenuItem();
    void setItemActive(const QString &name, bool active);

private:
    QString dtmfActiveCall;
    QString dtmfDigits;
    DialerControl *control;
    QLineEdit *digits;
    QListView *listView;
    QMenu *contextMenu;
    QAction *actionAnswer;
    QAction *actionSendBusy;
    QAction *actionMute;
    QAction *actionHold;
    QAction *actionResume;
    QAction *actionEnd;
    QAction *actionEndAll;
    QAction *actionMerge;
    QAction *actionSplit;
    QAction *actionTransfer;
    QAction *actionGsm;
    int activeCount;
    int holdCount;
    bool incoming;
    bool keypadVisible;
    QVBoxLayout *mLayout;
    QTimer* updateTimer;
    QTimer* gsmActionTimer;
    SecondaryCallScreen *secondaryCallScreen;
    ThemeListModel* m_model;
    CallAudioHandler* m_callAudioHandler;
    QWidget* videoWidget;
    QAudioStateConfiguration *m_audioConf;
#ifdef QTOPIA_CELL
    QSimToolkit *simToolkit;
#endif
    QAbstractMessageBox *simMsgBox;
    bool showWaitDlg;
    QTimer *symbolTimer;
    MouseControlDialog *m_mouseCtrlDlg;
};

#endif
