/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: servlist.h,v 1.3 2003/02/21 22:45:47 skharkar Exp $ 
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

#ifndef _SERVLIST_H_
#define _SERVLIST_H_

class HXListElem 
{
public:
		HXListElem();

    HXListElem*	prev;
    HXListElem*	next;
};

class HXList 
{
public:
		HXList();
		~HXList();

    void	insert(HXListElem*);
    void	insert_before(HXListElem* elem, HXListElem* where);
    void	insert_after(HXListElem* elem, HXListElem* where);
    BOOL	remove(HXListElem*);
    HXListElem*	remove_head();
    HXListElem*	peek_head();
    HXListElem*	peek_tail();
    BOOL	empty();
    HXListElem*	head;
    UINT32	size;
};

inline BOOL
HXList::empty()
{
    return (head->prev == head->next && head->next == head);
}

class	HXList_iterator
{
public:
			HXList_iterator(HXList*);
    HXListElem*		operator*();
    HXListElem*		operator++();
private:
    HXListElem*		_h;
    HXListElem*		_e;
    HXListElem*		_n;
};

inline
HXList_iterator::HXList_iterator(HXList* l)
{
   _h = l->head; 
   _e = l->head->next;
   _n = l->head->next->next;
}

inline HXListElem*
HXList_iterator::operator*()
{
    return (_e != _h) ? _e : 0;
}

inline HXListElem*
HXList_iterator::operator++()
{
    _e = _n;
    _n = _n->next;
    return _e;
}

class	HXList_reverse_iterator
{
public:
			HXList_reverse_iterator(HXList*);
    HXListElem*		operator*();
    HXListElem*		operator++();
private:
    HXListElem*		_h;
    HXListElem*		_e;
    HXListElem*		_n;
};

inline
HXList_reverse_iterator::HXList_reverse_iterator(HXList* l)
{
   _h = l->head; 
   _e = l->head->prev;
   _n = l->head->prev->prev;
}

inline HXListElem*
HXList_reverse_iterator::operator*()
{
    return (_e != _h) ? _e : 0;
}

inline HXListElem*
HXList_reverse_iterator::operator++()
{
    _e = _n;
    _n = _n->prev;
    return _e;
}

#endif /* _SERVLIST_H_ */
