/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qmetatype.h"
#include "qobjectdefs.h"
#include "qdatetime.h"
#include "qbytearray.h"
#include "qreadwritelock.h"
#include "qstring.h"
#include "qstringlist.h"
#include "qvector.h"
#include "qlocale.h"

#ifdef QT_BOOTSTRAPPED
# ifndef QT_NO_GEOM_VARIANT
#  define QT_NO_GEOM_VARIANT
# endif
#else
#  include "qbitarray.h"
#  include "qurl.h"
#  include "qvariant.h"
#endif

#ifndef QT_NO_GEOM_VARIANT
#include "qsize.h"
#include "qpoint.h"
#include "qrect.h"
#include "qline.h"
#endif

/*!
    \macro Q_DECLARE_METATYPE(Type)
    \relates QMetaType

    This macro makes the type \a Type known to QMetaType. It is
    needed to use the type \a Type as a custom type in QVariant.

    Ideally, this macro should be placed below the declaration of
    the class or struct. If that is not possible, it can be put in
    a private header file which has to be included every time that
    type is used in a QVariant.

    Adding a Q_DECLARE_METATYPE() makes the type known to all template
    based functions, including QVariant. Note that if you intend to
    use the type in \e queued signal and slot connections, you also
    have to call qRegisterMetaType() since such connections are
    resolved at runtime.

    This example shows a typical use case of Q_DECLARE_METATYPE():

    \code
        struct MyStruct
        {
            int i;
            ...
        };

        Q_DECLARE_METATYPE(MyStruct)
    \endcode

    If \c MyStruct is in a namespace, the Q_DECLARE_METATYPE() macro
    has to be outside the namespace:

    \code
        namespace MyNamespace
        {
            ...
        }

        Q_DECLARE_METATYPE(MyNamespace::MyStruct)
    \endcode

    Since \c{MyStruct} is now known to QMetaType, it can be used in QVariant:

    \code
        MyStruct s;
        QVariant var;
        var.setValue(s); // copy s into the variant

        ...

        // retrieve the value
        MyStruct s2 = var.value<MyStruct>();
    \endcode

    \sa qRegisterMetaType()
*/

/*!
    \enum QMetaType::Type

    These are the built-in types supported by QMetaType:

    \value Void \c void
    \value Bool \c bool
    \value Int \c int
    \value UInt \c{unsigned int}
    \value Double \c double
    \value QChar QChar
    \value QString QString
    \value QByteArray QByteArray

    \value VoidStar \c{void *}
    \value Long \c{long}
    \value LongLong LongLong
    \value Short \c{short}
    \value Char \c{char}
    \value ULong \c{unsigned long}
    \value ULongLong ULongLong
    \value UShort \c{unsigned short}
    \value UChar \c{unsigned char}
    \value Float \c float
    \value QObjectStar QObject *
    \value QWidgetStar QWidget *

    \value QColorGroup QColorGroup
    \value QCursor QCursor
    \value QDate QDate
    \value QSize QSize
    \value QTime QTime
    \value QVariantList QVariantList
    \value QPolygon QPolygon
    \value QColor QColor
    \value QSizeF QSizeF
    \value QRectF QRectF
    \value QLine QLine
    \value QTextLength QTextLength
    \value QStringList QStringList
    \value QVariantMap QVariantMap
    \value QIcon QIcon
    \value QPen QPen
    \value QLineF QLineF
    \value QTextFormat QTextFormat
    \value QRect QRect
    \value QPoint QPoint
    \value QUrl QUrl
    \value QRegExp QRegExp
    \value QDateTime QDateTime
    \value QPointF QPointF
    \value QPalette QPalette
    \value QFont QFont
    \value QBrush QBrush
    \value QRegion QRegion
    \value QBitArray QBitArray
    \value QImage QImage
    \value QKeySequence QKeySequence
    \value QSizePolicy QSizePolicy
    \value QPixmap QPixmap
    \value QLocale QLocale
    \value QBitmap QBitmap
    \value QMatrix QMatrix
    \value QTransform QTransform

    \value User  Base value for user types

    \omitvalue FirstCoreExtType
    \omitvalue FirstGuiType
    \omitvalue LastCoreExtType
    \omitvalue LastCoreType
    \omitvalue LastGuiType

    Additional types can be registered using Q_DECLARE_METATYPE().

    \sa type(), typeName()
*/

/*!
    \class QMetaType
    \brief The QMetaType class manages named types in the meta-object system.

    \ingroup objectmodel
    \threadsafe

    The class is used as a helper to marshall types in QVariant and
    in queued signals and slots connections. It associates a type
    name to a type so that it can be created and destructed
    dynamically at run-time. Declare new types with Q_DECLARE_METATYPE()
    to make them available to QVariant and other template-based functions.
    Call qRegisterMetaType() to make type available to non-template based
    functions, such as the queued signal and slot connections.

    Any class or struct that has a public default
    constructor, a public copy constructor, and a public destructor
    can be registered.

    The following code allocates and destructs an instance of
    \c{MyClass}:

    \code
        int id = QMetaType::type("MyClass");
        if (id != -1) {
            void *myClassPtr = QMetaType::construct(id);
            ...
            QMetaType::destroy(id, myClassPtr);
            myClassPtr = 0;
        }
    \endcode

    If we want the stream operators \c operator<<() and \c
    operator>>() to work on QVariant objects that store custom types,
    the custom type must provide \c operator<<() and \c operator>>()
    operators.

    \sa Q_DECLARE_METATYPE(), QVariant::setValue(), QVariant::value(), QVariant::fromValue()
*/

