/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxarray.h,v 1.4 2007/07/06 20:35:03 jfinnecy Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
 * 
 * The contents of this file, and the files included with this file,
 * are subject to the current version of the RealNetworks Public
 * Source License (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the current version of the RealNetworks Community
 * Source License (the "RCSL") available at
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL
 * will apply. You may also obtain the license terms directly from
 * RealNetworks.  You may not use this file except in compliance with
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for
 * the rights, obligations and limitations governing use of the
 * contents of the file.
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
 * 
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the
 * portions it created.
 * 
 * This file, and the files included with this file, is distributed
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET
 * ENJOYMENT OR NON-INFRINGEMENT.
 * 
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 * 
 * Contributor(s):
 * 
 * ***** END LICENSE BLOCK ***** */

#ifndef HLXCARRAY_H
#define HLXCARRAY_H

#include "hxassert.h"
#include "carray.h"

template <class T>
class HLXArray
{
public:
    HLXArray();
    ~HLXArray();
    
    bool IsEmpty() const;
    int GetSize() const;
    int GetUpperBound() const;

    void SetSize(int size, int growSize);
    void FreeExtra();

    void RemoveAll();

    bool IsSet(int i) const;

    const T& GetAt(int i) const;
    T& ElementAt(int i);

    void SetAt(int i, const T& value);
    void SetAtGrow(int i, const T& value);

    T& operator[](int i);

    void InsertAt(int i, const T& value, int num = 1);
    void InsertAt(int i, const HLXArray<T>& array);
    void RemoveAt(int i);

private:
    CHXPtrArray m_rep;
};

template <class T>
inline
HLXArray<T>::HLXArray()
{}

template <class T>
inline
HLXArray<T>::~HLXArray()
{
    RemoveAll();
}
  
template <class T>
inline  
bool HLXArray<T>::IsEmpty() const
{
    return m_rep.IsEmpty();
}

template <class T>
inline
int HLXArray<T>::GetSize() const
{
    return m_rep.GetSize();
}

template <class T>
inline
int HLXArray<T>::GetUpperBound() const
{
    return m_rep.GetUpperBound();
}

template <class T>
inline
void HLXArray<T>::SetSize(int size, int growSize)
{
    m_rep.SetSize(size, growSize);
}

template <class T>
inline
void HLXArray<T>::FreeExtra()
{
    m_rep.FreeExtra();
}

template <class T>
inline
void HLXArray<T>::RemoveAll()
{
    for (int i = 0; i < m_rep.GetSize(); i++)
    {
	T* pTmp = (T*)m_rep[i];

	delete pTmp;
    }
    m_rep.RemoveAll();
}

template <class T>
inline
bool HLXArray<T>::IsSet(int i) const
{
    bool ret = false;

    if ((i < m_rep.GetSize()) && m_rep.GetAt(i))
	ret = true;

    return ret;
}

template <class T>
inline
const T& HLXArray<T>::GetAt(int i) const
{
    T* pTmp = (T*)m_rep.GetAt(i); 
    
    HX_ASSERT(pTmp);

    return *pTmp;
}

template <class T>
inline
T& HLXArray<T>::ElementAt(int i)
{
    T* pTmp = (T*)m_rep.ElementAt(i); 
    
    HX_ASSERT(pTmp);

    return *pTmp;
}

template <class T>
inline
void HLXArray<T>::SetAt(int i, const T& value)
{
    T* pTmp = 0;

    if (i < m_rep.GetSize())
	pTmp = m_rep.GetAt(i);

    if (!pTmp)
    {
	pTmp = new T(value);
	m_rep.SetAt(i, pTmp);
    }
    else
	*pTmp = value;
}

template <class T>
inline
void HLXArray<T>::SetAtGrow(int i, const T& value)
{
    T* pTmp = 0;

    if (i < m_rep.GetSize())
	pTmp = (T*)m_rep.GetAt(i);

    if (!pTmp)
    {
	pTmp = new T(value);
	m_rep.SetAtGrow(i, pTmp);
    }
    else
	*pTmp = value;
}

template <class T>
inline
T& HLXArray<T>::operator[](int i)
{
    T* pTmp = (T*)m_rep[i]; 
    
    HX_ASSERT(pTmp);

    return *pTmp;
}

template <class T>
inline
void HLXArray<T>::InsertAt(int i, const T& value, int num)
{
    for (int j = 0; j < num; j++)
	m_rep.InsertAt(i + j, new T(value), 1);
}

template <class T>
inline
void HLXArray<T>::InsertAt(int i, const HLXArray<T>& array)
{
    for (int j = 0; j < array.GetSize(); j++)
	m_rep.Insert(i + j, new T(array[j]), 1);
}
 
template <class T>
inline
void HLXArray<T>::RemoveAt(int i)
{
    delete (T*)m_rep.GetAt(i);

    m_rep.RemoveAt(i);
}

#endif // HXLCARRAY_H
