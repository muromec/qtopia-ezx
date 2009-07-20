/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: watchlst.h,v 1.7 2009/05/30 19:10:24 atin Exp $
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

#ifndef _WATCHLIST_H_
#define _WATCHLIST_H_

class WListElem 
{
public:
		WListElem();
		~WListElem();

    WListElem*	prev;
    WListElem*	next;
    void*	data;
};

class WatchList 
{
public:
    WatchList();
    ~WatchList();
    
    void	insert(WListElem*);
    void	removeElem(WListElem*);
    int		empty();
    WListElem*	head;
};

inline
WListElem::~WListElem()
{
    next = prev = 0;
    data = 0;
}

inline int
WatchList::empty()
{
    return (head->prev == head->next && head->next == head);
}

class	WatchList_iterator
{
public:
			WatchList_iterator(WatchList*);
    WListElem*		operator*();
    WListElem*		operator++();
private:
    WListElem*		_h;
    WListElem*		_e;
    WListElem*		_n;
};

inline
WatchList_iterator::WatchList_iterator(WatchList* l)
{
   _h = l->head; 
   _e = l->head->next;
   _n = l->head->next->next;
}

inline WListElem*
WatchList_iterator::operator*()
{
    return (_e != _h) ? _e : 0;
}

inline WListElem*
WatchList_iterator::operator++()
{
    _e = _n;
    _n = _n->next;
    return _e;
}
#endif /* _WATCHLIST_H_ */