/* Note: these MUST be in the order of the enums */
static const struct { const char * typeName; int type; } types[] = {

    /* All Core types */
    {"void", QMetaType::Void},
    {"bool", QMetaType::Bool},
    {"int", QMetaType::Int},
    {"uint", QMetaType::UInt},
    {"qlonglong", QMetaType::LongLong},
    {"qulonglong", QMetaType::ULongLong},
    {"double", QMetaType::Double},
    {"QChar", QMetaType::QChar},
    {"QVariantMap", QMetaType::QVariantMap},
    {"QVariantList", QMetaType::QVariantList},
    {"QString", QMetaType::QString},
    {"QStringList", QMetaType::QStringList},
    {"QByteArray", QMetaType::QByteArray},
    {"QBitArray", QMetaType::QBitArray},
    {"QDate", QMetaType::QDate},
    {"QTime", QMetaType::QTime},
    {"QDateTime", QMetaType::QDateTime},
    {"QUrl", QMetaType::QUrl},
    {"QLocale", QMetaType::QLocale},
    {"QRect", QMetaType::QRect},
    {"QRectF", QMetaType::QRectF},
    {"QSize", QMetaType::QSize},
    {"QSizeF", QMetaType::QSizeF},
    {"QLine", QMetaType::QLine},
    {"QLineF", QMetaType::QLineF},
    {"QPoint", QMetaType::QPoint},
    {"QPointF", QMetaType::QPointF},
    {"QRegExp", QMetaType::QRegExp},

    /* All GUI types */
    {"QColorGroup", 63},
    {"QFont", QMetaType::QFont},
    {"QPixmap", QMetaType::QPixmap},
    {"QBrush", QMetaType::QBrush},
    {"QColor", QMetaType::QColor},
    {"QPalette", QMetaType::QPalette},
    {"QIcon", QMetaType::QIcon},
    {"QImage", QMetaType::QImage},
    {"QPolygon", QMetaType::QPolygon},
    {"QRegion", QMetaType::QRegion},
    {"QBitmap", QMetaType::QBitmap},
    {"QCursor", QMetaType::QCursor},
    {"QSizePolicy", QMetaType::QSizePolicy},
    {"QKeySequence", QMetaType::QKeySequence},
    {"QPen", QMetaType::QPen},
    {"QTextLength", QMetaType::QTextLength},
    {"QTextFormat", QMetaType::QTextFormat},
    {"QMatrix", QMetaType::QMatrix},
    {"QTransform", QMetaType::QTransform},

    /* All Metatype builtins */
    {"void*", QMetaType::VoidStar},
    {"long", QMetaType::Long},
    {"short", QMetaType::Short},
    {"char", QMetaType::Char},
    {"ulong", QMetaType::ULong},
    {"ushort", QMetaType::UShort},
    {"uchar", QMetaType::UChar},
    {"float", QMetaType::Float},
    {"QObject*", QMetaType::QObjectStar},
    {"QWidget*", QMetaType::QWidgetStar},

    /* Type aliases - order doesn't matter */
    {"unsigned long", QMetaType::ULong},
    {"unsigned int", QMetaType::UInt},
    {"unsigned short", QMetaType::UShort},
    {"unsigned char", QMetaType::UChar},
    {"qint8", QMetaType::Char},
    {"quint8", QMetaType::UChar},
    {"qint16", QMetaType::Short},
    {"quint16", QMetaType::UShort},
    {"qint32", QMetaType::Int},
    {"quint32", QMetaType::UInt},
    {"qint64", QMetaType::LongLong},
    {"quint64", QMetaType::ULongLong},
    {"QList<QVariant>", QMetaType::QVariantList},
    {"QMap<QString,QVariant>", QMetaType::QVariantMap},
    // let QMetaTypeId2 figure out the type at compile time
    {"qreal", QMetaTypeId2<qreal>::MetaType},

    {0, QMetaType::Void}
};

struct QMetaTypeGuiHelper
{
    QMetaType::Constructor constr;
    QMetaType::Destructor destr;
#ifndef QT_NO_DATASTREAM
    QMetaType::SaveOperator saveOp;
    QMetaType::LoadOperator loadOp;
#endif
};
Q_CORE_EXPORT const QMetaTypeGuiHelper *qMetaTypeGuiHelper = 0;

class QCustomTypeInfo
{
public:
    QCustomTypeInfo() : typeName(), constr(0), destr(0)
#ifndef QT_NO_DATASTREAM
    , saveOp(0), loadOp(0)
#endif
    {}

    QByteArray typeName;
    QMetaType::Constructor constr;
    QMetaType::Destructor destr;
#ifndef QT_NO_DATASTREAM
    QMetaType::SaveOperator saveOp;
    QMetaType::LoadOperator loadOp;
#endif
};

