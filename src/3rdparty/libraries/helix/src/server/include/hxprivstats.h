/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxprivstats.h,v 1.2 2004/11/17 19:01:11 seansmith Exp $ 
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

///////////////////////////////////////////////////////////////////////////////
// hxprivstats.h - Private client stats interfaces. 
/////////////////////////////////////////////////////////////////////////////// 

#ifndef _HXPRIVSTATS_H
#define _HXPRIVSTATS_H


_INTERFACE      IUnknown;
_INTERFACE      IHXPrivateClientStats;


///////////////////////////////////////////////////////////////////////////////
// Interface: 
//
//      IHXPrivateClientStats
//
// Purpose:
//
//      Allows access to private client statistics. 
//
// IID_IHXPrivateClientStats:
//
//          {2399CC75-3C38-4680-ACA5-CD663A64701A}
//
///////////////////////////////////////////////////////////////////////////////

// {2399CC75-3C38-4680-ACA5-CD663A64701A}
DEFINE_GUID(IID_IHXPrivateClientStats, 0x2399cc75, 0x3c38, 0x4680, 0xac, 
            0xa5, 0xcd, 0x66, 0x3a, 0x64, 0x70, 0x1a);

#define CLSID_IHXPrivateClientStats    IID_IHXPrivateClientStats

#undef  INTERFACE
#define INTERFACE   IHXPrivateClientStats

///////////////////////////////////////////////////////////////////////////////
// End Status Values
///////////////////////////////////////////////////////////////////////////////

DECLARE_INTERFACE_(IHXPrivateClientStats, IUnknown)
{
    // IUnknown methods

    STDMETHOD(QueryInterface)		(THIS_
					                REFIID riid,
					                void** ppvObj) PURE;

    STDMETHOD_(ULONG32,AddRef)		(THIS) PURE;

    STDMETHOD_(ULONG32,Release)		(THIS) PURE;

    // IHXPrivateClientStats methods

    ///////////////////////////////////////////////////////////////////////////
    // Private statistics accessor/mutator methods.
    //
    // Purpose:
    //      All used to either acquire a property value or set it.
    //
    // Arguments:
    //      Mutators take an IN parameter, the new value to set the property
    //      to. Accessors take nothing.
    ///////////////////////////////////////////////////////////////////////////
    
    STDMETHOD_(BOOL, IsPrevAuth)            (THIS) PURE;
    STDMETHOD(SetPrevAuth)                  (THIS_
                                             BOOL bIsPrevAuth) PURE;

};

#endif
