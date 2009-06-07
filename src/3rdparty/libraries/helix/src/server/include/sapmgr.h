/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: sapmgr.h,v 1.2 2003/01/23 23:42:59 damonlan Exp $ 
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

#ifndef _SAPMGR_H_
#define _SAPMGR_H_

typedef LISTPOSITION SapHandle;

enum SapType
{
    SAP_ANNOUNCEMENT = 0,
    SAP_DELETION = 1
};


/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHXSapManager
 * 
 *  Purpose:
 *  
 *	Deals with Session Announcement Protocol
 * 
 *  IID_IHXSapManager:
 * 
 *	{00003400-0901-11d1-8B06-00A024406D59}
 * 
 */
DEFINE_GUID(IID_IHXSapManager, 0x00003400, 0x901, 0x11d1, 0x8b, 0x6, 
					  0x0, 0xa0, 0x24, 0x40, 0x6d, 0x59);

#undef  INTERFACE
#define INTERFACE   IHXSapManager

DECLARE_INTERFACE_(IHXSapManager, IUnknown)
{
    /*
     *  IUnknown methods
     */
    STDMETHOD (QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     *  IHXSapManager methods
     */
    /************************************************************************
     *	Method:
     *	    IHXSapManager::GetSAPInterval
     *	Purpose:
     *	    Get Sap packet transmission interval
     */
    STDMETHOD_(UINT32, GetSAPInterval)	    (THIS_ 
					    UINT8   uchTTL, 
					    UINT32  ulPktSize) PURE;

    /************************************************************************
     *	Method:
     *	    IHXSapManager::StartAnnouncement
     *	Purpose:
     *	    Start the announcement.  It will multicast this packet until 
     *	    StopAnnoucement is called for this.
     */
    STDMETHOD_(SapHandle, StartAnnouncement) (THIS_ 
					    IHXBuffer* pTextPayload, 
					    UINT8 uchTTL) PURE;
    
    /************************************************************************
     *	Method:
     *	    IHXSapManager::StopAnnouncement
     *	Purpose:
     *	    Stop announcement for hSap
     */
    STDMETHOD(StopAnnouncement)		    (THIS_ SapHandle hSap) PURE;
    

    /************************************************************************
     *	Method:
     *	    IHXSapManager::ChangeTTL
     *	Purpose:
     *	    change the TTL of announcement specified by hSap
     */
    STDMETHOD(ChangeTTL)		    (THIS_
					    SapHandle hSap,
					    UINT8 uchTTL) PURE;

    /************************************************************************
     *	Method:
     *	    IHXSapManager::IsAnnouncerEnabled
     *	Purpose:
     *	    Stop the directory
     */
    STDMETHOD_(BOOL, IsAnnouncerEnabled)    (THIS_) PURE;					    
};

#endif // _SAPMGR_H_




