/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxcache2.h,v 1.7 2007/07/06 20:43:41 jfinnecy Exp $
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


#ifndef _HXCACHE2_H_
#define _HXCACHE2_H_

/*
 * Forward declarations of some interfaces defined herein.
 */

typedef _INTERFACE	IHXCacheObject			IHXCacheObject;
typedef _INTERFACE	IHXCacheObjectResponse		IHXCacheObjectResponse;


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXCache2
 * 
 *  Purpose:
 *
 *      IHXCache2 acts as a factory for IHXCacheObject objects.  
 * 
 *  IID_IHXCache2:
 * 
 *	{00002E0E-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXCache2, 0x00002E0E, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXCache2

DECLARE_INTERFACE_(IHXCache2, IUnknown)
{
    /*
     *	IUnknown methods
     */

    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;


    /*
     *	IHXCache2 methods
     */

    /************************************************************************
     *	Method:
     *
     *	    IHXCache2::CreateMemCacheObject
     *
     *	Purpose:
     *
     *      Creates an object which implements the IHXCacheObject interface.
     *      This object uses ONLY the memory for caching.
     */

    STDMETHOD(CreateMemCacheObject)	(THIS_
				         IHXCacheObject**          /*OUT*/	ppObject,
                                         IHXCommonClassFactory*    /*IN*/  pClassFactory) PURE;



#ifdef HELIX_FEATURE_FILECACHE // use the local file system for caching

    /************************************************************************
     *	Method:
     *
     *	    IHXCache2::CreateFileCacheObject
     *
     *	Purpose:
     *
     *      Creates an object which implements the IHXCacheObject interface.
     *      This object *uses the local filesystem* in addition to the memory
     *      for caching.
     */

    STDMETHOD(CreateFileCacheObject)	(THIS_
				         IHXCacheObject**           /*OUT*/	ppObject,
                                         IHXCommonClassFactory*     /*IN*/      pClassFactory,
                                         UINT32                     /*IN*/      ulFileLength,
                                         char*                      /*IN*/      pFileName) PURE;
#endif

   
}; // IHXCache2 


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXCacheObject
 * 
 *  Purpose:
 * 
 *      This interface defines methods that are implemented by a simple
 *      cache object. The cache object has only one RESTRICTION and that 
 *      is the data should be added in a linear fashion. To be precise,
 *      the starting offset of the currently being added block should be
 *      exactly one more than the ending offset of the most previously 
 *      added block. Data can be read out in any order.
 *
 *   
 * 
 *  IID_IHXCacheObject:
 * 
 *	{00002E10-0901-11d1-8B06-00A024406D59}
 * 
 */

DEFINE_GUID(IID_IHXCacheObject, 0x00002E10, 0x901, 0x11d1, 0x8b, 0x6, 0x0, 
			0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXCacheObject

DECLARE_INTERFACE_(IHXCacheObject, IUnknown)
{
    /*
     *	IUnknown methods
     */

    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXCacheObject methods
     */

    /************************************************************************
     *	Method:
     *
     *	    IHXCacheObject::Init
     *
     *	Purpose:
     *
     *	    Associates a cache object with the response object
     *	    it should notify of operation completeness. Also, sets the Capacity
     *      and threshold of the cache object. The capacity determines how much
     *      data the object can store before it needs to discard already present 
     *      data. Threshold determines when cached data is discarded. If the amount
     *      of data which has been read out of the cache is > (STRICTLY GREATER)
     *      than the threshold percentage of the total capacity (not the current
     *      amount of data present in the cache), then exactly that much amount of
     *      data is discarded so that the threshold condition is satisfied. Note that
     *      0 <= uThreshold < 100.
     */

    STDMETHOD(Init)	(THIS_
			IHXCacheObjectResponse*   /*IN*/  pCacheObjectResponse,
                        UINT32                    /*IN*/  ulCapacity,
                        UINT32                    /*IN*/  uThreshold) PURE;


    /************************************************************************
     *	Method:
     *
     *	    IHXCacheObject::GetThreshold
     *
     *	Purpose:
     *
     *	    Obtain the threshold of the cache object.
     */

    STDMETHOD_(UINT32, GetThreshold)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *
     *	    IHXCacheObject::ChangeThreshold
     *
     *	Purpose:
     *
     *	    The object keeps caching data until it is full (exhausts its 
     *      capacity). Once it is full, it will overwite existing cached data
     *      with new data ONLY if the percentage of cached data which has been
     *      read from the cache using the ReadBlock() method is greater than a
     *      given percentage of Capacity. This percentage is set by Init()
     *      method. In case the threshold is exceeded, the oldest added data
     *      (the data with the least offset ) will be discarded and the 
     *      amount of data discarded is so that the remaining cached data exactly
     *      satisfies the threshold condidtion.
     *
     *      For eg., this cache object is used in the HTTP/1.0 file system plugin for
     *      mobile devices and in this case, the threshold was set to 70%
     *      i.e., uNewThreshold = 70
     *
     */

    STDMETHOD(ChangeThreshold)      (THIS_
			             UINT32  /*IN*/	uNewThreshold) PURE;

    /************************************************************************
     *	Method:
     *
     *	    IHXCacheObject::GetCapacity
     *
     *	Purpose:
     *
     *	    Obtain the capacity in bytes of the cache object.
     */

    STDMETHOD_(UINT32, GetCapacity)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *
     *	    IHXCacheObject::ChangeCapacity
     *
     *	Purpose:
     *
     *	    Changes the capacity of the cache object (in bytes). It is used to
     *      increase or decrease the capacity after the cache object has been
     *      created and it's capacity set usinf Init(). This method can
     *      be called any number of times. If new capacity is less than old capacity,
     *      the oldest data are discarded.
     *
     */

    STDMETHOD(ChangeCapacity)      (THIS_
			            UINT32  /*IN*/	newByteCount) PURE;

    /************************************************************************
     *	Method:
     *
     *	    IHXCacheObject::GetUnusedCapacity
     *
     *	Purpose:
     *
     *	    Obtain the unused capacity in bytes of the cache object.
     */

    STDMETHOD_(UINT32, GetUnusedCapacity)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *
     *	    IHXCacheObject::AddBlock
     *
     *	Purpose:
     *
     *	    Adds a block of data to the cache. 
     *      NOTE: !!! The data should be added in a linear fashion. To be precise,
     *      the starting offset of the currently being added block should be
     *      exactly one more than the ending offset of the most previously 
     *      added block.
     *      Returns HXR_OK for success and HXR_OUTOFMEMORY if the cache can't
     *      accomodate the block (since it has a fixed capacity).
     *      
     */

    STDMETHOD(AddBlock)	(THIS_
			 IHXBuffer*		/*IN*/	pBlock) PURE;

    /************************************************************************
     *	Method:
     *
     *	    IHXCacheObject::VerifyBlock
     *
     *	Purpose:
     *
     *	    Verify that a block of data is in the cache.
     */

    STDMETHOD(VerifyBlock)	(THIS_
				 UINT32		/*IN*/	ulBlockOffset,
				 UINT32		/*IN*/	ulBlockLength) PURE;

    /************************************************************************
     *	Method:
     *
     *	    IHXCacheObject::ReadBlock
     *
     *	Purpose:
     *
     *	    Read a block out of the cache. Returns HXR_OK if it can supply
     *      *all* data. It returns HXR_FAIL if, at some point in the past,
     *      it had the data but has since discarded (a part of whole of) it.
     *      It returns HXR_INCOMPLETE in all other cases, viz
     *       
     *          (1) Only part of the data is present but no part of the data
     *              was discarded in the past.
     *          (2) No part of the requested data is present but is expected
     *              to be recieved in the future.
     *
     *      To understand it clearly, visulaize yourself storing the data of a
     *      remote file as it arrives to you. As you keep storing more and more
     *      data, then the cache keeps discarding some of the old data. Now, this
     *      discarded data can never be got back by cache. Hence when the user
     *      request contains some data which has already been discarded, then 
     *      the cache replies back with HXR_FAIL otherwise it is HXR_INCOMPLETE
     *      or HXR_OK depending on whether on how much of data it can supply *now*.
     *
     */

    STDMETHOD(ReadBlock)	(THIS_
				 UINT32		/*IN*/	ulBlockOffset,
				 UINT32		/*IN*/	ulBlockLength) PURE;

    /************************************************************************
     *	Method:
     *	    IHXCacheObject::Flush
     *
     *	Purpose:
     *
     *	    Releases all data buffers cached in the object. The object now
     *      gets to a same state as it was when newly created. After flushing,
     *	    the object can be used for writing/reading as before.
     */

    STDMETHOD(Flush)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *
     *	    IHXCacheObject::IsFull
     *
     *	Purpose:
     *
     *	    Is UnusedCapacity = 0?
     */

    STDMETHOD_(HXBOOL, IsFull)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *
     *	    IHXCacheObject::IsEmpty
     *
     *	Purpose:
     *
     *	    Is UnusedCapacity = TotalCapacity?
     */

    STDMETHOD_(HXBOOL, IsEmpty)	(THIS) PURE;

}; // IHXCacheObject


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXCacheObjectResponse
 * 
 *  Purpose:
 * 
 *	Response interface for IHXCacheObject operations
 * 
 *  IID_IHXCacheObjectResponse:
 * 
 *	{00002E11-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXCacheObjectResponse, 0x00002E11, 0x901, 0x11d1, 0x8b, 0x6,
		0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXCacheObjectResponse

DECLARE_INTERFACE_(IHXCacheObjectResponse, IUnknown)
{
    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXCacheObjectResponse methods
     */

    /************************************************************************
     *	Method:
     *
     *	    IHXCacheObjectResponse::InitDone
     *
     *	Purpose:
     *
     *	    Notification that IHXCacheObject::Init call has completed.
     */

    STDMETHOD(InitDone)		(THIS_
				 HX_RESULT	/*IN*/	status) PURE;

    /************************************************************************
     *	Method:
     *
     *	    IHXCacheObjectResponse::ChangeCapacityDone
     *
     *	Purpose:
     *
     *	    Notification that IHXCacheObject::ChangeCapacity call has completed.
     */

    STDMETHOD(ChangeCapacityDone)		(THIS_
				                HX_RESULT	/*IN*/	status) PURE;

    /************************************************************************
     *	Method:
     *
     *	    IHXCacheObjectResponse::AddBlockDone
     *
     *	Purpose:
     *
     *	    Notification that IHXCacheObject::AddBlock operation has completed.
     */

    STDMETHOD(AddBlockDone)	(THIS_
				HX_RESULT	/*IN*/	status) PURE;

    /************************************************************************
     *	Method:
     *
     *	    IHXCacheObjectResponse::VerifyBlockDone
     *
     *	Purpose:
     *
     *	    Notification that IHXCacheObject::VerifyBlock operation has
     *	    completed.
     */

    STDMETHOD(VerifyBlockDone)	(THIS_
				HXBOOL		/*IN*/	bExist) PURE;

    /************************************************************************
     *	Method:
     *
     *	    IHXCacheObjectResponse::ReadBlockDone
     *
     *	Purpose:
     *
     *	    Notification that IHXCacheObject::ReadBlock operation has
     *	    completed.
     */

    STDMETHOD(ReadBlockDone)	(THIS_
				HX_RESULT	/*IN*/	status,
				IHXBuffer*	/*IN*/	pBuffer) PURE;

    /************************************************************************
     *	Method:
     *
     *	    IHXCacheObjectResponse::FlushDone
     *
     *	Purpose:
     *
     *	    Notification that IHXCacheObject::Flush operation has completed.
     */

    STDMETHOD(FlushDone)	(THIS_
				HX_RESULT	/*IN*/	status) PURE;


}; // IHXCacheObjectResponse



#endif  /* _HXCACHE2_H_ */
