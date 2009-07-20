/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ihxperplex.h,v 1.3 2004/07/09 18:20:48 hubbe Exp $
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


#ifndef _IHXPERPLEX_H
#define _IHXPERPLEX_H

/******************************************************************************/
/****************************** INCLUDE FILES *********************************/
/******************************************************************************/

#include "hxcom.h"
#include "ihxpckts.h"	// for IHXBuffer
#include "hxstring.h"	// for CHXString

/******************************************************************************/
/************************** INTERFACE DEFINITIONS *****************************/
/******************************************************************************/

//*****************************************************************************
// Interface:	IHXMessRequest
// Purpose:	To be used by the client of the queue to submit a request
// Notes:	N/A
//*****************************************************************************

#undef  INTERFACE
#define INTERFACE IHXMessRequest

// {B0F17EE1-DD86-11d2-B339-00C0F0318798}   Defined by plamanna March 18 1999
DEFINE_GUID(IID_IHXPerplex, 
0xb0f17ee1, 0xdd86, 0x11d2, 0xb3, 0x39, 0x0, 0xc0, 0xf0, 0x31, 0x87, 0x98);

DECLARE_INTERFACE_(IHXPerplex, IUnknown)
{
    // IUnknown methods
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj) PURE;
    STDMETHOD_(ULONG32,AddRef)	(THIS) PURE;
    STDMETHOD_(ULONG32,Release)	(THIS) PURE;

    STDMETHOD(Perplex)		(THIS_ IHXBuffer *pInBuf, IHXBuffer *pOutBuf) PURE;
    STDMETHOD(DePerplex)	(THIS_ IHXBuffer *pInBuf, IHXBuffer *pOutBuf) PURE;
};

#endif
