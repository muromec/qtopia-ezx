/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: altgrputil.cpp,v 1.12 2009/03/17 17:25:43 ckarusala Exp $
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
#include "altgrputil.h"

#include "altgrpselect.h"
#include "langaltfilt.h"
#include "maxbwaltfilt.h"
#include "highbwaltfilt.h"
#include "mimealtfilt.h"
#include "safestring.h"

#include "hxslist.h"
#include "hxprefs.h"
#include "hxprefutil.h"
#include "hxengin.h"

#include "hxswitch.h"

#ifdef HELIX_FEATURE_SERVER_MR_RTP
#include "switchverifier.h"
#endif

#include "ihxpckts.h"

#define MAX_STREAM_GRP 3
#define MAX_RULE_BOOK 1000

HX_RESULT 
CHXAltGroupUtil::SelectAltGroups(IUnknown* pContext,
                                 UINT16 nValues, IHXValues** ppValues)
{
    HX_RESULT res = HXR_OK;
    
    if (pContext && ppValues && nValues)
    {
        ULONG32 ulMaxBW = 0;
        CHXString languages;
        IHXPreferences* pPrefs = NULL;
        
        if (HXR_OK == pContext->QueryInterface(IID_IHXPreferences, (void**)&pPrefs))
        {
            ReadPrefUINT32(pPrefs, "MaxBandwidth", ulMaxBW);
            ReadPrefCSTRING(pPrefs, "Language", languages);

            res = SelectAltGroups(pContext, ulMaxBW, languages, nValues,
                                  ppValues);
        }
        HX_RELEASE(pPrefs);
    }

    return res;
}

HX_RESULT 
CHXAltGroupUtil::SelectAltGroups(IUnknown* pContext,
                                 ULONG32 ulMaxBW, const char* pLanguages,
                                 UINT16 nValues, IHXValues** ppValues)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;
    
    if (ulMaxBW == 0 && pContext)
    {
        IHXConnectionBWInfo* pConnBWInfo = NULL;
        if (HXR_OK == pContext->QueryInterface(IID_IHXConnectionBWInfo, (void**)&pConnBWInfo))
        {
            pConnBWInfo->GetConnectionBW(ulMaxBW, FALSE);
            HX_RELEASE(pConnBWInfo);
        }
    }

    if (pContext && ppValues && nValues)
    {
        IHXValues** ppNewValues = new IHXValues*[nValues];
        
        if (ppNewValues)
        {
            CHXAltGroupSelector selector;
            
            res = selector.Init(pContext);
            
            if (HXR_OK == res)
            {
                res = AddMimetypeFilter(selector, pContext);
            }

            if ((HXR_OK == res) && pLanguages && strlen(pLanguages))
            {
                res = AddLanguageFilter(selector, pLanguages);
            }
                
            if ((HXR_OK == res) && ulMaxBW)
            {
                res = AddMaxBWFilter(selector, ulMaxBW);
            }
                
            if (HXR_OK == res)
            {
                res = AddHighBWFilter(selector);
            }
                
            if (HXR_OK == res)
            {
                res = selector.Select(nValues, ppValues, ppNewValues);
                    
                if (HXR_OK == res)
                {
                    for (UINT16 i = 0; i < nValues; i++)
                    {
                        // Release reference to old header
                        HX_RELEASE(ppValues[i]);
                        
                        // Transfer ownership of new header
                        ppValues[i] = ppNewValues[i];
                        ppNewValues[i] = NULL;
                    }
                }
            }
            HX_VECTOR_DELETE(ppNewValues);
        }
        else
        {
            res = HXR_OUTOFMEMORY;
        }
    }

    return res;
}

HX_RESULT 
CHXAltGroupUtil::AddLanguageFilter(CHXAltGroupSelector& selector,
                                   const char* pLanguages)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pLanguages)
    {
        CHXLanguageAltIDFilter* pLangFilt = new CHXLanguageAltIDFilter;
    
        if (pLangFilt)
        {
            res = pLangFilt->Init(pLanguages);
            
            if (HXR_OK == res)
            {
                res = selector.AddFilter(pLangFilt);
                
                if (HXR_OK == res)
                {
                    // clear pointer since ownership was transfered
                    pLangFilt = NULL;
                }
            }
                HX_DELETE(pLangFilt);
        }
        else
        {
            res = HXR_OUTOFMEMORY;
        }
    }

    return res;
}