Q_DECLARE_TYPEINFO(QCustomTypeInfo, Q_MOVABLE_TYPE);
Q_GLOBAL_STATIC(QVector<QCustomTypeInfo>, customTypes)
Q_GLOBAL_STATIC(QReadWriteLock, customTypesLock)

#ifndef QT_NO_DATASTREAM
/*! \internal
*/
void QMetaType::registerStreamOperators(const char *typeName, SaveOperator saveOp,
                                        LoadOperator loadOp)
{
    int idx = type(typeName);
    if (!idx)
        return;

    QVector<QCustomTypeInfo> *ct = customTypes();
    if (!ct)
        return;
    QWriteLocker locker(customTypesLock());
    QCustomTypeInfo &inf = (*ct)[idx - User];
    inf.saveOp = saveOp;
    inf.loadOp = loadOp;
}
#endif

/*!
    Returns the type name associated with the given \a type, or 0 if no
    matching type was found. The returned pointer must not be deleted.

    \sa type(), isRegistered(), Type
*/
const char *QMetaType::typeName(int type)
{
    enum { GuiTypeCount = LastGuiType - FirstGuiType };

    if (type >= 0 && type <= LastCoreType) {
        return types[type].typeName;
    } else if (type >= FirstGuiType && type <= LastGuiType) {
        return types[type - FirstGuiType + LastCoreType + 1].typeName;
    } else if (type >= FirstCoreExtType && type <= LastCoreExtType) {
        return types[type - FirstCoreExtType + GuiTypeCount + LastCoreType + 2].typeName;
    } else if (type >= User) {
        const QVector<QCustomTypeInfo> * const ct = customTypes();
        QReadLocker locker(customTypesLock());
        return ct && ct->count() > type - User
                ? ct->at(type - User).typeName.constData()
                : static_cast<const char *>(0);
    }

    return 0;
}

/*! \internal
    Same as QMetaType::type(), but doesn't lock the mutex.
*/
static int qMetaTypeType_unlocked(const QByteArray &typeName)
{
    int i = 0;
    while (types[i].typeName && strcmp(typeName.constData(), types[i].typeName))
        ++i;
    if (!types[i].type) {
        const QVector<QCustomTypeInfo> * const ct = customTypes();
        if (!ct)
            return 0;

        for (int v = 0; v < ct->count(); ++v) {
            if (ct->at(v).typeName == typeName)
                return v + QMetaType::User;
        }
    }
    return types[i].type;
}

/*! \internal

    Registers a user type for marshalling, with \a typeName, a \a
    destructor, and a \a constructor. Returns the type's handle,
    or -1 if the type could not be registered.
 */
int QMetaType::registerType(const char *typeName, Destructor destructor,
                            Constructor constructor)
{
    QVector<QCustomTypeInfo> *ct = customTypes();
    if (!ct || !typeName || !destructor || !constructor)
        return -1;

#ifdef QT_NO_QOBJECT
    ::QByteArray normalizedTypeName = typeName;
#else
    ::QByteArray normalizedTypeName = QMetaObject::normalizedType(typeName);
#endif

    QWriteLocker locker(customTypesLock());
    static int currentIdx = User;
    int idx = qMetaTypeType_unlocked(normalizedTypeName);

    if (!idx) {
        idx = currentIdx++;
        ct->resize(ct->count() + 1);
        QCustomTypeInfo &inf = (*ct)[idx - User];
        inf.typeName = normalizedTypeName;
        inf.constr = constructor;
        inf.destr = destructor;
    }
    return idx;
}

/*!
    Returns true if the datatype with ID \a type is registered;
    otherwise returns false.

    \sa type(), typeName(), Type
*/
bool QMetaType::isRegistered(int type)
{
    if (type >= 0 && type < User) {
        // predefined type
        return true;
    }
    QReadLocker locker(customTypesLock());
    const QVector<QCustomTypeInfo> * const ct = customTypes();
    return ((type >= User) && (ct && ct->count() > type - User));
}

/*!
    Returns a handle to the type called \a typeName, or 0 if there is
    no such type.

    \sa isRegistered(), typeName(), Type
*/
int QMetaType::type(const char *typeName)
{
#ifdef QT_NO_QOBJECT
    const ::QByteArray normalizedTypeName = typeName;
#else
    const ::QByteArray normalizedTypeName = QMetaObject::normalizedType(typeName);
#endif

    QReadLocker locker(customTypesLock());
    return qMetaTypeType_unlocked(normalizedTypeName);
}

