#include "chxcharstack.h"
#include "hlxclib/string.h"

CHXCharStack::CHXCharStack(IUnknown* pUnk) :
    m_ulOffset(0),
    m_pBuffer(0),
    m_pCCF(0)
{
    if (pUnk &&
	(HXR_OK == pUnk->QueryInterface(IID_IHXCommonClassFactory,
					(void**)&m_pCCF)))
    {
	m_pCCF->CreateInstance(IID_IHXBuffer, (void**)&m_pBuffer);
    }
}

CHXCharStack::~CHXCharStack()
{
    HX_RELEASE(m_pBuffer);
    HX_RELEASE(m_pCCF);
}

HX_RESULT CHXCharStack::AddChar(char ch)
{
    HX_RESULT res = HXR_UNEXPECTED;

    if (m_pBuffer)
    {
	ULONG32 ulCurrentSize = m_pBuffer->GetSize();

	if (m_ulOffset >= ulCurrentSize)
	{
	    ULONG32 ulNewSize = (ulCurrentSize) ? 2 * ulCurrentSize : 1;
	    res = m_pBuffer->SetSize(ulNewSize);
	}
	else
	{
	    res = HXR_OK;
	}
	
	if (HXR_OK == res)
	{
	    ((char*)m_pBuffer->GetBuffer())[m_ulOffset++] = ch;
	}
    }

    return res;
}

HX_RESULT CHXCharStack::Finish(REF(IHXBuffer*) pBuf)
{
    HX_RESULT res = HXR_UNEXPECTED;

    pBuf = 0;

    if (m_pBuffer && m_pCCF)
    {
	res = AddChar('\0');

	if (HXR_OK == res)
	{
	    ULONG32 len = strlen((char*)m_pBuffer->GetBuffer()) + 1;
	    
	    res = m_pCCF->CreateInstance(IID_IHXBuffer, (void**)&pBuf);

	    if ((HXR_OK == res) && (HXR_OK == (res = pBuf->SetSize(len))))
	    {
		strcpy((char*)pBuf->GetBuffer(),(char*)m_pBuffer->GetBuffer());
	    }
	    else
	    {
		HX_RELEASE(pBuf);
	    }
	}
    }

    return res;
}

void CHXCharStack::Reset()
{
    m_ulOffset = 0;
}

