#ifndef CHXCHARSTACK_H
#define CHXCHARSTACK_H

#include "hxcom.h"
#include "ihxpckts.h" // IHXBuffer
#include "hxccf.h" // IHXCommonClassFactory

class CHXCharStack
{
public:
    CHXCharStack(IUnknown* pUnk);
    ~CHXCharStack();

    HX_RESULT AddChar(char ch);
    HX_RESULT Finish(REF(IHXBuffer*) pBuf);
    void Reset();

private:
    ULONG32 m_ulOffset;
    IHXBuffer* m_pBuffer;
    IHXCommonClassFactory* m_pCCF;
};

#endif /* CHXCHARSTACK_H */
