#include "alttesthlpr.h"
#include "netbyte.h"

#include "ut_param_util.h"
#include "hx_ut_debug.h"

CHXAltHdrTestHelper::CHXAltHdrTestHelper() :
    m_pCCF(NULL),
    m_uHeaderCount(0),
    m_ppHeaders(NULL)
{}

CHXAltHdrTestHelper::~CHXAltHdrTestHelper()
{
    Clear();
    HX_RELEASE(m_pCCF);
}

HX_RESULT 
CHXAltHdrTestHelper::Init(IUnknown* pContext)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pContext)
    {
        HX_RELEASE(m_pCCF);
        res = pContext->QueryInterface(IID_IHXCommonClassFactory,
                                       (void**)&m_pCCF);
    }

    return res;
}

void CHXAltHdrTestHelper::GetCommandInfo(UTVector<HLXUnitTestCmdInfo*>& cmds)
{
    int index = cmds.Nelements();

    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltHdrTestHelper>(this, 
                                                        "AHTH::Clear",
                                                        &CHXAltHdrTestHelper::HandleClearCmd,
                                                        1);
    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltHdrTestHelper>(this, 
                                                        "AHTH::AddAltGroupID",
                                                        &CHXAltHdrTestHelper::HandleAddAltGroupIDCmd,
                                                        5);
    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltHdrTestHelper>(this, 
                                                        "AHTH::AddULONG32",
                                                        &CHXAltHdrTestHelper::HandleAddULONG32Cmd,
                                                        4);
    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltHdrTestHelper>(this, 
                                                        "AHTH::AddCString",
                                                        &CHXAltHdrTestHelper::HandleAddCStringCmd,
                                                        4);
    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltHdrTestHelper>(this, 
                                                        "AHTH::AddAltULONG32",
                                                        &CHXAltHdrTestHelper::HandleAddAltULONG32Cmd,
                                                        5);
    cmds.Resize(index + 1);
    cmds[index++] = 
	new HLXUnitTestCmdInfoDisp<CHXAltHdrTestHelper>(this, 
                                                        "AHTH::AddAltCString",
                                                        &CHXAltHdrTestHelper::HandleAddAltCStringCmd,
                                                        5);
}

void CHXAltHdrTestHelper::Clear()
{
    if (m_ppHeaders)
    {
        for (UINT32 i = 0; i < m_uHeaderCount; i++)
        {
            HX_RELEASE(m_ppHeaders[i]);
        }

        HX_VECTOR_DELETE(m_ppHeaders);

        m_uHeaderCount = 0;
    }
}

HX_RESULT 
CHXAltHdrTestHelper::SetHeaders(UINT32 uHeaderCount, IHXValues** ppHeaders)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (uHeaderCount && ppHeaders)
    {
        IHXValues** pNew = new IHXValues*[uHeaderCount];

        if (pNew)
        {
            for (UINT32 i = 0; i < m_uHeaderCount; i++)
            {
                HX_RELEASE(m_ppHeaders[i]);
            }

            delete [] m_ppHeaders;

            for (UINT32 j = 0; j < uHeaderCount; j++)
            {
                pNew[j] = ppHeaders[j];

                if (pNew[j])
                {
                    pNew[j]->AddRef();
                }
            }

            m_ppHeaders = pNew;
            m_uHeaderCount = uHeaderCount;
            res = HXR_OK;
        }
        else
        {
            res = HXR_OUTOFMEMORY;
        }
    }

    return res;
}
    
UINT32 CHXAltHdrTestHelper::HeaderCount() const
{
    return m_uHeaderCount;
}

IHXValues** CHXAltHdrTestHelper::Headers() const
{
    return m_ppHeaders;
}

