/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxlist.h,v 1.3 2004/07/09 18:21:31 hubbe Exp $
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
 * terms of the GNU General Public License Version 2 or later (the
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

#ifndef HELIX_LIST_H
#define HELIX_LIST_H

#include "hxslist.h"

template<class T>
class HLXList
{
public:
    HLXList();
    ~HLXList();

    int GetCount() const;
    bool IsEmpty() const;
    
    void AddHead(const T& value);
    void AddTail(const T& value);

    void RemoveHead();
    void RemoveTail();
    void RemoveAll();

    bool Find(const T& value);

private:
    CHXSimpleList m_rep;
};

template<class T>
inline
HLXList<T>::HLXList()
{}

template<class T>
inline
HLXList<T>::~HLXList()
{
    RemoveAll();
}

template<class T>
inline
int HLXList<T>::GetCount() const
{
    return m_rep.GetCount();
}

template<class T>
inline
bool HLXList<T>::IsEmpty() const
{
    return m_rep.IsEmpty();
}

template<class T>
inline
void HLXList<T>::AddHead(const T& value)
{
    m_rep.AddHead(new T(value));
}

template<class T>
inline
void HLXList<T>::AddTail(const T& value)
{
    m_rep.AddTail(new T(value));
}

template<class T>
inline
void HLXList<T>::RemoveHead()
{
    delete (T*)m_rep.RemoveHead();
}

template<class T>
inline
void HLXList<T>::RemoveTail()
{
    delete (T*)m_rep.RemoveTail();
}

template<class T>
inline
void HLXList<T>::RemoveAll()
{
    LISTPOSITION pos = m_rep.GetHeadPosition();

    while(pos)
	delete (T*)m_rep.GetNext(pos);;

    m_rep.RemoveAll();
}

template<class T>
inline
bool HLXList<T>::Find(const T& value)
{
    bool ret = false;

    LISTPOSITION pos = m_rep.GetHeadPosition();

    while(!ret && pos)
    {
	if (*((T*)m_rep.GetNext(pos)) == value)
	    ret = true;
    }
    
    return ret;
}
#endif // HELIX_LIST_H
