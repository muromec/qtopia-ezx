/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: servlist.cpp,v 1.3 2003/02/21 22:45:47 skharkar Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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
#include "servlist.h"

HXListElem::HXListElem()
{
    prev = next = this;
}

HXList::HXList()
{
    head = new HXListElem();
    size = 0;
}

HXList::~HXList()
{
    // XXX Should delete everything that is on the HXList too
    delete head;
}

void
HXList::insert(HXListElem* elem)
{
    elem->next = head;
    elem->prev = head->prev;
    head->prev->next = elem;
    head->prev = elem;
    size++;
}
 
void
HXList::insert_before(HXListElem* elem, HXListElem* where)
{
    if (where == 0)
    {
	where = head;
    } 
    elem->next = where;
    elem->prev = where->prev;
    where->prev->next = elem;
    where->prev = elem;
    size++;
}

void
HXList::insert_after(HXListElem* elem, HXListElem* where)
{
    if (where == 0)
    {
	where = head;
    } 
    elem->prev = where;
    elem->next = where->next;
    where->next->prev = elem;
    where->next = elem;
    size++;
}

BOOL
HXList::remove(HXListElem* elem)
{
    HXListElem* lpp;
    HXListElem* lp;
    BOOL bRemoved = FALSE;

    for (lpp = head; (lp = lpp->next) != head; lpp = lpp->next)
    {
	if (lp == elem)
	{
	    lpp->next = lp->next;
	    lp->next->prev = lpp;
	    lp->prev = lp->next = lp;
	    size--;
	    bRemoved = TRUE;
	    break;
	}
    }
    return bRemoved;
}

HXListElem* 
HXList::remove_head()
{
    HXListElem* return_elem;
    if (head->next == head)
    {
	return 0;
    }
    return_elem = head->next;
    head->next->next->prev = head;
    head->next = head->next->next;
    size--;

    return return_elem;
}

HXListElem* 
HXList::peek_head()
{
    if (head->next == head)
    {
	return 0;
    }
    return head->next;
}

HXListElem* 
HXList::peek_tail()
{
    if (head->prev == head)
    {
	return 0;
    }
    return head->prev;
}

#if 0
// Untested XXXSMP
HXListElem* 
HXList::remove_tail()
{
    HXListElem* return_elem;
    if (head->prev == head)
    {
	return 0;
    }
    return_elem = head->prev;
    head->prev->prev->next = head;
    head->prev = head->prev->prev;
    size--;

    return return_elem;
}
#endif
