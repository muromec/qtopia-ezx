/* ***** BEGIN LICENSE BLOCK *****
 * Version: RCSL 1.0/RPSL 1.0
 *
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved.
 *
 * The contents of this file, and the files included with this file, are
 * subject to the current version of the RealNetworks Public Source License
 * Version 1.0 (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the RealNetworks Community Source License Version 1.0
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl,
 * in which case the RCSL will apply. You may also obtain the license terms
 * directly from RealNetworks.  You may not use this file except in
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks
 * applicable to this file, the RCSL.  Please see the applicable RPSL or
 * RCSL for the rights, obligations and limitations governing use of the
 * contents of the file.
 *
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the portions
 * it created.
 *
 * This file, and the files included with this file, is distributed and made
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT.
 *
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 *
 * Contributor(s):
 *
 * ***** END LICENSE BLOCK ***** */
/*
 * blist.h: block list
 */

#ifndef _BLIST_H_
#define _BLIST_H_

#define BLIST_DEFAULT_BLOCK_SIZE 256
class CBList
{
public:
    CBList( int iBlockSize = BLIST_DEFAULT_BLOCK_SIZE );
    ~CBList();

    // get, set
    void*  GetAt(int idx) const;
    void   SetAt(int idx, void* value);
    void*  operator[](int idx) const
	{
	    return GetAt(idx);
	}
    void*& operator[](int idx);

    // controls for growing
    void   SetBlockSize( int iBlockSize )
	{
	    if( iBlockSize )
	    {
		m_iBlockSize = iBlockSize;
	    }
	}

    // dump everything to an external buffer
    void   DumpToBuffer( void* pBuf, bool bWordSwap = TRUE );
    
    // state - totalsize includes the unused
    // portions that have been allocated
    int    GetSize()
	{
	    return m_iSize;
	}
    int    GetTotalSize()
	{
	    return m_iTotalSize;
	}
private:
    int m_iBlockSize;

    int m_iSize;
    int m_iTotalSize;

    typedef struct _memblock_s
    {
	void** ppBlock;
	int    iBlockSize;
	int    iBlockLen;

	struct _memblock_s* pNext;
    } _memblock_t;
    
    _memblock_t* m_pHead;

    _memblock_t* NewBlock();
    
};

#endif /* _BLIST_H_ */
