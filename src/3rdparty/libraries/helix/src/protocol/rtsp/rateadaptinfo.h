/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rateadaptinfo.h,v 1.9 2007/07/06 20:51:35 jfinnecy Exp $
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
#ifndef RATEADAPTINFO_H
#define RATEADAPTINFO_H

#include "hxslist.h"


class HXRAIStreamInfo;
_INTERFACE IHXClientRateAdaptControl;
_INTERFACE IHXCommonClassFactory;;
_INTERFACE IHXPreferences;
_INTERFACE IHX3gppNADU;
_INTERFACE IHXValues;
_INTERFACE IHXBuffer;


class CHXRateAdaptationInfo
{
public:
    CHXRateAdaptationInfo();
    ~CHXRateAdaptationInfo();


    HX_RESULT Init(IUnknown* pContext);
    HX_RESULT Close();
    
    // Called when we get a stream header
    HX_RESULT OnStreamHeader(UINT16 uStreamNumber,
                             IHXValues* pHdr);

    // Called when the RTSP code needs rate adaptation
    // headers for a stream
    HX_RESULT CreateRateAdaptHeaders(UINT16 uStreamNumber,
                                     const char* pStreamURL,
                                     REF(IHXValues*) pHdrs);

    // Called when the server responds to our
    // rate adaptation request
    HX_RESULT OnRateAdaptResponse(UINT16 uStreamNumber,
                                  IHXValues* pReqHdrs,
                                  IHXValues* pRespHdrs);

    HXBOOL IsRateAdaptationInternal()   {return m_bRateAdaptationUsed;};

    void SetUseRTPFlag(HXBOOL bUseRTP)  {m_bUseRTP = bUseRTP;};

    HXBOOL GetUseRTPFlag()  { return m_bUseRTP;};

private:
    HXRAIStreamInfo* GetStreamInfo(UINT16 uStreamNumber) const;
    void RemoveStreamInfo(UINT16 uStreamNumber);
    IHXBuffer* Create3gpAdaptHdrs(const char* pStreamURL,
                                  HXRAIStreamInfo* pInfo);
    IHXBuffer* CreateHelixAdaptHdrs(const char* pStreamURL,
                                    HXRAIStreamInfo* pInfo);

    IUnknown* m_pContext;
    IHXClientRateAdaptControl* m_pRateAdaptCtl;
    IHXCommonClassFactory* m_pCCF;
    CHXSimpleList m_streamInfo;
    IHX3gppNADU* m_pNADU;
    HXBOOL m_bHlxAdaptEnabled;
    HXBOOL m_bRateAdaptationUsed;
    HXBOOL m_bUseRTP;
};
#endif /* RATEADAPTINFO_H */