HX_RESULT 
CHXAltGroupUtil::AddMaxBWFilter(CHXAltGroupSelector& selector,
                                UINT32 uMaxBW)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (uMaxBW)
    {
        CHXMaxBWAltIDFilter* pMaxBWFilt = new CHXMaxBWAltIDFilter;
        
        if (pMaxBWFilt)
        {
            res = pMaxBWFilt->Init(uMaxBW);
            
            if (HXR_OK == res)
            {
                res = selector.AddFilter(pMaxBWFilt);

                if (HXR_OK == res)
                {
                    // clear pointer since ownership was transfered
                    pMaxBWFilt = NULL;
                }
            }
            
            HX_DELETE(pMaxBWFilt);
        }
        else
        {
            res = HXR_OUTOFMEMORY;
        }
    }

    return res;
}

HX_RESULT 
CHXAltGroupUtil::AddHighBWFilter(CHXAltGroupSelector& selector)
{
    HX_RESULT res = HXR_OUTOFMEMORY;

    CHXHighestBWAltIDFilter* pHighBWFilt = new CHXHighestBWAltIDFilter;
    
    if (pHighBWFilt)
    {
        res = selector.AddFilter(pHighBWFilt);
        
        if (HXR_OK == res)
        {
            // clear pointer since ownership was transfered
            pHighBWFilt = NULL;
        }
        HX_DELETE(pHighBWFilt);
    }

    return res;
}

HX_RESULT 
CHXAltGroupUtil::AddMimetypeFilter(CHXAltGroupSelector& selector,
                                   IUnknown* pContext)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pContext)
    {
        CHXMimetypeAltIDFilter* pMimetypeFilt = new CHXMimetypeAltIDFilter;
        
        if (pMimetypeFilt)
        {
            res = pMimetypeFilt->Init(pContext);
            
            if (HXR_OK == res)
            {
                res = selector.AddFilter(pMimetypeFilt);

                if (HXR_OK == res)
                {
                    // clear pointer since ownership was transfered
                    pMimetypeFilt = NULL;
                }
            }
            
            HX_DELETE(pMimetypeFilt);
        }
        else
        {
            res = HXR_OUTOFMEMORY;
        }
    }
    return res;
}

/**
 * CreateStreamGroups  - The method create groupings based on the Media Type i.e
 * Streams with the same media types are grouped together, with the exception of
 * video/3gpp-tt Mime type, which are storeed as a separate stream group.
 * This method is used by the server to stream a feed from a live encoder which do not
 * specify stream groupings
 */
HX_RESULT
CHXAltGroupUtil::CreateStreamGroups(IHXCommonClassFactory* pCCF, UINT16 nValues, 
                                   REF(IHXValues**) ppValues, REF(UINT16) nStreamGrpCount)
{
    HX_RESULT res = HXR_OK;
    IHXValues* pStreamGrps = NULL;
    
    //Validations
    if (!pCCF || nValues == 0 || !ppValues)
    {
        return HXR_FAIL;
    }

    //Get IHXValues for internal processing
    if (HXR_OK != pCCF->CreateInstance(IID_IHXValues, (void**)&pStreamGrps))
    {
        return HXR_FAIL;
    }

    //Holds the streamGroup number of the next new streamgroup to be created.
    UINT16 nStreamCnt = 0;

    //To Handle video/3gpp-tt
    HXBOOL bVideoTTFound = FALSE;
    UINT32 ulVideoTTStreamNum;

    char* pMimeType = NULL;

    //For every Stream
    for (UINT16 i = 0; i < nValues; i++)
    {
        IHXBuffer* pBufMimeType = NULL;

        if (ppValues[i]->GetPropertyCString("MimeType",pBufMimeType) == HXR_OK)
        {
            pMimeType = (char*)(pBufMimeType->GetBuffer());
        }
        HX_ASSERT(pMimeType);
                
        //Handle Exception
        if (!strncasecmp(pMimeType,"video/3gpp-tt",strlen("video/3gpp-tt")))
        {
            if (!bVideoTTFound)
            {
                bVideoTTFound = TRUE;
                ulVideoTTStreamNum = nStreamCnt++;
            }
            ppValues[i]->SetPropertyULONG32("StreamGroupNumber", ulVideoTTStreamNum);
        }
        else    //Normal Case
        {
            UINT32 ulCurStreamGrpNum;
            char* pSlash = NULL;
            INT32 nLen = 0;

            //Extract MediaType from MimeType
            pSlash = strchr(pMimeType, '/');
            HX_ASSERT(pSlash);
            nLen = pSlash-pMimeType;

            //Form MediaType
            char pMediaType[256];
            strncpy(pMediaType, pMimeType, nLen);
            pMediaType[nLen] = '\0';

            if (HXR_OK == pStreamGrps->GetPropertyULONG32(pMediaType, ulCurStreamGrpNum))
            { 
                ppValues[i]->SetPropertyULONG32("StreamGroupNumber", ulCurStreamGrpNum);
            }
            else
            { 
                pStreamGrps->SetPropertyULONG32(pMediaType, nStreamCnt);
                ppValues[i]->SetPropertyULONG32("StreamGroupNumber", nStreamCnt);
                nStreamCnt++;
            }
        }

        HX_RELEASE(pBufMimeType);
    }
    nStreamGrpCount = nStreamCnt;
    HX_RELEASE(pStreamGrps);
    return res;
}

