/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: sdpchunk.h,v 1.5 2005/03/10 20:59:20 bobclark Exp $
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

#ifndef _SDPCHUNK_H_
#define _SDPCHUNK_H_

/****************************************************************************
 *  Includes
 */
#include "hxcom.h"
#include "hxtypes.h"
#include "hxcomm.h"
#include "ihxpckts.h"

/****************************************************************************
 *  Globals
 */
/****************************************************************************
 *  Types
 */
typedef enum
{
    SDPCTX_Session,
    SDPCTX_Media,
    SDPCTX_Time,
    SDPCTX_Generic,
    SDPCTX_Renderer,
    SDPCTX_Group,
    SDPCTX_Bandwidth
} SDPChunkContext;

typedef HX_RESULT (*SDPPullFuncPtr) (char* pData, 
				     ULONG32 ulLength, 
				     IHXValues* pSDPValues,
				     IHXCommonClassFactory* pClassFactory);

struct SDPRecordPuller
{
    char* pSDPMatch;
    ULONG32 ulSDPMatchSize;
    SDPPullFuncPtr pPullFunc;
};


/****************************************************************************
 *  Utilities
 */
HX_RESULT SDPParseChunk(char* pData,
			ULONG32 ulDataLen,
			IHXValues* &pSDPValues,
			IHXCommonClassFactory *pClassFactory,
			SDPChunkContext SDPContext = SDPCTX_Generic,
			HXBOOL bPullRecords = FALSE);

HX_RESULT SDPParseChunk(char* pData,
			ULONG32 ulDataLen,
			IHXValues* &pSDPValues,
			IHXCommonClassFactory *pClassFactory,
			const SDPRecordPuller* pPullTable,
			HXBOOL bPullRecords = FALSE);

HX_RESULT SDPMapPayloadToMime(ULONG32 ulPayloadType,
			      IHXBuffer* &pMimeType,
			      IHXCommonClassFactory *pClassFactory);

HX_RESULT SDPMapMimeToPayload(IHXBuffer* pMimeTypeBuffer, ULONG32 &ulPayload);

HXBOOL SDPIsKnownPayload(ULONG32 ulPayloadType);

HXBOOL SDPIsFixedRatePayload(ULONG32 ulPayloadType);

ULONG32 SDPMapMimeToSamplesPerSecond(IHXBuffer* pMimeTypeBuffer);

HX_RESULT SDPPullLine(char* pData, 
                      ULONG32 ulLength,
                      REF(IHXBuffer*) pLine,
                      IHXCommonClassFactory* pClassFactory);

#endif  // _SDPCHUNK_H_
