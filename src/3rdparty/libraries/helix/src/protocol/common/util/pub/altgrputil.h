/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: altgrputil.h,v 1.6 2009/02/02 17:20:36 svaidhya Exp $
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
#ifndef ALTGRPUTIL_H
#define ALTGRPUTIL_H

#include "hxcom.h"
#include "ihxpckts.h"

class CHXAltGroupSelector;

class CHXAltGroupUtil
{
public:
    // Select groups based on preference values
    static HX_RESULT SelectAltGroups(IUnknown* pContext,
                                     UINT16 nValues, IHXValues** ppValues);

    // Select groups based on specified parameters. If ulMaxBW == 0, then
    // no bandwidth cap will be applied. If pLanguages == NULL or is an
    // empty string, then any language can be selected.
    static HX_RESULT SelectAltGroups(IUnknown* pContext,
                                     ULONG32 ulMaxBW, const char* pLanguages,
                                     UINT16 nValues, IHXValues** ppValues);

	static HX_RESULT CreateStreamGroups(IHXCommonClassFactory* pCCF, UINT16 nValues,
									REF(IHXValues**) ppValues, REF(UINT16) nStreamGrpCount);
#ifdef HELIX_FEATURE_SERVER_MR_RTP
	static HX_RESULT CreateSwitchGroups(UINT16 nValues, REF(IHXValues**) ppValues, HXBOOL bSourceQtbc);
#endif
        static HX_RESULT SetTrackID(UINT16 nValues, REF(IHXValues**) ppValues,REF (IHXValues*) pFileHeader);
	static HX_RESULT CreateASMRuleBook(IHXCommonClassFactory* pCCF,
									UINT16 nValues, REF(IHXValues**) ppValues);
        static HX_RESULT SortStreams ( REF(IHXValues**) ppHdrs, UINT32 nVal);
        static HX_RESULT AddASMRuleBook(REF(IHXValues**) ppHdrs, UINT32 nVal, IHXCommonClassFactory* pCCF);

private:
    static HX_RESULT AddLanguageFilter(CHXAltGroupSelector& selector,
                                       const char* pLanguages);
    static HX_RESULT AddMaxBWFilter(CHXAltGroupSelector& selector,
                                    UINT32 uMaxBW);
    static HX_RESULT AddHighBWFilter(CHXAltGroupSelector& selector);
    static HX_RESULT AddMimetypeFilter(CHXAltGroupSelector& selector,
                                       IUnknown* pContext);
};

#endif /* ALTGRPUTIL_H */