#ifdef HELIX_FEATURE_SERVER_MR_RTP
HX_RESULT 
CHXAltGroupUtil::CreateSwitchGroups(UINT16 nValues, REF(IHXValues**) ppValues, HXBOOL bSourceQtbc)
{
    HX_RESULT res = HXR_OK;

    if (nValues == 0)
    {
        return HXR_FAIL;
    }

    //Stores one stream for each "SwitchGroupID" so that it can be used for switcability test
    //with other streams
    IHXValues **pSwitchGrpStream = new IHXValues*[nValues];

    //Stores the "StreamGroup" Number for Streams in **pSwitchGrpStream
    UINT16* StreamGroupNum = new UINT16[nValues];

    //This variable stores the total SwitchGroups and also acts as an index to **pSwitchGrpStream
    UINT16 nSwitchGrpCnt = 0;

    //Create Switch Verifier;
    CHXVerifySwitch* pSwitchVerifier = new CHXVerifySwitch();
    pSwitchVerifier->AddRef();

    //For QTBCPln we need to supress the Bitrate Compatibility Chk
    if (bSourceQtbc)
    {
        pSwitchVerifier->NoBitrateChk();
    }

    HXBOOL bSwitchGrpFound;
    UINT32 ulCurStreamGrpNum;
    for (UINT16 i = 0 ; i < nValues; i++)
    {
        bSwitchGrpFound = FALSE;

        if (HXR_OK != ppValues[i]->GetPropertyULONG32("StreamGroupNumber", ulCurStreamGrpNum))
        {
            res = HXR_FAIL;
            goto bail;
        }

        for (UINT16 j = 0; j < nSwitchGrpCnt ; j++)
        { 
            //Switchability test should me made only for streams under same stream grp.
            if (ulCurStreamGrpNum == StreamGroupNum[j])
            {
                //Check for lang attribute
                IHXBuffer* pBufLang1 = NULL;
                char *pLang1 = NULL;

                IHXBuffer* pBufLang2 = NULL;
                char *pLang2 = NULL;

                if (ppValues[i]->GetPropertyCString("lang",pBufLang1) == HXR_OK)
                {
                    pLang1 = (char*)(pBufLang1->GetBuffer());
                    HX_ASSERT(pLang1);
                }
                if (pSwitchGrpStream[j]->GetPropertyCString("lang",pBufLang2) == HXR_OK)
                {
                    pLang2 = (char*)(pBufLang2->GetBuffer());
                    HX_ASSERT(pLang2);
                }

                if ((pLang1 && pLang2 && !(strcasecmp(pLang1, pLang2))) || (!pLang1 && !pLang2 ))
                {
                    if (pSwitchVerifier->IsSwitchable(pSwitchGrpStream[j], ppValues[i]))
                    {
                        bSwitchGrpFound = TRUE;
                        ppValues[i]->SetPropertyULONG32("SwitchGroupID", j + 1);
                        break;
                    }
                }                       

                HX_RELEASE(pBufLang1);
                HX_RELEASE(pBufLang2);
            }
        }

        if (!bSwitchGrpFound)
        {
            pSwitchGrpStream[nSwitchGrpCnt] = ppValues[i];
            StreamGroupNum[nSwitchGrpCnt] = ulCurStreamGrpNum;
            ppValues[i]->SetPropertyULONG32("SwitchGroupID", ++nSwitchGrpCnt);
        }    
    }

bail:
    HX_VECTOR_DELETE(pSwitchGrpStream);
    HX_VECTOR_DELETE(StreamGroupNum);
    HX_RELEASE(pSwitchVerifier);

    return res;
}
#endif


