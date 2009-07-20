/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxdeque.cpp,v 1.5 2007/07/06 20:51:32 jfinnecy Exp $
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

#include "hxtypes.h"
#include "hxassert.h"
#include "carray.h"
#include "hxdeque.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif


const u_long32 HX_deque::INVALID_INDEX = 0xffffffff;
const u_long32 HX_deque::INITIAL_ALLOCATION = 50;

HX_deque::HX_deque(u_long32 initial_allocation)
    :
    array(0),
    num_items(0)
{
    init(initial_allocation);
}

HX_deque::~HX_deque()
{
    delete array;
}

void
HX_deque::init(u_long32 initial_allocation)
{
    array = new CHXPtrArray();
    array->SetSize(HX_SAFEINT(initial_allocation));
    back_index = 0;
    front_index = 1;
}

void
HX_deque::grow()
{
    UINT32	new_middle;

    new_middle = (UINT32) array->GetSize();
    array->SetSize(array->GetSize() * 2);

    if (back_index < front_index)
    {
	for (INT32 i = (INT32)back_index; i >= 0; --i)
	{
	    (*array)[HX_SAFEINT(new_middle + i)] = (*array)[HX_SAFEINT(i)];
	}
	back_index += new_middle;
    }
}

u_long32
HX_deque::translate_index(u_long32 index)
{
    if (index >= size())
    {
	HX_ASSERT(0);
	return 0;
    }
    if (front_index + index > (u_long32) array->GetSize() - 1)
    {
	return front_index + index - array->GetSize();
    }
    return front_index + index;
}

void*&
HX_deque::operator[](u_long32 index)
{
    if (index >= size())
    {
	HX_ASSERT(0);
	index = 0;
    }
    return (*array)[HX_SAFEINT(translate_index(index))];
}

void*&
HX_deque::front()
{
    if (empty())
    {
	HX_ASSERT(0);
    }
    return (*array)[HX_SAFEINT(front_index)];
}

void*&
HX_deque::back()
{
    if (empty())
    {
	HX_ASSERT(0);
    }
    return (*array)[HX_SAFEINT(back_index)];
}

void
HX_deque::push_front(void* item)
{
    if (num_items == (u_long32) array->GetSize())
    {
	grow();
    }
    if (front_index == 0)
    {
	front_index = (u_long32)(array->GetSize() - 1);
    }
    else
    {
	--front_index;
    }
    (*array)[HX_SAFEINT(front_index)] = item;
    ++num_items;
}

void
HX_deque::push_back(void* item)
{
    if (num_items == (u_long32) array->GetSize())
    {
	grow();
    }
    if (back_index == (u_long32) array->GetSize() - 1)
    {
	back_index = 0;
    }
    else
    {
	++back_index;
    }
    (*array)[HX_SAFEINT(back_index)] = item;
    ++num_items;
}

void* 
HX_deque::pop_front()
{
    if (empty())
    {
	HX_ASSERT(0);
	return 0;
    }
    void*   return_value;
    
    return_value = (*array)[HX_SAFEINT(front_index)];
    if (front_index == (u_long32) array->GetSize() - 1)
    {
	front_index = 0;
    }
    else
    {
	++front_index;
    }
    --num_items;

    return return_value;
}

void*
HX_deque::pop_back()
{
    if (empty())
    {
	HX_ASSERT(0);
	return 0;
    }
    void*   return_value;

    return_value = (*array)[HX_SAFEINT(back_index)];
    if (back_index == 0)
    {
	back_index = (u_long32)(array->GetSize() - 1);
    }
    else
    {
	--back_index;
    }
    --num_items;

    return return_value;
}

