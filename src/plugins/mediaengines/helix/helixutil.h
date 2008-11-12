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

#ifndef HELIX_UTIL_H
#define HELIX_UTIL_H

#include <QtCore>

#include <config.h>
#include <hxcom.h>

// Return path to Helix libraries
QString helix_library_path();

class GenericContext : public IUnknown
{
public:
    GenericContext( const QList<IUnknown*>& classes = QList<IUnknown*>() );
    virtual ~GenericContext();

    // IUnknown
    STDMETHOD(QueryInterface) (THIS_
        REFIID ID,
        void **object);
    STDMETHOD_(UINT32, AddRef) (THIS);
    STDMETHOD_(UINT32, Release) (THIS);

private:
    INT32 m_refCount;

    QList<IUnknown*> m_classes;
};

#endif // HELIX_UTIL_H