//TODO: adding bandwidth partitions
HX_RESULT 
CHXAltGroupUtil::CreateASMRuleBook(IHXCommonClassFactory* pCCF, UINT16 nValues, 
                  REF(IHXValues**) ppValues)
{
    HX_RESULT res = HXR_OK;

    if (nValues == 0)
    {
        return HXR_FAIL;
    }

    HXBOOL bIsMultiRate = FALSE;
    UINT32 ulSwitchGrp = 0;
    if (HXR_OK == ppValues[0]->GetPropertyULONG32("SwitchGroupID", ulSwitchGrp))
    {
        bIsMultiRate = TRUE;
    }

    UINT32 j = 0;
    UINT32 nCnt = 0;

    if (bIsMultiRate)
    {
        HXBOOL bSwitchGrpFound;

        //Strting with the first Switch Group
        UINT32 ulSwitchGrpCur = 1;
    
        //For each S/W grp
        do
    {
            //Determines when to stop the loop
            bSwitchGrpFound = FALSE;

            //Create a IHXValues** to store the streams belonging to this S/W Grp
            //nValues being the max streams a s/w grp can have
            IHXValues** ppStreams = new IHXValues*[nValues]; 
            nCnt = 0;

            for (j = 0; j < nValues; j++)
	{
                //Clearing the value
                ulSwitchGrp = 0;

                if (HXR_OK == ppValues[j]->GetPropertyULONG32("SwitchGroupID", ulSwitchGrp))
                {
                    if (ulSwitchGrp == ulSwitchGrpCur)
                    {
                        bSwitchGrpFound = TRUE;
                        ppStreams[nCnt] = ppValues[j];
                        ppStreams[nCnt]->AddRef();
                        nCnt++;
                    }
        }
        else
        {
                    res = HXR_UNEXPECTED;
                    goto clean;
                }
            }
            
            if (bSwitchGrpFound)
            {
                //Once we got the streams of "ulSwitchGrpCur" s/w grp we have to sort them acc to B/W
                // and add the ASMRulebook w/BW partition
                res = SortStreams(ppStreams, nCnt);
                
                //pStreams are now sorted. Now Add ASMRuleBook in each of these streams
                res = AddASMRuleBook(ppStreams, nCnt, pCCF);
            }

clean:
            //Release Objects
            //Create a IHXValues** to store the streams of this S/W Grp
            for (j = 0; j < nCnt; j++)
            {
                HX_RELEASE(ppStreams[j]);
        }
            HX_VECTOR_DELETE(ppStreams);

            //Next SwitchGrp
            ulSwitchGrpCur++;
        }while(bSwitchGrpFound && res == HXR_OK); //Loop till we have covered all s/w grps  
    }
    else
    {
        //For single rate streams each stream is considered as a separate s/w grp
        //So set the ASMRuleBook in each stream here itself.
        //Cannot reuse the AddASMRuleBook() because it takes IHXValues** as input
        //where as here we need only IHXValues*
        UINT32 i = 0;
        UINT32 ulAvgBW = 0;
        UINT32 ulBaseRule = 0;
        for (i = 0; i < nValues; i++)
        {
            ppValues[i]->GetPropertyULONG32("AvgBitRate", ulAvgBW);

        //Form ASM RuleBook
        char pTemp[1024];
        UINT32 ulSize = 0;

        ulSize = SafeSprintf(pTemp, 1024,
            "Marker=0,AverageBandwidth=%u,TimestampDelivery=TRUE,InterDepend=%u;"
            "Marker=1,AverageBandwidth=0,TimestampDelivery=TRUE,InterDepend=%u;",
            (unsigned int)ulAvgBW, (unsigned int)ulBaseRule + 1,(unsigned int)ulBaseRule);

            //Set Base Rule
            ppValues[i]->SetPropertyULONG32("BaseRule", ulBaseRule);

        IHXBuffer* pASMRuleBook = NULL;
        res = pCCF->CreateInstance(CLSID_IHXBuffer, (void**) &pASMRuleBook);
        if (res != HXR_OK)
        {
            res = HXR_FAIL;
        }

            pASMRuleBook->Set((UCHAR*)pTemp, strlen(pTemp)+ 1);
            ppValues[i]->SetPropertyCString("ASMRuleBook", pASMRuleBook);

            HX_RELEASE(pASMRuleBook);
        }
    }
    return res;
}

