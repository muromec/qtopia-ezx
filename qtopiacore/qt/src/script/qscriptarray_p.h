/****************************************************************************
**
** Copyright (C) 1992-2008 Trolltech ASA. All rights reserved.
**
** This file is part of the QtScript module of the Qt Toolkit.
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

#ifndef QSCRIPTARRAY_P_H
#define QSCRIPTARRAY_P_H

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

#include <QtCore/QMap>

#ifndef QT_NO_SCRIPT

#include <QtCore/QVector>

#include "qscriptvalueimplfwd_p.h"
#include "qscriptenginefwd_p.h"

namespace QScript {

class Array // ### private
{
public:
    inline Array();
    inline Array(const Array &other);
    inline ~Array();

    inline Array &operator = (const Array &other);

    inline bool isEmpty() const;
    inline uint size() const;
    inline uint count() const;
    inline QScriptValueImpl at(uint index) const;
    inline void assign(uint index, const QScriptValueImpl &v);
    inline void clear();
    inline void mark(int generation);
    inline void resize(uint size);
    inline void concat(const Array &other);
    inline QScriptValueImpl pop();
    inline void sort(const QScriptValueImpl &comparefn);
    inline void splice(qsreal start, qsreal deleteCount,
                       const QVector<QScriptValueImpl> &items,
                       Array &other);

private:
    enum Mode {
        VectorMode,
        MapMode
    };

    Mode m_mode;
    int m_instances;

    union {
        QMap<uint, QScriptValueImpl> *to_map;
        QVector<QScriptValueImpl> *to_vector;
    };
};

class ArrayElementLessThan
{
public:
    inline ArrayElementLessThan(const QScriptValueImpl &comparefn)
        : m_comparefn(comparefn) {}

    inline bool operator()(const QScriptValueImpl &v1, const QScriptValueImpl &v2) const
    {
        if (v1.isUndefined())
            return false;
        if (v2.isUndefined())
            return true;
        if (!m_comparefn.isUndefined()) {
            ArrayElementLessThan *that = const_cast<ArrayElementLessThan*>(this);
            QScriptValueImpl result = that->m_comparefn.call(QScriptValueImpl(),
                                                             QScriptValueImplList() << v1 << v2);
            return result.toNumber() <= 0;
        }
        return v1.toString() < v2.toString();
    }

private:
    QScriptValueImpl m_comparefn;
};

} // namespace QScript

inline QScript::Array::Array():
    m_mode(VectorMode),
    m_instances(0)
{
    to_vector = new QVector<QScriptValueImpl>();
}

inline QScript::Array::Array(const Array &other):
    m_mode(other.m_mode),
    m_instances(other.m_instances)
{
    if (m_mode == VectorMode)
        to_vector = new QVector<QScriptValueImpl> (*other.to_vector);
    else
        to_map = new QMap<uint, QScriptValueImpl> (*other.to_map);
}

inline QScript::Array::~Array()
{
    if (m_mode == VectorMode)
        delete to_vector;
    else
        delete to_map;
}

inline QScript::Array &QScript::Array::operator = (const Array &other)
{
    m_instances = other.m_instances;
    if (m_mode != other.m_mode) {
        if (m_mode == VectorMode)
            delete to_vector;
        else
            delete to_map;
        m_mode = other.m_mode;

        if (m_mode == VectorMode)
            to_vector = new QVector<QScriptValueImpl> (*other.to_vector);
        else
            to_map = new QMap<uint, QScriptValueImpl> (*other.to_map);
    }

    if (m_mode == VectorMode)
        *to_vector = *other.to_vector;
    else
        *to_map = *other.to_map;

    return *this;
}

inline bool QScript::Array::isEmpty() const
{
    if (m_mode == VectorMode)
        return to_vector->isEmpty();

    return to_map->isEmpty();
}

inline uint QScript::Array::size() const
{
    if (m_mode == VectorMode)
        return to_vector->size();

    if (to_map->isEmpty())
        return 0;

    return (--to_map->end()).key();
}

inline uint QScript::Array::count() const
{
    return size();
}

inline QScriptValueImpl QScript::Array::at(uint index) const
{
    QScriptValueImpl v;

    if (m_mode == VectorMode) {
        if (index < uint(to_vector->size()))
            v = to_vector->at(index);
        else
            v.invalidate();
    }

    else
        v = to_map->value(index, QScriptValueImpl());

    return v;
}

inline void QScript::Array::assign(uint index, const QScriptValueImpl &v)
{
    if (index >= size()) {
        resize(index + 1);
        if (v.isValid()) {
            QScriptEnginePrivate *eng = QScriptEnginePrivate::get(v.engine());
            eng->adjustBytesAllocated(sizeof(QScriptValueImpl) * (size() - index));
        }
    }

    if (v.isValid() && (v.isObject() || v.isString()))
        ++m_instances;

    if (m_mode == VectorMode)
        to_vector->replace(index, v);

    else
        to_map->insert(index, v);
}

inline void QScript::Array::clear()
{
    m_instances = 0;

    if (m_mode == VectorMode)
        to_vector->clear();

    else
        to_map->clear();
}

inline void QScript::Array::mark(int generation)
{
    if (! m_instances)
        return;

    if (m_mode == VectorMode) {
        for (int i = 0; i < to_vector->size(); ++i)
            to_vector->at(i).mark(generation);
    } else {
        QMap<uint, QScriptValueImpl>::const_iterator it = to_map->constBegin();
        for (; it != to_map->constEnd(); ++it)
            it.value().mark(generation);
    }
}

inline void QScript::Array::resize(uint s)
{
    const uint N = 10 * 1024;

    if (m_mode == VectorMode) {
        if (s < N) {
            int oldSize = to_vector->size();
            to_vector->resize (s);
            if (oldSize < to_vector->size()) {
                for (int i = oldSize; i < to_vector->size(); ++i)
                    (*to_vector)[i].invalidate();
            }
        }

        else {
            QMap<uint, QScriptValueImpl> *m = new QMap<uint, QScriptValueImpl>();
            for (uint i = 0; i < uint(to_vector->size()); ++i)
                m->insert(i, to_vector->at(i));
            m->insert(s, QScriptValueImpl());
            delete to_vector;
            to_map = m;
            m_mode = MapMode;
        }
    }

    else {
        if (s < N) {
            QVector<QScriptValueImpl> *v = new QVector<QScriptValueImpl> ();
            v->fill(QScriptValueImpl(), s);
            QMap<uint, QScriptValueImpl>::const_iterator it = to_map->constBegin();
            uint i = 0;
            for (; i < s && it != to_map->constEnd(); ++it, ++i)
                (*v) [i] = it.value();
            delete to_map;
            to_vector = v;
            m_mode = VectorMode;
            return;
        }

        if (! to_map->isEmpty()) {
            QMap<uint, QScriptValueImpl>::iterator it = to_map->insert(s, QScriptValueImpl());
            for (++it; it != to_map->end(); )
                it = to_map->erase(it);
            to_map->insert(s, QScriptValueImpl()); // ### hmm
        }
    }
}

inline void QScript::Array::concat(const QScript::Array &other)
{
    int k = size();
    resize (k + other.size());
    QScriptValueImpl def;
    def.invalidate();
    for (uint i = 0; i < other.size(); ++i) {
        QScriptValueImpl v = other.at(i);
        if (! v.isValid())
            continue;

        assign(k + i, v);
    }
}

inline QScriptValueImpl QScript::Array::pop()
{
    if (isEmpty())
        return QScriptValueImpl();

    QScriptValueImpl v;

    if (m_mode == VectorMode)
        v = to_vector->last();

    else
        v = *--to_map->end();

    resize(size() - 1);

    return v;
}

inline void QScript::Array::sort(const QScriptValueImpl &comparefn)
{
    ArrayElementLessThan lessThan(comparefn);
    if (m_mode == VectorMode) {
        qSort(to_vector->begin(), to_vector->end(), lessThan);
    } else {
        QList<uint> keys = to_map->keys();
        QList<QScriptValueImpl> values = to_map->values();
        qSort(values.begin(), values.end(), lessThan);
        const uint len = size();
        for (uint i = 0; i < len; ++i)
            to_map->insert(keys.at(i), values.at(i));
    }
}

inline void QScript::Array::splice(qsreal start, qsreal deleteCount,
                                   const QVector<QScriptValueImpl> &items,
                                   Array &other)
{
    const qsreal len = size();
    if (start < 0)
        start = qMax(len + start, qsreal(0));
    else if (start > len)
        start = len;
    deleteCount = qMax(qMin(deleteCount, len), qsreal(0));

    const uint st = uint(start);
    const uint dc = uint(deleteCount);
    other.resize(dc);

    if (m_mode == VectorMode) {

        for (uint i = 0; i < dc; ++i) {
            QScriptValueImpl v = to_vector->at(st + i);
            other.assign(i, v);
            if (i < uint(items.size()))
                to_vector->replace(st + i, items.at(i));
        }
        to_vector->remove(st + items.size(), dc - items.size());

    } else {

        for (uint i = 0; i < dc; ++i) {
            QScriptValueImpl v = to_map->value(st + i, QScriptValueImpl());
            other.assign(i, v);
            if (i < uint(items.size()))
                to_map->insert(st + i, items.at(i));
        }

        uint del = dc - items.size();
        if (del != 0) {
            for (uint j = st + items.size(); j < uint(len); ++j) {
                if (to_map->contains(j)) {
                    QScriptValueImpl v = to_map->take(j);
                    to_map->insert(j - del, v);
                }
            }
            resize(uint(len) - del);
        }

    }
}

#endif // QT_NO_SCRIPT
#endif // QSCRIPTARRAY_P_H