#ifndef QT_NO_DATASTREAM
/*!
    Writes the object pointed to by \a data with the ID \a type to
    the given \a stream. Returns true if the object is saved
    successfully; otherwise returns false.

    The type must have been registered with qRegisterMetaType() and
    qRegisterMetaTypeStreamOperators() beforehand.

    Normally, you should not need to call this function directly.
    Instead, use QVariant's \c operator<<(), which relies on save()
    to stream custom types.

    \sa load(), qRegisterMetaTypeStreamOperators()
*/
bool QMetaType::save(QDataStream &stream, int type, const void *data)
{
    if (!data || !isRegistered(type))
        return false;

    switch(type) {
    case QMetaType::Void:
    case QMetaType::VoidStar:
    case QMetaType::QObjectStar:
    case QMetaType::QWidgetStar:
        return false;
    case QMetaType::Long:
        stream << qlonglong(*static_cast<const long *>(data));
        break;
    case QMetaType::Int:
        stream << *static_cast<const int *>(data);
        break;
    case QMetaType::Short:
        stream << *static_cast<const short *>(data);
        break;
    case QMetaType::Char:
        // force a char to be signed
        stream << *static_cast<const signed char *>(data);
        break;
    case QMetaType::ULong:
        stream << qulonglong(*static_cast<const ulong *>(data));
        break;
    case QMetaType::UInt:
        stream << *static_cast<const uint *>(data);
        break;
    case QMetaType::LongLong:
        stream << *static_cast<const qlonglong *>(data);
        break;
    case QMetaType::ULongLong:
        stream << *static_cast<const qulonglong *>(data);
        break;
    case QMetaType::UShort:
        stream << *static_cast<const ushort *>(data);
        break;
    case QMetaType::UChar:
        stream << *static_cast<const uchar *>(data);
        break;
    case QMetaType::Bool:
        stream << qint8(*static_cast<const bool *>(data));
        break;
    case QMetaType::Float:
        stream << *static_cast<const float *>(data);
        break;
    case QMetaType::Double:
        stream << *static_cast<const double *>(data);
        break;
    case QMetaType::QChar:
        stream << *static_cast<const ::QChar *>(data);
        break;
#ifndef QT_BOOTSTRAPPED
    case QMetaType::QVariantMap:
        stream << *static_cast<const ::QVariantMap *>(data);
        break;
    case QMetaType::QVariantList:
        stream << *static_cast<const ::QVariantList *>(data);
        break;
#endif
    case QMetaType::QByteArray:
        stream << *static_cast<const ::QByteArray *>(data);
        break;
    case QMetaType::QString:
        stream << *static_cast<const ::QString *>(data);
        break;
    case QMetaType::QStringList:
        stream << *static_cast<const ::QStringList *>(data);
        break;
#ifndef QT_BOOTSTRAPPED
    case QMetaType::QBitArray:
        stream << *static_cast<const ::QBitArray *>(data);
        break;
#endif
    case QMetaType::QDate:
        stream << *static_cast<const ::QDate *>(data);
        break;
    case QMetaType::QTime:
        stream << *static_cast<const ::QTime *>(data);
        break;
    case QMetaType::QDateTime:
        stream << *static_cast<const ::QDateTime *>(data);
        break;
#ifndef QT_BOOTSTRAPPED
    case QMetaType::QUrl:
        stream << *static_cast<const ::QUrl *>(data);
        break;
#endif
    case QMetaType::QLocale:
        stream << *static_cast<const ::QLocale *>(data);
        break;
#ifndef QT_NO_GEOM_VARIANT
    case QMetaType::QRect:
        stream << *static_cast<const ::QRect *>(data);
        break;
    case QMetaType::QRectF:
        stream << *static_cast<const ::QRectF *>(data);
        break;
    case QMetaType::QSize:
        stream << *static_cast<const ::QSize *>(data);
        break;
    case QMetaType::QSizeF:
        stream << *static_cast<const ::QSizeF *>(data);
        break;
    case QMetaType::QLine:
        stream << *static_cast<const ::QLine *>(data);
        break;
    case QMetaType::QLineF:
        stream << *static_cast<const ::QLineF *>(data);
        break;
    case QMetaType::QPoint:
        stream << *static_cast<const ::QPoint *>(data);
        break;
    case QMetaType::QPointF:
        stream << *static_cast<const ::QPointF *>(data);
        break;
#endif
#ifndef QT_NO_REGEXP
    case QMetaType::QRegExp:
        stream << *static_cast<const ::QRegExp *>(data);
        break;
#endif
#ifdef QT3_SUPPORT
    case QMetaType::QColorGroup:
#endif
    case QMetaType::QFont:
    case QMetaType::QPixmap:
    case QMetaType::QBrush:
    case QMetaType::QColor:
    case QMetaType::QPalette:
    case QMetaType::QIcon:
    case QMetaType::QImage:
    case QMetaType::QPolygon:
    case QMetaType::QRegion:
    case QMetaType::QBitmap:
    case QMetaType::QCursor:
    case QMetaType::QSizePolicy:
    case QMetaType::QKeySequence:
    case QMetaType::QPen:
    case QMetaType::QTextLength:
    case QMetaType::QTextFormat:
    case QMetaType::QMatrix:
    case QMetaType::QTransform:
        if (!qMetaTypeGuiHelper)
            return false;
        qMetaTypeGuiHelper[type - FirstGuiType].saveOp(stream, data);
        break;
    default: {
        const QVector<QCustomTypeInfo> * const ct = customTypes();
        if (!ct)
            return false;

        SaveOperator saveOp = 0;
        {
            QReadLocker locker(customTypesLock());
            saveOp = ct->at(type - User).saveOp;
        }

        if (!saveOp)
            return false;
        saveOp(stream, data);
        break; }
    }

    return true;
}

