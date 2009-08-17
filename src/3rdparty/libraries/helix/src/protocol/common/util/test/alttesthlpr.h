#ifndef ALTTESTHLPR_H
#define ALTTESTHLPR_H

#include "hxcom.h"
#include "ihxpckts.h"
#include "hxccf.h"
#include "ihxlist.h"
#include "ut_vector.h"
#include "ut_string.h"
#include "hx_ut_cmd_info.h"

class CHXAltHdrTestHelper
{
public:
    CHXAltHdrTestHelper();
    ~CHXAltHdrTestHelper();

    HX_RESULT Init(IUnknown* pContext);

    void GetCommandInfo(UTVector<HLXUnitTestCmdInfo*>& cmds);

    void Clear();

    HX_RESULT SetHeaders(UINT32 uHeaderCount, IHXValues** ppHeaders);
    
    UINT32 HeaderCount() const;
    IHXValues** Headers() const;

    HX_RESULT AddAltGroupID(const char* pGroupType,
                            const char* pGroupValue,
                            UINT32 uGroupValueIndex,
                            UINT32 uAltID);

    HX_RESULT AddULONG32(UINT32 uHdrIdx, const char* pKey,
                         ULONG32 ulValue);
    HX_RESULT AddCString(UINT32 uHdrIdx, const char* pKey,
                               const char* pValue);
    HX_RESULT AddAltULONG32(UINT32 uStreamIdx, UINT32 uAltID,
                            const char* pKey, ULONG32 ulValue);
    HX_RESULT AddAltCString(UINT32 uStreamIdx, UINT32 uAltID, 
                            const char* pKey, const char* pValue);

private:
    bool HandleClearCmd(const UTVector<UTString>& info);
    bool HandleAddAltGroupIDCmd(const UTVector<UTString>& info);
    bool HandleAddULONG32Cmd(const UTVector<UTString>& info);
    bool HandleAddCStringCmd(const UTVector<UTString>& info);
    bool HandleAddAltULONG32Cmd(const UTVector<UTString>& info);
    bool HandleAddAltCStringCmd(const UTVector<UTString>& info);

    HX_RESULT createHeaders(UINT32 uHeaderCount);
    HX_RESULT getGroupList(const char* pGroupType,
                           const char* pGroupValue,
                           REF(IHXList*) pList);

    HX_RESULT getIHXValues2(IHXValues* pHdr, const char* pKey,
                            REF(IHXValues2*) pVal);
    HX_RESULT getIHXList(IHXValues* pHdr, const char* pKey,
                         REF(IHXList*) pVal);
    HX_RESULT getAltValues(UINT32 uStreamIdx, UINT32 uAltID,
                           REF(IHXValues2*)pAltValues);
    
    IHXCommonClassFactory* m_pCCF;

    UINT32 m_uHeaderCount;
    IHXValues** m_ppHeaders;
};

#endif /* ALTTESTHLPR_H */
