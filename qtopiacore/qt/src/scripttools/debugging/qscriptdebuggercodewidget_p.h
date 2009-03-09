/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the QtScriptTools module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial Usage
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain
** additional rights. These rights are described in the Nokia Qt LGPL
** Exception version 1.0, included in the file LGPL_EXCEPTION.txt in this
** package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSCRIPTDEBUGGERCODEWIDGET_P_H
#define QSCRIPTDEBUGGERCODEWIDGET_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qscriptdebuggercodewidgetinterface_p.h"

QT_BEGIN_NAMESPACE

class QScriptDebuggerCodeWidgetPrivate;
class Q_AUTOTEST_EXPORT QScriptDebuggerCodeWidget:
    public QScriptDebuggerCodeWidgetInterface
{
    Q_OBJECT
public:
    QScriptDebuggerCodeWidget(QWidget *parent = 0);
    ~QScriptDebuggerCodeWidget();

    QScriptDebuggerScriptsModel *scriptsModel() const;
    void setScriptsModel(QScriptDebuggerScriptsModel *model);

    void setToolTipProvider(QScriptToolTipProviderInterface *toolTipProvider);

    qint64 currentScriptId() const;
    void setCurrentScript(qint64 scriptId);

    void invalidateExecutionLineNumbers();

    QScriptBreakpointsModel *breakpointsModel() const;
    void setBreakpointsModel(QScriptBreakpointsModel *model);

    QScriptDebuggerCodeViewInterface *currentView() const;

private:
    Q_DECLARE_PRIVATE(QScriptDebuggerCodeWidget)
    Q_DISABLE_COPY(QScriptDebuggerCodeWidget)

    Q_PRIVATE_SLOT(d_func(), void _q_onBreakpointToggleRequest(int,bool))
    Q_PRIVATE_SLOT(d_func(), void _q_onBreakpointEnableRequest(int,bool))
    Q_PRIVATE_SLOT(d_func(), void _q_onBreakpointsAboutToBeRemoved(const QModelIndex &,int,int))
    Q_PRIVATE_SLOT(d_func(), void _q_onBreakpointsInserted(const QModelIndex &,int,int))
    Q_PRIVATE_SLOT(d_func(), void _q_onBreakpointsDataChanged(const QModelIndex &, const QModelIndex &))
    Q_PRIVATE_SLOT(d_func(), void _q_onScriptsChanged())
    Q_PRIVATE_SLOT(d_func(), void _q_onToolTipRequest(const QPoint &, int, const QStringList &))
};

QT_END_NAMESPACE

#endif