HX_RESULT
CHXAltGroupUtil::SortStreams ( REF(IHXValues**) ppHdrs, UINT32 nVal)
{
    HX_RESULT res = HXR_OK;

    //Bubble Sort
    UINT16 i, j = 0;
    IHXValues* pTemp = NULL;
    UINT32 ulAvgBWCurrent = 0;
    UINT32 ulAvgBWNext = 0;
    for (i = 0 ; i < (nVal - 1); i++)
    {
        for (j = 0; j < (nVal - 1 -i); j++)
        {
            if (HXR_OK != ppHdrs[j]->GetPropertyULONG32("AvgBitRate", ulAvgBWCurrent))
            {
                res = HXR_UNEXPECTED;
                goto clean;
            }
            if (HXR_OK != ppHdrs[j+1]->GetPropertyULONG32("AvgBitRate", ulAvgBWNext))
            {
                res = HXR_UNEXPECTED;
                goto clean;
            }
            if (ulAvgBWNext < ulAvgBWCurrent)
            {
                pTemp = ppHdrs[j];
                ppHdrs[j] = ppHdrs[j+1];
                ppHdrs[j+1] = pTemp;
            }
        }
    }
clean:
    return res;
}

HX_RESULT
CHXAltGroupUtil::AddASMRuleBook(REF(IHXValues**) ppHdrs, UINT32 nVal, IHXCommonClassFactory* pCCF)
{
    HX_RESULT res = HXR_OK;

    if (nVal == 0)
    {
        return HXR_FAIL;
    }

    IHXBuffer* pASMRuleBook = NULL;

    UINT32 ulMaxBookSize = MAX_RULE_BOOK * nVal;
    char* pRuleBook = new char [ulMaxBookSize];
    char* pRuleBookWriter = pRuleBook;
    
    //To store the base rule number n Bandwidth
    UINT32 ulBaseRule = 0; //BaseRuleIndex
    UINT32 ulBandwidth, ulBandwidthTo = 0;
    UINT uIdx = 0;
    UINT i = 0;
    
    if (nVal > 1)
    {
        do
        {
            if (HXR_OK != ppHdrs[uIdx]->GetPropertyULONG32("AvgBitRate",ulBandwidth))
            {
                res = HXR_UNEXPECTED;
                goto clean;
            }

            if (uIdx == 0)
            {
                // first entry
                ppHdrs[uIdx + 1]->GetPropertyULONG32("AvgBitRate",ulBandwidthTo);

                pRuleBookWriter += SafeSprintf(pRuleBookWriter, 
                    ulMaxBookSize - (pRuleBookWriter - pRuleBook), 
                    "#($Bandwidth < %d),Marker=0,AverageBandwidth=%d,TimeStampDelivery=TRUE,InterDepend=%u;"
                    "#($Bandwidth < %d),Marker=1,AverageBandwidth=0,TimeStampDelivery=TRUE,InterDepend=%u;",
                    ulBandwidthTo,
                    ulBandwidth,
                    (unsigned int)ulBaseRule + 1,
                    ulBandwidthTo,                  
                    (unsigned int)ulBaseRule);
            }
            else if ((uIdx + 1) < nVal)
            {
                // mid entry (not first and not last)
                ppHdrs[uIdx + 1]->GetPropertyULONG32("AvgBitRate",ulBandwidthTo);

                pRuleBookWriter += SafeSprintf(pRuleBookWriter, 
                    ulMaxBookSize - (pRuleBookWriter - pRuleBook), 
                    "#($Bandwidth >= %d) && ($Bandwidth < %d),Marker=0,AverageBandwidth=%d,TimeStampDelivery=TRUE,InterDepend=%u;"
                    "#($Bandwidth >= %d) && ($Bandwidth < %d),Marker=1,AverageBandwidth=0,TimeStampDelivery=TRUE,InterDepend=%u;",
                    ulBandwidth,
                    ulBandwidthTo,                  
                    ulBandwidth,
                    (unsigned int)ulBaseRule + 1,
                    ulBandwidth,
                    ulBandwidthTo,
                    (unsigned int)ulBaseRule);
        }
        else
        {       
                // last entry
                pRuleBookWriter += SafeSprintf(pRuleBookWriter, 
                    ulMaxBookSize - (pRuleBookWriter - pRuleBook), 
                    "#($Bandwidth >= %d),Marker=0,AverageBandwidth=%d,TimeStampDelivery=TRUE,InterDepend=%u;"
                    "#($Bandwidth >= %d),Marker=1,AverageBandwidth=0,TimeStampDelivery=TRUE,InterDepend=%u;",
                    ulBandwidth,                    
                    ulBandwidth,
                    (unsigned int)ulBaseRule + 1,
                    ulBandwidth,                    
                    (unsigned int)ulBaseRule);
        }
            ppHdrs[uIdx]->SetPropertyULONG32("BaseRule", ulBaseRule);
            ulBaseRule += 2; //Incr Base Rule by 2

            uIdx++;
        } while (uIdx < nVal);
    }
    else
    {
        if (HXR_OK != ppHdrs[uIdx]->GetPropertyULONG32("AvgBitRate",ulBandwidth))
        {
            res = HXR_UNEXPECTED;
            goto clean;
   }     

        pRuleBookWriter += SafeSprintf(pRuleBookWriter, 
            ulMaxBookSize - (pRuleBookWriter - pRuleBook), 
            "Marker=0,AverageBandwidth=%d,TimeStampDelivery=TRUE,Interdepend=%u;"
            "Marker=1,AverageBandwidth=0,TimeStampDelivery=TRUE,Interdepend=%u;",
            ulBandwidth,
            (unsigned int)ulBaseRule + 1,
            (unsigned int)ulBaseRule);

        ppHdrs[uIdx]->SetPropertyULONG32("BaseRule", ulBaseRule);
    }

    //Now we store the ASMRuleBook in each stream
    for (i = 0; i < nVal; i++)
    {

        res = pCCF->CreateInstance(CLSID_IHXBuffer, (void**) &pASMRuleBook);
           
        if (res != HXR_OK)
        {
            res = HXR_FAIL;
            goto clean;
    }

        pASMRuleBook->Set((UCHAR*)pRuleBook, strlen(pRuleBook)+ 1);
        ppHdrs[i]->SetPropertyCString("ASMRuleBook", pASMRuleBook);
           
        HX_RELEASE(pASMRuleBook);
}

clean:
    HX_DELETE(pRuleBook);
    HX_RELEASE(pASMRuleBook);

    return res;
}

HX_RESULT 
CHXAltGroupUtil::SetTrackID(UINT16 nValues, REF(IHXValues**) ppValues,
                              REF (IHXValues*) pFileHeader)
{
    HX_RESULT res = HXR_OK;
    UINT32 ulStreamNum = 0;
    UINT32 ulTrackID = 0;

    for (UINT32 i = 0; i < nValues; i++)
    {
        if (HXR_OK == ppValues[i]->GetPropertyULONG32("TrackID", ulTrackID))
        {
            //if found track Id..break
            break;
        }
        else
        {
            if (HXR_OK == ppValues[i]->GetPropertyULONG32("StreamNumber", ulStreamNum))
            {
                ppValues[i]->SetPropertyULONG32("TrackID", ulStreamNum + 1);
    }
            else
            {
                return HXR_UNEXPECTED;
            }
	}
    }

    return res;
}