/*!
    Reads the object of the specified \a type from the given \a
    stream into \a data. Returns true if the object is loaded
    successfully; otherwise returns false.

    The type must have been registered with qRegisterMetaType() and
    qRegisterMetaTypeStreamOperators() beforehand.

    Normally, you should not need to call this function directly.
    Instead, use QVariant's \c operator>>(), which relies on load()
    to stream custom types.

    \sa save(), qRegisterMetaTypeStreamOperators()
*/
bool QMetaType::load(QDataStream &stream, int type, void *data)
{
    if (!data || !isRegistered(type))
        return false;

    switch(type) {
    case QMetaType::Void:
    case QMetaType::VoidStar:
    case QMetaType::QObjectStar:
    case QMetaType::QWidgetStar:
        return false;
    case QMetaType::Long: {
        qlonglong l;
        stream >> l;
        *static_cast<long *>(data) = long(l);
        break; }
    case QMetaType::Int:
        stream >> *static_cast<int *>(data);
        break;
    case QMetaType::Short:
        stream >> *static_cast<short *>(data);
        break;
    case QMetaType::Char:
        // force a char to be signed
        stream >> *static_cast<signed char *>(data);
        break;
    case QMetaType::ULong: {
        qulonglong ul;
        stream >> ul;
        *static_cast<ulong *>(data) = ulong(ul);
        break; }
    case QMetaType::UInt:
        stream >> *static_cast<uint *>(data);
        break;
    case QMetaType::LongLong:
        stream >> *static_cast<qlonglong *>(data);
        break;
    case QMetaType::ULongLong:
        stream >> *static_cast<qulonglong *>(data);
        break;
    case QMetaType::UShort:
        stream >> *static_cast<ushort *>(data);
        break;
    case QMetaType::UChar:
        stream >> *static_cast<uchar *>(data);
        break;
    case QMetaType::Bool: {
        qint8 b;
        stream >> b;
        *static_cast<bool *>(data) = b;
        break; }
    case QMetaType::Float:
        stream >> *static_cast<float *>(data);
        break;
    case QMetaType::Double:
        stream >> *static_cast<double *>(data);
        break;
    case QMetaType::QChar:
        stream >> *static_cast< ::QChar *>(data);
        break;
#ifndef QT_BOOTSTRAPPED
    case QMetaType::QVariantMap:
        stream >> *static_cast< ::QVariantMap *>(data);
        break;
    case QMetaType::QVariantList:
        stream >> *static_cast< ::QVariantList *>(data);
        break;
#endif
    case QMetaType::QByteArray:
        stream >> *static_cast< ::QByteArray *>(data);
        break;
    case QMetaType::QString:
        stream >> *static_cast< ::QString *>(data);
        break;
    case QMetaType::QStringList:
        stream >> *static_cast< ::QStringList *>(data);
        break;
#ifndef QT_BOOTSTRAPPED
    case QMetaType::QBitArray:
        stream >> *static_cast< ::QBitArray *>(data);
        break;
#endif
    case QMetaType::QDate:
        stream >> *static_cast< ::QDate *>(data);
        break;
    case QMetaType::QTime:
        stream >> *static_cast< ::QTime *>(data);
        break;
    case QMetaType::QDateTime:
        stream >> *static_cast< ::QDateTime *>(data);
        break;
#ifndef QT_BOOTSTRAPPED
    case QMetaType::QUrl:
        stream >> *static_cast< ::QUrl *>(data);
        break;
#endif
    case QMetaType::QLocale:
        stream >> *static_cast< ::QLocale *>(data);
        break;
#ifndef QT_NO_GEOM_VARIANT
    case QMetaType::QRect:
        stream >> *static_cast< ::QRect *>(data);
        break;
    case QMetaType::QRectF:
        stream >> *static_cast< ::QRectF *>(data);
        break;
    case QMetaType::QSize:
        stream >> *static_cast< ::QSize *>(data);
        break;
    case QMetaType::QSizeF:
        stream >> *static_cast< ::QSizeF *>(data);
        break;
    case QMetaType::QLine:
        stream >> *static_cast< ::QLine *>(data);
        break;
    case QMetaType::QLineF:
        stream >> *static_cast< ::QLineF *>(data);
        break;
    case QMetaType::QPoint:
        stream >> *static_cast< ::QPoint *>(data);
        break;
    case QMetaType::QPointF:
        stream >> *static_cast< ::QPointF *>(data);
        break;
#endif
#ifndef QT_NO_REGEXP
    case QMetaType::QRegExp:
        stream >> *static_cast< ::QRegExp *>(data);
        break;
#endif
#ifdef QT3_SUPPORT
    case QMetaType::QColorGroup:
#endif
    case QMetaType::QFont:
    case QMetaType::QPixmap:
    case QMetaType::QBrush:
    case QMetaType::QColor:
    case QMetaType::QPalette:
    case QMetaType::QIcon:
    case QMetaType::QImage:
    case QMetaType::QPolygon:
    case QMetaType::QRegion:
    case QMetaType::QBitmap:
    case QMetaType::QCursor:
    case QMetaType::QSizePolicy:
    case QMetaType::QKeySequence:
    case QMetaType::QPen:
    case QMetaType::QTextLength:
    case QMetaType::QTextFormat:
    case QMetaType::QMatrix:
    case QMetaType::QTransform:
        if (!qMetaTypeGuiHelper)
            return false;
        qMetaTypeGuiHelper[type - FirstGuiType].loadOp(stream, data);
        break;
    default: {
        const QVector<QCustomTypeInfo> * const ct = customTypes();
        if (!ct)
            return false;

        LoadOperator loadOp = 0;
        {
            QReadLocker locker(customTypesLock());
            loadOp = ct->at(type - User).loadOp;
        }

        if (!loadOp)
            return false;
        loadOp(stream, data);
        break; }
    }
    return true;
}
#endif

