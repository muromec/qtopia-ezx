#include "ihxpckts.h"
#include "hxbuffer.h"
#include "pckunpck.h"

#include "recognizer.h"

#if !defined (_SYMBIAN)
HX_RESULT CHXFileRecognizer::GetMimeType(const char* pFileName, IHXBuffer* pBuffer, REF(IHXBuffer*) pMimeType)
{
    if (IsSDPFile(pBuffer))
    {
	if (HXR_OK == CreateBufferCCF(pMimeType, m_pContext))
	{
            int len = strlen("application/sdp");
	    pMimeType->Set((const UCHAR*)"application/sdp", len + 1);
	    ((char*)pMimeType->GetBuffer())[len] = '\0';
	    return HXR_OK;
	}
    }

    return HXR_FAILED;
}
#endif

HXBOOL
CHXFileRecognizer::IsSDPFile(IHXBuffer* pBuffer)
{
    HXBOOL bResult = FALSE;

    if (pBuffer && pBuffer->GetSize())
    {
        if (0 == strncmp((const char*)pBuffer->GetBuffer(), "v=", 2))
        {
            bResult = TRUE;
        }
    }

    return bResult;
}

// length of buffer read from file and passed to recognizer
static const int KRecogLength = 512;


CHXFileRecognizer::CHXFileRecognizer(IUnknown* pContext)
    : m_lRefCount(0),
      m_pResponse(NULL),
      m_pFile(NULL),
      m_pBuffer(NULL),
      m_bGetMimeTypeDone(FALSE),
      m_pContext(pContext)
{
    HX_ADDREF(m_pContext);
}

CHXFileRecognizer::~CHXFileRecognizer()
{
    HX_RELEASE(m_pBuffer);
    HX_RELEASE(m_pResponse);
    HX_RELEASE(m_pFile);
    HX_RELEASE(m_pContext);
}

STDMETHODIMP CHXFileRecognizer::QueryInterface(REFIID riid, void** ppvObj)
{
    HX_RESULT ret = HXR_OK;

    if (IsEqualIID(riid, IID_IHXFileRecognizer))
    {
	AddRef();
	*ppvObj = (IHXFileRecognizer*)this;
    }
    else if (IsEqualIID(riid, IID_IHXFileResponse))
    {
	AddRef();
	*ppvObj = (IHXFileResponse*)this;
    }
    else if (IsEqualIID(riid, IID_IUnknown))
    {
	AddRef();
	*ppvObj = this;
    }
    else
    {
	*ppvObj = NULL;
	ret = HXR_NOINTERFACE;
    }

    return ret;
}

STDMETHODIMP_(ULONG32) CHXFileRecognizer::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);
}

STDMETHODIMP_(ULONG32) CHXFileRecognizer::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

STDMETHODIMP CHXFileRecognizer::GetMimeType(IHXFileObject* /*IN*/ pFile, 
	    IHXFileRecognizerResponse* /*IN*/ pRecognizerResponse)
{
    HX_RESULT ret = HXR_FAIL;

    if (pRecognizerResponse)
    {
	m_pResponse = pRecognizerResponse;
	m_pResponse->AddRef();
    }
    
    // get our own IHXFileResponse interface
    IHXFileResponse* pFileResponse = NULL;
    ret = QueryInterface(IID_IHXFileResponse, (void**) &pFileResponse);

    if (SUCCEEDED(ret) && pFileResponse && pFile != NULL)
    {
	m_pFile = pFile;
	m_pFile->AddRef();
    
        ret = m_pFile->Init(HX_FILE_READ | HX_FILE_BINARY, pFileResponse);
    }

    if (FAILED(ret) && m_pResponse) 
    {
	m_pResponse->GetMimeTypeDone(ret, NULL); //XXXLCM this is bad (we return fail)
    }
    
    HX_RELEASE(pFileResponse);

    return ret;
}

STDMETHODIMP CHXFileRecognizer::InitDone(HX_RESULT status)
{
    if (SUCCEEDED(status))
    {
	status = m_pFile->Read(KRecogLength);
    }

    if (FAILED(status) && m_pResponse)
    {
	m_pResponse->GetMimeTypeDone(status, NULL);
    }

    return status;
}

STDMETHODIMP CHXFileRecognizer::SeekDone(HX_RESULT /* status */)
{
    if (!m_bGetMimeTypeDone)
    {
        DoFileRecognize();
        HX_RELEASE(m_pBuffer);
    }

    return HXR_OK;
}

STDMETHODIMP CHXFileRecognizer::ReadDone(HX_RESULT status,
					 IHXBuffer* pBuffer)
{
    if (FAILED(status) && m_pResponse)
    {
	m_pResponse->GetMimeTypeDone(HXR_FAIL, NULL);
    }
    else
    {
        HX_ASSERT(!m_pBuffer);

        m_pBuffer = pBuffer;
        HX_ADDREF(m_pBuffer);
 
         // seek back to the beginning of the file
         // since the same file object will be passed to the file format
        if (HXR_OK != m_pFile->Seek(0, FALSE) &&
            !m_bGetMimeTypeDone)
        {
            DoFileRecognize();
            HX_RELEASE(m_pBuffer);
        }
    }

    return HXR_OK;
}

STDMETHODIMP CHXFileRecognizer::WriteDone(HX_RESULT /* status */)
{
    return HXR_OK;
}

STDMETHODIMP CHXFileRecognizer::CloseDone(HX_RESULT /* status */)
{
    return HXR_OK;
}

void
CHXFileRecognizer::DoFileRecognize(void)
{
    HX_RESULT   ret = HXR_OK;
    IHXBuffer*  pMimeType = NULL;
    const char* pFileName = NULL;

    m_bGetMimeTypeDone = TRUE;

    if (m_pFile)
	m_pFile->GetFilename(pFileName);

    ret = GetMimeType(pFileName, m_pBuffer, pMimeType);

    if (SUCCEEDED(ret) && 
	pMimeType && 
	pMimeType->GetSize() > 0 && 
	m_pResponse)
    {
	m_pResponse->GetMimeTypeDone(HXR_OK, pMimeType);
    }
    else
    {
	m_pResponse->GetMimeTypeDone(HXR_FAIL, NULL);
    }

    HX_RELEASE(pMimeType);
}
