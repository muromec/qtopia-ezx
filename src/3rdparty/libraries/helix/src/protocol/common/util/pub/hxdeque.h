/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxdeque.h,v 1.4 2005/03/10 20:59:17 bobclark Exp $
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

#ifndef _HXDEQUE_H_
#define _HXDEQUE_H_

#include "carray.h"

#if defined __QNXNTO__
/* This compiler gets this index() confused with the index() system call
 * in strings.h when compiling rtsptran.cpp.
 */
#define index HX_index
#endif

class HX_deque
{
public:
    static const u_long32	INVALID_INDEX;
    static const u_long32	INITIAL_ALLOCATION;

				HX_deque(u_long32 initial_allocation = 
					 INITIAL_ALLOCATION);
				~HX_deque();

    class Iterator
    {
    public:
	friend class		HX_deque;

				Iterator();
	Iterator&		operator++();
	HXBOOL			operator==(const Iterator& iter) const;
	HXBOOL			operator!=(const Iterator& iter) const;
	void*			operator*();

    private:
				Iterator(HX_deque* _deque, u_long32 _index);
	HX_deque*		deque;
	u_long32		index;
    };

    friend class Iterator;

    Iterator			begin();
    Iterator			end();
    u_long32			size();
    HXBOOL			empty();
    void*&			operator[](u_long32 index);
    void*&			front();
    void*&			back();

    void			push_front(void* item);
    void			push_back(void* item);
    void*			pop_front();
    void*			pop_back();

private:

    void			init(u_long32 initial_allocation);
    void			grow();
    u_long32			translate_index(u_long32 index);

    CHXPtrArray*		array;
    u_long32			front_index;
    u_long32			back_index;
    u_long32			num_items;
};

inline u_long32
HX_deque::size()
{
    return num_items;
}

inline HXBOOL
HX_deque::empty()
{
    return (num_items == 0);
}

inline 
HX_deque::Iterator::Iterator()
    :
    deque(0),
    index(HX_deque::INVALID_INDEX)
{
}

inline
HX_deque::Iterator::Iterator(HX_deque* _deque, u_long32 _index)
    :
    deque(_deque),
    index(_index)
{
}

inline HX_deque::Iterator&
HX_deque::Iterator::operator++()
{
    if (index == deque->back_index || index == HX_deque::INVALID_INDEX)
    {
	index = HX_deque::INVALID_INDEX;
    }
    else
    {
	++index;
	if (index == (u_long32) deque->array->GetSize())
	{
	    index = 0;
	}
    }
    return *this;
}

inline HXBOOL
HX_deque::Iterator::operator==(const HX_deque::Iterator& iter) const
{
    return ((deque == iter.deque) && (index == iter.index));
}

inline HXBOOL
HX_deque::Iterator::operator!=(const HX_deque::Iterator& iter) const
{
    return !(*this == iter);
}

inline void*
HX_deque::Iterator::operator*()
{
    if (index == HX_deque::INVALID_INDEX)
    {
	HX_ASSERT(0);
	return 0;
    }
    return (*deque->array)[HX_SAFEINT(index)];
}

inline HX_deque::Iterator
HX_deque::begin()
{
    if (empty())
    {
    	return HX_deque::Iterator(this, HX_deque::INVALID_INDEX);
    }
    return HX_deque::Iterator(this, front_index);
}

inline HX_deque::Iterator
HX_deque::end()
{
    return HX_deque::Iterator(this, HX_deque::INVALID_INDEX);
}

#endif /* _HXDEQUE_H_ */