/*!
    Returns a copy of \a copy, assuming it is of type \a type. If \a
    copy is zero, creates a default type.

    \sa destroy(), isRegistered(), Type
*/
void *QMetaType::construct(int type, const void *copy)
{
    if (copy) {
        switch(type) {
        case QMetaType::VoidStar:
        case QMetaType::QObjectStar:
        case QMetaType::QWidgetStar:
            return new void *(*static_cast<void* const *>(copy));
        case QMetaType::Long:
            return new long(*static_cast<const long*>(copy));
        case QMetaType::Int:
            return new int(*static_cast<const int*>(copy));
        case QMetaType::Short:
            return new short(*static_cast<const short*>(copy));
        case QMetaType::Char:
            return new char(*static_cast<const char*>(copy));
        case QMetaType::ULong:
            return new ulong(*static_cast<const ulong*>(copy));
        case QMetaType::UInt:
            return new uint(*static_cast<const uint*>(copy));
        case QMetaType::LongLong:
            return new qlonglong(*static_cast<const qlonglong*>(copy));
        case QMetaType::ULongLong:
            return new qulonglong(*static_cast<const qulonglong*>(copy));
        case QMetaType::UShort:
            return new ushort(*static_cast<const ushort*>(copy));
        case QMetaType::UChar:
            return new uchar(*static_cast<const uchar*>(copy));
        case QMetaType::Bool:
            return new bool(*static_cast<const bool*>(copy));
        case QMetaType::Float:
            return new float(*static_cast<const float*>(copy));
        case QMetaType::Double:
            return new double(*static_cast<const double*>(copy));
        case QMetaType::QChar:
            return new ::QChar(*static_cast<const ::QChar*>(copy));
#ifndef QT_BOOTSTRAPPED
        case QMetaType::QVariantMap:
            return new ::QVariantMap(*static_cast<const ::QVariantMap*>(copy));
        case QMetaType::QVariantList:
            return new ::QVariantList(*static_cast<const ::QVariantList*>(copy));
#endif
        case QMetaType::QByteArray:
            return new ::QByteArray(*static_cast<const ::QByteArray*>(copy));
        case QMetaType::QString:
            return new ::QString(*static_cast<const ::QString*>(copy));
        case QMetaType::QStringList:
            return new ::QStringList(*static_cast<const ::QStringList *>(copy));
#ifndef QT_BOOTSTRAPPED
        case QMetaType::QBitArray:
            return new ::QBitArray(*static_cast<const ::QBitArray *>(copy));
#endif
        case QMetaType::QDate:
            return new ::QDate(*static_cast<const ::QDate *>(copy));
        case QMetaType::QTime:
            return new ::QTime(*static_cast<const ::QTime *>(copy));
        case QMetaType::QDateTime:
            return new ::QDateTime(*static_cast<const ::QDateTime *>(copy));
#ifndef QT_BOOTSTRAPPED
        case QMetaType::QUrl:
            return new ::QUrl(*static_cast<const ::QUrl *>(copy));
#endif
        case QMetaType::QLocale:
            return new ::QLocale(*static_cast<const ::QLocale *>(copy));
#ifndef QT_NO_GEOM_VARIANT
        case QMetaType::QRect:
            return new ::QRect(*static_cast<const ::QRect *>(copy));
        case QMetaType::QRectF:
            return new ::QRectF(*static_cast<const ::QRectF *>(copy));
        case QMetaType::QSize:
            return new ::QSize(*static_cast<const ::QSize *>(copy));
        case QMetaType::QSizeF:
            return new ::QSizeF(*static_cast<const ::QSizeF *>(copy));
        case QMetaType::QLine:
            return new ::QLine(*static_cast<const ::QLine *>(copy));
        case QMetaType::QLineF:
            return new ::QLineF(*static_cast<const ::QLineF *>(copy));
        case QMetaType::QPoint:
            return new ::QPoint(*static_cast<const ::QPoint *>(copy));
        case QMetaType::QPointF:
            return new ::QPointF(*static_cast<const ::QPointF *>(copy));
#endif
#ifndef QT_NO_REGEXP
        case QMetaType::QRegExp:
            return new ::QRegExp(*static_cast<const ::QRegExp *>(copy));
#endif
        case QMetaType::Void:
            return 0;
        default:
            ;
        }
    } else {
        switch(type) {
        case QMetaType::VoidStar:
        case QMetaType::QObjectStar:
        case QMetaType::QWidgetStar:
            return new void *;
        case QMetaType::Long:
            return new long;
        case QMetaType::Int:
            return new int;
        case QMetaType::Short:
            return new short;
        case QMetaType::Char:
            return new char;
        case QMetaType::ULong:
            return new ulong;
        case QMetaType::UInt:
            return new uint;
        case QMetaType::LongLong:
            return new qlonglong;
        case QMetaType::ULongLong:
            return new qulonglong;
        case QMetaType::UShort:
            return new ushort;
        case QMetaType::UChar:
            return new uchar;
        case QMetaType::Bool:
            return new bool;
        case QMetaType::Float:
            return new float;
        case QMetaType::Double:
            return new double;
        case QMetaType::QChar:
            return new ::QChar;
#ifndef QT_BOOTSTRAPPED
        case QMetaType::QVariantMap:
            return new ::QVariantMap;
        case QMetaType::QVariantList:
            return new ::QVariantList;
#endif
        case QMetaType::QByteArray:
            return new ::QByteArray;
        case QMetaType::QString:
            return new ::QString;
        case QMetaType::QStringList:
            return new ::QStringList;
#ifndef QT_BOOTSTRAPPED
        case QMetaType::QBitArray:
            return new ::QBitArray;
#endif
        case QMetaType::QDate:
            return new ::QDate;
        case QMetaType::QTime:
            return new ::QTime;
        case QMetaType::QDateTime:
            return new ::QDateTime;
#ifndef QT_BOOTSTRAPPED
        case QMetaType::QUrl:
            return new ::QUrl;
#endif
        case QMetaType::QLocale:
            return new ::QLocale;
#ifndef QT_NO_GEOM_VARIANT
        case QMetaType::QRect:
            return new ::QRect;
        case QMetaType::QRectF:
            return new ::QRectF;
        case QMetaType::QSize:
            return new ::QSize;
        case QMetaType::QSizeF:
            return new ::QSizeF;
        case QMetaType::QLine:
            return new ::QLine;
        case QMetaType::QLineF:
            return new ::QLineF;
        case QMetaType::QPoint:
            return new ::QPoint;
        case QMetaType::QPointF:
            return new ::QPointF;
#endif
#ifndef QT_NO_REGEXP
        case QMetaType::QRegExp:
            return new ::QRegExp;
#endif
        case QMetaType::Void:
            return 0;
        default:
            ;
        }
    }

    Constructor constr = 0;
    if (type >= FirstGuiType && type <= LastGuiType) {
        if (!qMetaTypeGuiHelper)
            return 0;
        constr = qMetaTypeGuiHelper[type - FirstGuiType].constr;
    } else {
        const QVector<QCustomTypeInfo> * const ct = customTypes();
        QReadLocker locker(customTypesLock());
        if (type < User || !ct || ct->count() <= type - User)
            return 0;

        constr = ct->at(type - User).constr;
    }

    return constr(copy);
}

