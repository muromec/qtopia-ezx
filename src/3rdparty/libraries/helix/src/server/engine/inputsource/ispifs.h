/* ***** BEGIN LICENSE BLOCK *****  
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
#ifndef _ISPIFS_H_
#define _ISPIFS_H_

#include "isifs.h"
#include "ihxpckts.h"

typedef _INTERFACE  IHXSyncHeaderSource     IHXSyncHeaderSource;
typedef _INTERFACE  IHXRateDescVerifier     IHXRateDescVerifier;

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXSyncHeaderSource
 *	
 *  Purpose:
 *	Fetch headers synchronously
 * 
 *  IID_IHXSyncHeaderSource:
 * 
 *	{5E5ED607-79F0-4b8d-A0F6-70D6578D6399}
 * 
 */
DEFINE_GUID(IID_IHXSyncHeaderSource, 
    0x5e5ed607, 0x79f0, 0x4b8d, 0xa0, 0xf6, 0x70, 0xd6, 0x57, 0x8d, 0x63, 0x99);
 

#undef  INTERFACE
#define INTERFACE   IHXSyncHeaderSource

DECLARE_INTERFACE_(IHXSyncHeaderSource, IUnknown)
{
    /*
     * IUnknown Methods
     */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *	IHXSyncHeaderSource Methods
     */
    /************************************************************************
     *	Method:
     *	    IHXSyncHeaderSource::GetFileHeader
     *	Purpose:
     */
    STDMETHOD(GetFileHeader)	(THIS_ REF(IHXValues*)pHeader) PURE;
    
    /************************************************************************
     *	Method:
     *	    IHXSyncHeaderSource::GetStreamHeader
     *	Purpose:
     */
    STDMETHOD(GetStreamHeader)	(THIS_ UINT32 ulStreamNo, REF(IHXValues*)pHeader) PURE;
};

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXRateDescVerifier
 * 
 *  Purpose:
 * 
 * 
 *  IID_IHXRateDescVerifier:
 * 
 *	{EC48325F-4F89-4542-9451-F88FD37403EF}
 * 
 */
DEFINE_GUID(IID_IHXRateDescVerifier, 
    0xec48325f, 0x4f89, 0x4542, 0x94, 0x51, 0xf8, 0x8f, 0xd3, 0x74, 0x3, 0xef);

#undef  INTERFACE
#define INTERFACE   IHXRateDescVerifier

DECLARE_INTERFACE_(IHXRateDescVerifier, IUnknown)
{
    /*
     * IUnknown Methods
     */
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /************************************************************************
     *	Method:
     *	    IHXRateDescVerifier::Verify
     *	Purpose:     
     *	    Vertify for stream selection
     *  Returns:
     *	    HXR_OK
     *	    HXR_NOTENOUGH_BANDWIDTH
     *	    HXR_NOTENOUGH_PREDECBUF
     *	    HXR_NOT_SUPPORTED
     */
    STDMETHOD(Verify)   (THIS_ IHXRateDescription* pRateDesc) PURE;    
};

#endif /* _ISPIFS_H_ */
