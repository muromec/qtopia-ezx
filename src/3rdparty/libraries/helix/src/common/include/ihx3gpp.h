/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: ihx3gpp.h,v 1.2 2004/11/19 18:20:17 acolwell Exp $ 
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
#ifndef IHX3GPP_H
#define IHX3GPP_H

/****************************************************************************
 * 
 *  Interface:
 * 
 *	IHX3gppOBSN
 * 
 *  Purpose:
 * 
 *	Provides information for 3GPP-Rel6 OBSN packets
 * 
 *  IID_IHX3gppNADU:
 * 
 *	{8069BAAF-777A-4cf0-9003-D26D68B8D786}
 * 
 */
DEFINE_GUID(IID_IHX3gppNADU, 
0x8069baaf, 0x777a, 0x4cf0, 0x90, 0x3, 0xd2, 0x6d, 0x68, 0xb8, 0xd7, 0x86);

#undef  INTERFACE
#define INTERFACE   IHX3gppNADU

DECLARE_INTERFACE_(IHX3gppNADU, IUnknown)
{
    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;

    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    /*
     * IHX3gppNADU Methods
     */

    /************************************************************************
     *	Method:
     *	    IHX3gppNADU::GetNADUInfo
     *	Purpose:
     *	    Get NADU information for the specified stream.
     *
     *      If this function returns HXR_OK then all the parameters
     *      are filled in with the appropriate values.
     *
     *      If this function returns HXR_NO_DATA then only uFreeBufferSpace
     *      contains a valid value.
     *
     */
    STDMETHOD(GetNADUInfo) (THIS_ UINT16 uStreamNumber,
                            REF(UINT16) uPlayoutDelay,
                            REF(UINT16) uNextSeqNumber,
                            REF(UINT16) uNextUnitNumber,
                            REF(UINT16) uFreeBufferSpace) PURE;

    /************************************************************************
     *	Method:
     *	    IHX3gppNADU::SetNADUParameters
     *	Purpose:
     *	    Notifies this object of the parameters negotiated in the 3GPP 
     *      rate adaptation exchange.
     *
     */
    STDMETHOD(SetNADUParameters) (THIS_ UINT16 uStreamNumber,
                                  UINT32 uFrequency,
                                  UINT32 uBufferSize) PURE;

    /************************************************************************
     *	Method:
     *	    IHX3gppNADU::GetNADUFrequency
     *	Purpose:
     *	    Get the frequency of NADU reports
     *
     */
    STDMETHOD(GetNADUFrequency) (THIS_ UINT16 uStreamNumber,
                                 REF(UINT32) uFrequency) PURE;

    /************************************************************************
     *	Method:
     *	    IHX3gppNADU::GetNADUBufferSize
     *	Purpose:
     *	    Get the client buffer size negotiated in the 3GPP
     *      rate adaptation exchange
     *
     */
    STDMETHOD(GetNADUBufferSize) (THIS_ UINT16 uStreamNumber,
                                  REF(UINT32) uBufferSize) PURE;

};

#endif /* IHX3GPP_H */