HX_RESULT 
CHXAltHdrTestHelper::AddAltGroupID(const char* pGroupType,
                                   const char* pGroupValue,
                                   UINT32 uGroupValueIndex,
                                   UINT32 uAltID)
{
    IHXList* pGroupList = NULL;
    HX_RESULT res = getGroupList(pGroupType, pGroupValue, 
                                 pGroupList);

    if (HXR_OK == res)
    {
        if (uGroupValueIndex < pGroupList->GetCount())
        {
            IHXListIterator* pItr = pGroupList->Begin();

            if (pItr)
            {
                // find the specified buffer and add the altid
                UINT32 uIndex = 0;
                HXBOOL bDone = FALSE;
                while(!bDone && (uIndex != uGroupValueIndex))
                {
                    if (pItr->MoveNext())
                    {
                        uIndex++;
                    }
                    else
                    {
                        bDone = TRUE;
                    }
                }

                if (uIndex == uGroupValueIndex)
                {
                    IUnknown* pUnk = pItr->GetItem();
                    IHXBuffer* pOldBuf = NULL;
                    
                    if (pUnk && m_pCCF &&
                        (HXR_OK == pUnk->QueryInterface(IID_IHXBuffer,
                                                        (void**)&pOldBuf)))
                    {
                        // We have to copy the data into a new buffer
                        // because you can't modify a buffer with a 
                        // refcount > 1
                        IHXBuffer* pIDBuf = NULL;
                        res = m_pCCF->CreateInstance(CLSID_IHXBuffer,
                                                     (void**)&pIDBuf);
                        
                        if (HXR_OK == res) 
                        {
                            res = pIDBuf->SetSize(pOldBuf->GetSize() + 4);

                            if (HXR_OK == res)
                            {
                                memcpy(pIDBuf->GetBuffer(),
                                       pOldBuf->GetBuffer(),
                                       pOldBuf->GetSize());

                                UINT32* pID = (UINT32*)(pIDBuf->GetBuffer() +
                                                        pOldBuf->GetSize());
                                *pID = DwToNet(uAltID);
                                
                                res = pGroupList->Replace(pItr, pIDBuf);
                            }
                        }
                        HX_RELEASE(pIDBuf);
                    }

                    HX_RELEASE(pOldBuf);
                    HX_RELEASE(pUnk);
                }
                else
                {
                    res = HXR_INVALID_PARAMETER;
                }
            }
            else
            {
                res = HXR_UNEXPECTED;
            }
        }
        else if (uGroupValueIndex == pGroupList->GetCount())
        {
            // We are adding a new set of AltIDs to the list
            if (m_pCCF)
            {
                IHXBuffer* pIDBuf = NULL;
                res = m_pCCF->CreateInstance(CLSID_IHXBuffer,
                                             (void**)&pIDBuf);

                if (HXR_OK == res)
                {
                    UINT32 ulVal = DwToNet(uAltID);
                    res = pIDBuf->Set((UCHAR*)&ulVal, 4);
                    
                    if (HXR_OK == res)
                    {
                        res = pGroupList->InsertTail(pIDBuf);
                    }
                }

                HX_RELEASE(pIDBuf);
            }
        }
        else
        {
            // You are not allowed to go more than 1 past
            // the end
            res = HXR_INVALID_PARAMETER;
        }
    }
    HX_RELEASE(pGroupList);

    return res;
}


HX_RESULT 
CHXAltHdrTestHelper::AddULONG32(UINT32 uHdrIdx, const char* pKey,
                                ULONG32 ulValue)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;
    
    if (pKey && (HXR_OK == createHeaders(uHdrIdx + 1)))
    {
        res = m_ppHeaders[uHdrIdx]->SetPropertyULONG32(pKey, ulValue);
    }

    return res;
}

HX_RESULT 
CHXAltHdrTestHelper::AddCString(UINT32 uHdrIdx, const char* pKey,
                                const char* pValue)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;
    
    if (pKey && pValue && m_pCCF &&
        (HXR_OK == createHeaders(uHdrIdx + 1)))
    {
        IHXBuffer* pBuf = NULL;

        res = m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pBuf);

        if (HXR_OK == res)
        {
            res = pBuf->Set((const UCHAR*)pValue, strlen(pValue) + 1);

            if (HXR_OK == res)
            {
                res = m_ppHeaders[uHdrIdx]->SetPropertyCString(pKey, 
                                                                      pBuf);
            }
        }

        HX_RELEASE(pBuf);
    }

    return res;
}

