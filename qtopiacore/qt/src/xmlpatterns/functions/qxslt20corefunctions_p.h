/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
** Contact: Qt Software Information (qt-info@nokia.com)
**
** This file is part of the QtXMLPatterns module of the Qt Toolkit.
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

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.

#ifndef Patternist_XSLT20CoreFunctions_H
#define Patternist_XSLT20CoreFunctions_H

#include "qabstractfunctionfactory_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

namespace QPatternist
{

    /**
     * @short Handles the functions defines in XSL-T 2.0, except those also available in XPath 2.0.
     *
     * @note XPath20CoreFunctions inherits from XPath10CoreFunctions only for implementation
     * reasons, it does not supply the functions in the XPath10CoreFunctions factory.
     *
     * @see <a href ="http://www.w3.org/TR/xpath-functions/">XQuery 1.0
     * and XPath 2.0 Functions and Operators</a>
     * @see <a href="http://www.w3.org/TR/xpath.html#corelib">XML Path Language (XPath)
     * Version 1.0, 4 Core Function Library</a>
     * @author Frans Englich <fenglich@trolltech.com>
     * @ingroup Patternist_functions
     * @since 4.5
     */
    class XSLT20CoreFunctions : public AbstractFunctionFactory
    {
    protected:
        virtual Expression::Ptr retrieveExpression(const QXmlName name,
                                                   const Expression::List &args,
                                                   const FunctionSignature::Ptr &sign) const;

        virtual FunctionSignature::Ptr retrieveFunctionSignature(const NamePool::Ptr &np,
                                                                 const QXmlName name);
    };
}

QT_END_NAMESPACE

QT_END_HEADER

#endif
