/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: kv_pair_inl.h,v 1.4 2007/07/06 20:35:03 jfinnecy Exp $
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

#ifndef KV_PAIR_I
#define KV_PAIR_I

#include "./class_ops.h"

template <class K, class V>
inline
KeyValuePair<K, V>::KeyValuePair() :
    m_key(ClassOps<K>().Create()),
    m_value(ClassOps<V>().Create())
{}

template <class K, class V>
inline
KeyValuePair<K, V>::~KeyValuePair()
{
    ClassOps<K>().Destroy(m_key);
    ClassOps<V>().Destroy(m_value);
}

template <class K, class V>
inline
KeyValuePair<K, V>::KeyValuePair(const KeyValuePair<K,V>& rhs) :
    m_key(ClassOps<K>().Copy(rhs.Key())),
    m_value(ClassOps<V>().Copy(rhs.Value()))
{}

template <class K, class V>
inline
KeyValuePair<K,V>& KeyValuePair<K, V>::operator=(const KeyValuePair<K,V>& rhs)
{
    if (&rhs != this)
    {
        ClassOps<K>().Destroy(m_key);
        ClassOps<V>().Destroy(m_value);

        m_key = ClassOps<K>().Copy(rhs.Key());
        m_value = ClassOps<V>().Copy(rhs.Value());
    }

    return *this;
}

template <class K, class V>
inline
const K&  KeyValuePair<K, V>::Key() const
{
    return m_key;
}

template <class K, class V>
inline
const V& KeyValuePair<K, V>::Value() const
{
    return m_value;
}

template <class K, class V>
inline
void KeyValuePair<K, V>::Print() const
{
    char* pKeyStr = ClassOps<K>().Print(m_key);
    char* pValueStr = ClassOps<V>().Print(m_value);

    DPRINTF(D_ERROR,("KeyValuePair<K, V>::Print() : '%s' '%s'\n",
		     pKeyStr,
		     pValueStr));

    delete [] pKeyStr;
    delete [] pValueStr;
}

#endif // KV_PAIR_I