HX_RESULT 
CHXAltHdrTestHelper::AddAltULONG32(UINT32 uHdrIdx, UINT32 uAltID,
                                   const char* pKey, ULONG32 ulValue)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;
    
    if (pKey && (HXR_OK == createHeaders(uHdrIdx + 1)))
    {
        IHXValues2* pAltValues = NULL;
        res = getAltValues(uHdrIdx, uAltID, pAltValues);

        if (HXR_OK == res)
        {
            res = pAltValues->SetPropertyULONG32(pKey, ulValue);
        }
        HX_RELEASE(pAltValues);
    }

    return res;
}

HX_RESULT 
CHXAltHdrTestHelper::AddAltCString(UINT32 uHdrIdx, UINT32 uAltID, 
                                   const char* pKey, const char* pValue)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;
    
    if (pKey && pValue && m_pCCF &&
        (HXR_OK == createHeaders(uHdrIdx + 1)))
    {
        IHXValues2* pAltValues = NULL;
        res = getAltValues(uHdrIdx, uAltID, pAltValues);

        if (HXR_OK == res)
        {
            IHXBuffer* pBuf = NULL;
            
            res = m_pCCF->CreateInstance(CLSID_IHXBuffer, (void**)&pBuf);

            if (HXR_OK == res)
            {
                res = pBuf->Set((const UCHAR*)pValue, strlen(pValue) + 1);
                
                if (HXR_OK == res)
                {
                    res = pAltValues->SetPropertyCString(pKey, pBuf);
                }
            }

            HX_RELEASE(pBuf);
        }
        HX_RELEASE(pAltValues);
    }

    return res;
}

bool 
CHXAltHdrTestHelper::HandleClearCmd(const UTVector<UTString>& info)
{
    Clear();
    return true;
}

bool 
CHXAltHdrTestHelper::HandleAddAltGroupIDCmd(const UTVector<UTString>& info)
{
    bool ret = false;

    unsigned int uGroupValueIndex = 0;
    unsigned int uAltID = 0;

    if (!UTParamUtil::GetUInt(info[3], uGroupValueIndex) ||
        !UTParamUtil::GetUInt(info[4], uAltID))
    {
        DPRINTF(D_ERROR, ("CHXAltHdrTestHelper::HandleAddAltGroupIDCmd : failed to convert parameter\n"));
    }
    else
    {
        HX_RESULT res = AddAltGroupID(info[1], info[2], uGroupValueIndex, 
                                      uAltID); 

        if (HXR_OK != res)
        {
            DPRINTF(D_ERROR, ("CHXAltHdrTestHelper::HandleAddAltGroupIDCmd : unexpected error %08x\n", res));
        }
        else
        {
            ret = true;
        }
    }

    return ret;
}

bool 
CHXAltHdrTestHelper::HandleAddULONG32Cmd(const UTVector<UTString>& info)
{
    bool ret = false;
    
    unsigned int uHdrIdx;
    unsigned int uValue;

    if (!UTParamUtil::GetUInt(info[1], uHdrIdx) ||
        !UTParamUtil::GetUInt(info[3], uValue))
    {
        DPRINTF(D_ERROR, ("CHXAltHdrTestHelper::HandleAddULONG32Cmd : failed to convert parameter\n"));
    }
    else
    {
        HX_RESULT res = AddULONG32(uHdrIdx, info[2], uValue); 

        if (HXR_OK != res)
        {
            DPRINTF(D_ERROR, ("CHXAltHdrTestHelper::HandleAddHdrULONG32Cmd : unexpected error %08x\n", res));
        }
        else
        {
            ret = true;
        }
    }

    return ret;
}

