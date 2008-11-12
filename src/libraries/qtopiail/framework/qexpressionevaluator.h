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

#ifndef _HAVE_FRAMEWORKEXPRESSION_H_
#define _HAVE_FRAMEWORKEXPRESSION_H_

/* Qt includes */
#include <QByteArray>
#include <QVariant>
#include <QObject>

/* Qtopia includes */
#ifdef QTOPIAIL
#include <qom.h>
#endif

#include "qtopiailglobal.h"

struct QExpressionEvaluatorPrivate;
#ifdef QTOPIAIL
class QAbstractExpressionEvaluatorTerm;
#endif

//========================================
//= Expression Declaration
//=======================================
class ExpressionTokenizer;
class QTOPIAIL_EXPORT QExpressionEvaluator : public QObject // FIXME : should just use QObject, but are there problems with Qom -> QObject connections? to test.
{
    Q_OBJECT
public:
    /* Public Data */
    enum FloatingPointFormat {
        Double,
        FixedPoint
    };

    /* Public Methods */
    /* Expression Ctors */
    explicit QExpressionEvaluator( QObject* parent = 0 );
    explicit QExpressionEvaluator( const QByteArray&, QObject* parent = 0 );
    /* Expression Dtor */
    ~QExpressionEvaluator();

    bool isValid() const;
    bool evaluate();
    QVariant result();

    void setFloatingPointFormat( const FloatingPointFormat& fmt );
    FloatingPointFormat floatingPointFormat() const;

    QByteArray expression() const;
public slots:
    bool setExpression( const QByteArray& expr );
    void clear();

#ifdef QTOPIAIL // FIXME : needed because QOM says this signature and termsChanged() are incompatible signals
private slots:
    void termChanged( const QReference<QAbstractExpressionEvaluatorTerm>& );
#endif

signals:
    void termsChanged();

private:
    /* Private Data */
    QExpressionEvaluatorPrivate* d;
};

//========================================
//= ExpressionToken Declaration
//=======================================
#ifdef QTOPIAIL
class QAbstractExpressionEvaluatorTerm : public QAbstractObject
{
    QOM_OBJECT
public:
    /* Public Methods */
    /* QAbstractExpressionEvaluatorTerm Ctors */
    QAbstractExpressionEvaluatorTerm() {}
    /* QAbstractExpressionEvaluatorTerm Dtor */
    virtual ~QAbstractExpressionEvaluatorTerm() {}

    enum DataType {
        String,
        Double,
        Integer,
        Bool
    };
    virtual bool canHandle( const QString& term ) = 0;
    virtual void setTerm( const QString& term) = 0;
    virtual QVariant value() = 0;

    virtual DataType dataType() const = 0;

    virtual bool canRead() const = 0;
    virtual bool canWrite() const = 0;

signals:
    void termChanged(QReference<QAbstractExpressionEvaluatorTerm>);
};
#endif

#endif
