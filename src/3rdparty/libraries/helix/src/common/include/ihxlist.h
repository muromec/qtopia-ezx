/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: ihxlist.h,v 1.2 2005/03/14 19:27:09 bobclark Exp $
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


#ifndef _IHXLIST_H_
#define _IHXLIST_H_


// IHXListIterator: e7ad1443-f6bf-4b0e-bc00-8f03c0b12724
DEFINE_GUID(IID_IHXListIterator, 0xe7ad1443, 0xf6bf, 0x4b0e, 0xbc, 0x00, 0x8f, 0x03, 0xc0, 0xb1, 0x27, 0x24);

#undef  INTERFACE
#define INTERFACE   IHXListIterator

DECLARE_INTERFACE_(IHXListIterator, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;
    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    // IHXListIterator
    STDMETHOD_(HXBOOL,HasItem)            (THIS) PURE;
    STDMETHOD_(HXBOOL,IsEqual)            (THIS_ IHXListIterator* pOther) PURE;
    STDMETHOD_(HXBOOL,MoveNext)           (THIS) PURE;
    STDMETHOD_(HXBOOL,MovePrev)           (THIS) PURE;
    STDMETHOD_(IUnknown*,GetItem)       (THIS) PURE;
};

// IHXList: 1599cb17-9db4-4f8a-865a-78a54effbc60
DEFINE_GUID(IID_IHXList, 0x1599cb17, 0x9db4, 0x4f8a, 0x86, 0x5a, 0x78, 0xa5, 0x4e, 0xff, 0xbc, 0x60);

#define CLSID_IHXList IID_IHXList

#undef  INTERFACE
#define INTERFACE   IHXList

DECLARE_INTERFACE_(IHXList, IUnknown)
{
    // IUnknown
    STDMETHOD(QueryInterface)           (THIS_
                                        REFIID riid,
                                        void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)          (THIS) PURE;
    STDMETHOD_(ULONG32,Release)         (THIS) PURE;

    // IHXList
    STDMETHOD_(HXBOOL,IsEmpty)            (THIS) PURE;
    STDMETHOD_(UINT32,GetCount)         (THIS) PURE;
    STDMETHOD_(IHXList*,AsConst)       (THIS) PURE;
    STDMETHOD(InsertHead)               (THIS_
                                        IUnknown* punkItem) PURE;
    STDMETHOD(InsertTail)               (THIS_
                                        IUnknown* punkItem) PURE;
    STDMETHOD(InsertBefore)             (THIS_
                                        IHXListIterator* pIter,
                                        IUnknown* punkItem) PURE;
    STDMETHOD(InsertAfter)              (THIS_
                                        IHXListIterator* pIter,
                                        IUnknown* punkItem) PURE;
    STDMETHOD(Replace)                  (THIS_
                                        IHXListIterator* pIter,
                                        IUnknown* punkItem) PURE;
    STDMETHOD_(IUnknown*,Remove)        (THIS_
                                        IHXListIterator* pIter) PURE;
    STDMETHOD_(IUnknown*,RemoveHead)    (THIS) PURE;
    STDMETHOD_(IUnknown*,RemoveTail)    (THIS) PURE;
    STDMETHOD_(IHXListIterator*,Begin) (THIS) PURE;
    STDMETHOD_(IHXListIterator*,End)   (THIS) PURE;
    
    STDMETHOD_(IUnknown*,GetHead)       (THIS) PURE;
    STDMETHOD_(IUnknown*,GetTail)       (THIS) PURE;
};

#endif /* _IHXLIST_H_ */