/*!
    Destroys the \a data, assuming it is of the \a type given.

    \sa construct(), isRegistered(), Type
*/
void QMetaType::destroy(int type, void *data)
{
    if (!data)
        return;
    switch(type) {
    case QMetaType::VoidStar:
    case QMetaType::QObjectStar:
    case QMetaType::QWidgetStar:
        delete static_cast<void**>(data);
        break;
    case QMetaType::Long:
        delete static_cast<long*>(data);
        break;
    case QMetaType::Int:
        delete static_cast<int*>(data);
        break;
    case QMetaType::Short:
        delete static_cast<short*>(data);
        break;
    case QMetaType::Char:
        delete static_cast<char*>(data);
        break;
    case QMetaType::ULong:
        delete static_cast<ulong*>(data);
        break;
    case QMetaType::LongLong:
        delete static_cast<qlonglong*>(data);
        break;
    case QMetaType::ULongLong:
        delete static_cast<qulonglong*>(data);
        break;
    case QMetaType::UInt:
        delete static_cast<uint*>(data);
        break;
    case QMetaType::UShort:
        delete static_cast<ushort*>(data);
        break;
    case QMetaType::UChar:
        delete static_cast<uchar*>(data);
        break;
    case QMetaType::Bool:
        delete static_cast<bool*>(data);
        break;
    case QMetaType::Float:
        delete static_cast<float*>(data);
        break;
    case QMetaType::Double:
        delete static_cast<double*>(data);
        break;
    case QMetaType::QChar:
        delete static_cast< ::QChar*>(data);
        break;
#ifndef QT_BOOTSTRAPPED
    case QMetaType::QVariantMap:
        delete static_cast< ::QVariantMap*>(data);
        break;
    case QMetaType::QVariantList:
        delete static_cast< ::QVariantList*>(data);
        break;
#endif
    case QMetaType::QByteArray:
        delete static_cast< ::QByteArray*>(data);
        break;
    case QMetaType::QString:
        delete static_cast< ::QString*>(data);
        break;
    case QMetaType::QStringList:
        delete static_cast< ::QStringList*>(data);
        break;
#ifndef QT_BOOTSTRAPPED
    case QMetaType::QBitArray:
        delete static_cast< ::QBitArray*>(data);
        break;
#endif
    case QMetaType::QDate:
        delete static_cast< ::QDate*>(data);
        break;
    case QMetaType::QTime:
        delete static_cast< ::QTime*>(data);
        break;
    case QMetaType::QDateTime:
        delete static_cast< ::QDateTime*>(data);
        break;
#ifndef QT_BOOTSTRAPPED
    case QMetaType::QUrl:
        delete static_cast< ::QUrl*>(data);
#endif
        break;
    case QMetaType::QLocale:
        delete static_cast< ::QLocale*>(data);
        break;
#ifndef QT_NO_GEOM_VARIANT
    case QMetaType::QRect:
        delete static_cast< ::QRect*>(data);
        break;
    case QMetaType::QRectF:
        delete static_cast< ::QRectF*>(data);
        break;
    case QMetaType::QSize:
        delete static_cast< ::QSize*>(data);
        break;
    case QMetaType::QSizeF:
        delete static_cast< ::QSizeF*>(data);
        break;
    case QMetaType::QLine:
        delete static_cast< ::QLine*>(data);
        break;
    case QMetaType::QLineF:
        delete static_cast< ::QLineF*>(data);
        break;
    case QMetaType::QPoint:
        delete static_cast< ::QPoint*>(data);
        break;
    case QMetaType::QPointF:
        delete static_cast< ::QPointF*>(data);
        break;
#endif
#ifndef QT_NO_REGEXP
    case QMetaType::QRegExp:
        delete static_cast< ::QRegExp*>(data);
        break;
#endif
    case QMetaType::Void:
        break;
    default: {
        const QVector<QCustomTypeInfo> * const ct = customTypes();
        Destructor destr = 0;
        if (type >= FirstGuiType && type <= LastGuiType) {
            Q_ASSERT(qMetaTypeGuiHelper);

            if (!qMetaTypeGuiHelper)
                return;
            destr = qMetaTypeGuiHelper[type - FirstGuiType].destr;
        } else {
            QReadLocker locker(customTypesLock());
            if (type < User || !ct || ct->count() <= type - User)
                break;
            destr = ct->at(type - User).destr;
        }
        destr(data);
        break; }
    }
}