bool 
CHXAltHdrTestHelper::HandleAddCStringCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    
    unsigned int uHdrIdx;

    if (!UTParamUtil::GetUInt(info[1], uHdrIdx))
    {
        DPRINTF(D_ERROR, ("CHXAltHdrTestHelper::HandleAddCStringCmd : failed to convert parameter\n"));
    }
    else
    {
        HX_RESULT res = AddCString(uHdrIdx, info[2], info[3]); 

        if (HXR_OK != res)
        {
            DPRINTF(D_ERROR, ("CHXAltHdrTestHelper::HandleAddCStringCmd : unexpected error %08x\n", res));
        }
        else
        {
            ret = true;
        }
    }

    return ret;
}

bool 
CHXAltHdrTestHelper::HandleAddAltULONG32Cmd(const UTVector<UTString>& info)
{
    bool ret = false;
    
    unsigned int uHdrIdx;
    unsigned int uAltID;
    unsigned int uValue;

    if (!UTParamUtil::GetUInt(info[1], uHdrIdx) ||
        !UTParamUtil::GetUInt(info[2], uAltID) ||
        !UTParamUtil::GetUInt(info[4], uValue))
    {
        DPRINTF(D_ERROR, ("CHXAltHdrTestHelper::HandleAddAltULONG32Cmd : failed to convert parameter\n"));
    }
    else
    {
        HX_RESULT res = AddAltULONG32(uHdrIdx, uAltID, info[3], uValue); 

        if (HXR_OK != res)
        {
            DPRINTF(D_ERROR, ("CHXAltHdrTestHelper::HandleAddAltULONG32Cmd : unexpected error %08x\n", res));
        }
        else
        {
            ret = true;
        }
    }

    return ret;
}

bool 
CHXAltHdrTestHelper::HandleAddAltCStringCmd(const UTVector<UTString>& info)
{
    bool ret = false;
    
    unsigned int uHdrIdx;
    unsigned int uAltID;

    if (!UTParamUtil::GetUInt(info[1], uHdrIdx) ||
        !UTParamUtil::GetUInt(info[2], uAltID))
    {
        DPRINTF(D_ERROR, ("CHXAltHdrTestHelper::HandleAddAltCStringCmd : failed to convert parameter\n"));
    }
    else
    {
        HX_RESULT res = AddAltCString(uHdrIdx, uAltID, info[3], info[4]); 

        if (HXR_OK != res)
        {
            DPRINTF(D_ERROR, ("CHXAltHdrTestHelper::HandleAddAltCStringCmd : unexpected error %08x\n", res));
        }
        else
        {
            ret = true;
        }
    }

    return ret;
}

HX_RESULT 
CHXAltHdrTestHelper::createHeaders(UINT32 uHeaderCount)
{
    HX_RESULT res = HXR_OK;

    if (uHeaderCount == m_uHeaderCount + 1)
    {
        // Handle the case where the header count
        // increases by 1

        IHXValues** ppOld = m_ppHeaders;
        UINT32 uOldCount = m_uHeaderCount;
        IHXValues** ppNew = new IHXValues*[uHeaderCount];

        if (ppNew)
        {
            if (ppOld)
            {
                for (UINT32 i = 0; i < uOldCount; i++)
                {
                    ppNew[i] = ppOld[i];
                }                
            }
            
            if (m_pCCF)
            {
                IHXValues2* pHdr2 = NULL;
                res = m_pCCF->CreateInstance(CLSID_IHXValues2,
                                             (void**)&pHdr2);

                if (HXR_OK == res)
                {
                    // transfering ownership here.
                    ppNew[uOldCount] = pHdr2;

                    m_ppHeaders = ppNew;
                    m_uHeaderCount = uHeaderCount;

                    HX_VECTOR_DELETE(ppOld);
                }
            }
            else
            {
                res = HXR_UNEXPECTED;
            }

            if (HXR_OK != res)
            {
                HX_VECTOR_DELETE(ppNew);
            }
        }
        else
        {
            res = HXR_OUTOFMEMORY;
        }
    }
    else if (uHeaderCount > m_uHeaderCount + 1)
    {
        // You are not allowed to grow by more than
        // one header at a time
        res = HXR_INVALID_PARAMETER;
    }

    return res;
}

