/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cachobj.h,v 1.5 2007/07/06 20:39:23 jfinnecy Exp $
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

#ifndef _HXFIFOCACHE_
#define _HXFIFOCACHE_

#include "hxcache.h"

class CChunkyRes;

struct ChunkyCacheLayout
{
    UINT16  size;   
    GUID    guid;
};
    
class HXFIFOCache : public IHXFIFOCache
{
private:
    IUnknown*		m_pContext;
    LONG32		m_lRefCount;
    CChunkyRes*		m_pChunkyRes;
    UINT32		m_ulCurrentReadPosition;
    UINT32		m_ulCurrentWritePosition;
public:
    HXFIFOCache(IUnknown* pContext);
    ~HXFIFOCache();

    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

    /*
     * IHXFIFOCache methods
     */

    /************************************************************************
     *	Method:
     *		IHXFIFOCache::Cache
     *	Purpose:
     *	    To cache objects in a temporary storage for later retrieval
     *
     *	    Currently supported objects:
     *		IHXBuffer
     *		IHXValues
     *		IHXPacket
// $Private:
     *		IHXTimeStampedBuffer
     *		IHXClientPacket
// $EndPrivate.
     *
     */
    STDMETHOD(Cache)	(THIS_
			 IUnknown*	    pObject);

    /************************************************************************
     *	Method:
     *	    IHXFIFOCache::Retrieve
     *	Purpose:
     *
     *
     */
    STDMETHOD(Retrieve)	    (THIS_
			    REF(IUnknown*)  pObject);

    /************************************************************************
     *	Method:
     *	    IHXFIFOCache::Flush
     *	Purpose:
     *
     *
     */
    STDMETHOD(Flush)	    (THIS);

private:
    HX_RESULT CacheClientPacket(IHXClientPacket* pClientPacket);
    HX_RESULT CacheTimestampBuffer(IHXTimeStampedBuffer* pTimeStampBuffer);
    HX_RESULT CacheBuffer(IHXBuffer* pBuffer);
    HX_RESULT CachePacket(IHXPacket* pPacket);
    HX_RESULT CacheValues(IHXValues* pValues);   
};

#endif /*_HXFIFOCACHE_*/