/*!
    \fn int qRegisterMetaType(const char *typeName)
    \relates QMetaType
    \threadsafe

    Registers the type name \a typeName to the type \c{T}. Returns
    the internal ID used by QMetaType. Any class or struct that has a
    public constructor, a public copy constructor, and a public
    destructor can be registered.

    After a type has been registered, you can create and destroy
    objects of that type dynamically at run-time.

    This example registers the class \c{MyClass}:

    \code
        qRegisterMetaType<MyClass>("MyClass");
    \endcode

    \sa qRegisterMetaTypeStreamOperators(), QMetaType::isRegistered(),
        Q_DECLARE_METATYPE()
*/

/*!
    \fn int qRegisterMetaTypeStreamOperators(const char *typeName)
    \relates QMetaType
    \threadsafe

    Registers the stream operators for the type \c{T} called \a
    typeName.

    Afterward, the type can be streamed using QMetaType::load() and
    QMetaType::save(). These functions are used when streaming a
    QVariant.

    \code
        qRegisterMetaTypeStreamOperators<MyClass>("MyClass");
    \endcode

    The stream operators should have the following signatures:

    \code
        QDataStream &operator<<(QDataStream &out, const MyClass &myObj);
        QDataStream &operator>>(QDataStream &in, MyClass &myObj);
    \endcode

    \sa qRegisterMetaType(), QMetaType::isRegistered(), Q_DECLARE_METATYPE()
*/

/*! \typedef QMetaType::Destructor
    \internal
*/
/*! \typedef QMetaType::Constructor
    \internal
*/
/*! \typedef QMetaType::SaveOperator
    \internal
*/
/*! \typedef QMetaType::LoadOperator
    \internal
*/

/*!
    \fn int qRegisterMetaType()
    \relates QMetaType
    \threadsafe
    \since 4.2

    Call this function to register the type \c T. \c T must be declared with
    Q_DECLARE_METATYPE(). Returns the meta type Id.

    Example:

    \code
        int id = qRegisterMetaType<MyStruct>();
    \endcode

    \bold{Note:} To use the type \c T in QVariant, using Q_DECLARE_METATYPE() is
    sufficient. To use the type \c T in queued signal and slot connections,
    \c{qRegisterMetaType<T>()} must be called before the first connection
    is established.

    \sa Q_DECLARE_METATYPE()
 */

/*! \fn int qMetaTypeId()
    \relates QMetaType
    \threadsafe
    \since 4.1

    Returns the meta type id of type \c T at compile time. If the
    type was not declared with Q_DECLARE_METATYPE(), compilation will
    fail.

    Typical usage:

    \code
        int id = qMetaTypeId<QString>();    // id is now QMetaType::QString
        id = qMetaTypeId<MyStruct>();       // compile error if MyStruct not declared
    \endcode

    QMetaType::type() returns the same ID as qMetaTypeId(), but does
    a lookup at runtime based on the name of the type.
    QMetaType::type() is a bit slower, but compilation succeeds if a
    type is not registered.

    \sa Q_DECLARE_METATYPE(), QMetaType::type()
*/