HX_RESULT 
CHXAltHdrTestHelper::getGroupList(const char* pGroupType,
                                  const char* pGroupValue,
                                  REF(IHXList*) pList)
{
    HX_RESULT res = createHeaders(1);
    
    if (!pGroupType || !pGroupValue)
    {
        res = HXR_INVALID_PARAMETER;
    }
    else if (HXR_OK == res)
    {
        IHXValues2* pGroups = NULL;
        res = getIHXValues2(m_ppHeaders[0], "Alt-Group", pGroups);

        if (HXR_OK == res)
        {
            IHXValues2* pGroupValues = NULL;

            res = getIHXValues2(pGroups, pGroupType, pGroupValues);

            if (HXR_OK == res)
            {
                res = getIHXList(pGroupValues, pGroupValue, pList);
            }

            HX_RELEASE(pGroupValues);
        }

        HX_RELEASE(pGroups);
    }

    return res;
}

HX_RESULT 
CHXAltHdrTestHelper::getIHXValues2(IHXValues* pHdr, const char* pKey,
                                   REF(IHXValues2*) pVal)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pHdr && pKey)
    {
        IHXValues2* pHdr2 = NULL;

        res = pHdr->QueryInterface(IID_IHXValues2, (void**)&pHdr2);

        if (HXR_OK == res)
        {
            IUnknown* pUnk = NULL;
            
            res = pHdr2->GetPropertyObject(pKey, pUnk);
        
            if (HXR_OK == res)
            {
                res = pUnk->QueryInterface(IID_IHXValues2, (void**)&pVal);
            }
            else if (m_pCCF)
            {
                res = m_pCCF->CreateInstance(CLSID_IHXValues2,
                                             (void**)&pVal);

                if (HXR_OK == res)
                {
                    res = pHdr2->SetPropertyObject(pKey, pVal);
                }
                
                if (HXR_OK != res)
                {
                    HX_RELEASE(pVal);
                }
            }

            HX_RELEASE(pUnk);
        }
        HX_RELEASE(pHdr2);
    }

    return res;
}

HX_RESULT 
CHXAltHdrTestHelper::getIHXList(IHXValues* pHdr, const char* pKey,
                     REF(IHXList*) pVal)
{
    HX_RESULT res = HXR_INVALID_PARAMETER;

    if (pHdr && pKey)
    {
        IHXValues2* pHdr2 = NULL;

        res = pHdr->QueryInterface(IID_IHXValues2, (void**)&pHdr2);

        if (HXR_OK == res)
        {
            IUnknown* pUnk = NULL;
            
            res = pHdr2->GetPropertyObject(pKey, pUnk);
        
            if (HXR_OK == res)
            {
                res = pUnk->QueryInterface(IID_IHXList, (void**)&pVal);
            }
            else if (m_pCCF)
            {
                res = m_pCCF->CreateInstance(CLSID_IHXList,
                                             (void**)&pVal);

                if (HXR_OK == res)
                {
                    res = pHdr2->SetPropertyObject(pKey, pVal);
                }
                
                if (HXR_OK != res)
                {
                    HX_RELEASE(pVal);
                }
            }

            HX_RELEASE(pUnk);
        }
        HX_RELEASE(pHdr2);
    }

    return res;
}

HX_RESULT 
CHXAltHdrTestHelper::getAltValues(UINT32 uHdrIdx, UINT32 uAltID,
                                  REF(IHXValues2*)pAltValues)
{
    IHXValues2* pAlt = NULL;
    HX_RESULT res = HXR_INVALID_PARAMETER;
    
    if (uHdrIdx > 0)
    {
        // Only try to get Alt from stream headers.

        res = getIHXValues2(m_ppHeaders[uHdrIdx], "Alt", pAlt);

        if (HXR_OK == res)
        {
            char altKey[11];
            sprintf(altKey, "%u", uAltID);
            
            res = getIHXValues2(pAlt, altKey, pAltValues);
        }
        HX_RELEASE(pAlt);
    }

    return res;
}
