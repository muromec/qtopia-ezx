#include "ihxpckts.h"
#include "hxbuffer.h"
#include "pckunpck.h"

#include "recognizer.h"

// file mime-types our recognizer might return
#define HX_MIMETYPE_RM		    "application/x-pn-realmedia"
#define HX_MIMETYPE_MP3		    "audio/mp3"
#define HX_MIMETYPE_MP4AUDIO	    "audio/mp4"
#define HX_MIMETYPE_MP4VIDEO	    "video/mp4"
#define HX_MIMETYPE_3GPAUDIO	    "audio/3gpp"
#define HX_MIMETYPE_3GPVIDEO	    "video/3gpp"
#define HX_MIMETYPE_MPEG	    "video/x-mpeg"
#define HX_MIMETYPE_WM		    "application/vnd.ms-asf"
#define HX_MIMETYPE_AVI		    "application/x-pn-avi-plugin"
#define HX_MIMETYPE_OGG		    "application/ogg"
#define HX_MIMETYPE_MIDI	    "application/x-midi"
#define HX_MIMETYPE_WAVE	    "audio/x-wav"
#define HX_MIMETYPE_QT		    "application/x-pn-quicktime-stream"
#define HX_MIMETYPE_SDP		    "application/sdp"

// length of buffer read from file and passed to recognizer
static const int KRecogLength = 512;

#if !defined (_SYMBIAN)
HX_RESULT CHXFileRecognizer::GetMimeType(const char* pFileName, IHXBuffer* pBuffer, REF(IHXBuffer*) pMimeType)
{
    HX_RESULT retVal = HXR_FAILED;
    char* pMimeTypeString = NULL;
    char* pFileExtension = NULL;
    UCHAR* pData = NULL;
    UINT32 ulSize = 0;
    HXBOOL bContinue = TRUE;

    // get file extension
    if (pFileName)
    {
	CHXString strExtension = pFileName;
	INT32 lPeriod = strExtension.ReverseFind('.');
	if (lPeriod >= 0)
	{
	    strExtension = strExtension.Right(strExtension.GetLength() - lPeriod - 1);
	    if (strExtension.GetLength() > 0)
	    {
		strExtension.MakeLower();
		pFileExtension = (char*)(const char*)strExtension;
	    }
	}
    }

    // data at the begining of the media file
    if (pBuffer)
    {
	ulSize = pBuffer->GetSize();
	pData = pBuffer->GetBuffer();
    }

    // check SDP
    if (IsSDPFile(pBuffer))
    {
	pMimeTypeString = HX_MIMETYPE_SDP;
	bContinue = FALSE;
    }

    //XXXqluo to minimize the risk, the following format check is for ANDROID platform only
#if defined(ANDROID) 
    // skip checking if the file name has an extension.
    if (bContinue && (pFileExtension || ulSize != KRecogLength))
    {
	bContinue = FALSE;
    }

    // RealMedia
    if (bContinue && IsRMFile(pData, ulSize))
    {
	pMimeTypeString = HX_MIMETYPE_RM;
	bContinue = FALSE;
    }

    // 3GPP
    if (bContinue && Is3GPPFile(pData, ulSize))
    {
	pMimeTypeString = HX_MIMETYPE_3GPAUDIO;
	bContinue = FALSE;
    }

    // WM
    if (bContinue && IsWMFile(pData, ulSize))
    {
	pMimeTypeString = HX_MIMETYPE_WM;
	bContinue = FALSE;
    }

    // AVI
    if (bContinue && IsAVIFile(pData, ulSize))
    {
	pMimeTypeString = HX_MIMETYPE_AVI;
	bContinue = FALSE;
    }

    // OGG
    if (bContinue && IsOGGFile(pData, ulSize))
    {
	pMimeTypeString = HX_MIMETYPE_OGG;
	bContinue = FALSE;
    }

    // MIDI
    if (bContinue && IsMIDIFile(pData, ulSize))
    {
	pMimeTypeString = HX_MIMETYPE_MIDI;
	bContinue = FALSE;
    }

    // WAVE
    if (bContinue && IsWAVEFile(pData, ulSize))
    {
	pMimeTypeString = HX_MIMETYPE_WAVE;
	bContinue = FALSE;
    }

    // MP4
    if (bContinue && IsMP4File(pData, ulSize))
    {
	pMimeTypeString = HX_MIMETYPE_MP4AUDIO;
	bContinue = FALSE;
    }

    // QuickTime
    if (bContinue && IsQuickTimeFile(pData, ulSize))
    {
	pMimeTypeString = HX_MIMETYPE_QT;
	bContinue = FALSE;
    }

    // MPEG
    if (bContinue && IsMPEGFile(pData, ulSize))
    {
	pMimeTypeString = HX_MIMETYPE_MPEG;
	bContinue = FALSE;
    }

    // MP3
    if (bContinue && IsMP3File(pData, ulSize))
    {
	pMimeTypeString = HX_MIMETYPE_MP3;
	bContinue = FALSE;
    }

#endif // ANDROID

    if (pMimeTypeString)
    {
	retVal = CreateBufferCCF(pMimeType, m_pContext);
	if (HXR_OK == retVal)
	{
            int len = strlen(pMimeTypeString);
	    pMimeType->Set((const UCHAR*)pMimeTypeString, len + 1);
	    ((char*)pMimeType->GetBuffer())[len] = '\0';
	}
    }

    return retVal;
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

HXBOOL
CHXFileRecognizer::IsRMFile(UCHAR* pData, UINT32 ulSize)
{
    HXBOOL bResult = FALSE;
    UCHAR pRAMagic[4] = {'.', 'r', 'a', 0xFD};
    if (pData && ulSize > 4)
    {
        if (0 == strncmp((const char*)pData, ".RMF", 4) ||		// .RMF
	    0 == strncmp((const char*)pData, ".RMS", 4) ||		// .RMS
	    0 == memcmp(pData, pRAMagic, 4))				// .ra\0xFD
        {
            bResult = TRUE;
        }
    }
    return bResult;
}

HXBOOL
CHXFileRecognizer::IsMP4File(UCHAR* pData, UINT32 ulSize)
{
    if (IsISOMediaFile(pData, ulSize, "M4A") ||
	IsISOMediaFile(pData, ulSize, "mp4"))
    {
	return TRUE;
    }
    else
    {
	return FALSE;
    }
}

HXBOOL
CHXFileRecognizer::IsQuickTimeFile(UCHAR* pData, UINT32 ulSize)
{
    HXBOOL bResult = FALSE;
    if (pData && ulSize > 8)
    {
	// check atom types
	pData += 4;
	if (0 == strncmp((const char*)pData, "moov", 4) ||
	    0 == strncmp((const char*)pData, "free", 4) ||
	    0 == strncmp((const char*)pData, "skip", 4) ||
	    0 == strncmp((const char*)pData, "mdat", 4) ||
	    0 == strncmp((const char*)pData, "wide", 4) ||
	    0 == strncmp((const char*)pData, "pnot", 4))
	{
	    bResult = TRUE;
	}
    }
    return bResult;
}

HXBOOL
CHXFileRecognizer::Is3GPPFile(UCHAR* pData, UINT32 ulSize)
{
    return IsISOMediaFile(pData, ulSize, "3gp");
}

HXBOOL
CHXFileRecognizer::IsWMFile(UCHAR* pData, UINT32 ulSize)
{
    HXBOOL bResult = FALSE;
    if (pData && ulSize > 16)
    {
	UCHAR pMagic[16] = {0x30, 0x26, 0xB2, 0x75, 0x8E, 0x66, 0xCF, 0x11, 0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C};
	if (memcmp(pData, pMagic, 16) == 0)
	{
	    bResult = TRUE;
	}
    }
    return bResult;
}

HXBOOL
CHXFileRecognizer::IsAVIFile(UCHAR* pData, UINT32 ulSize)
{
    HXBOOL bResult = FALSE;
    if (pData && ulSize > 16)
    {
	UCHAR pMagic1[4] = {'R', 'I', 'F', 'F'};			// RIFF
	UCHAR pMagic2[8] = {'A', 'V', 'I', ' ', 'L', 'I', 'S', 'T'};	// AVI LIST chunk
	if (memcmp(pData, pMagic1, 4) == 0 ||
	    memcmp(pData+8, pMagic2, 8) == 0)
	{
	    bResult = TRUE;
	}
    }
    return bResult;
}

HXBOOL
CHXFileRecognizer::IsOGGFile(UCHAR* pData, UINT32 ulSize)
{
    HXBOOL bResult = FALSE;
    if (pData && ulSize > 4)
    {
	UCHAR pMagic[4] = {'O', 'g', 'g', 'S'};				// page magic "OggS"
	if (memcmp(pData, pMagic, 4) == 0)
	{
	    bResult = TRUE;
	}
    }
    return bResult;
}


HXBOOL
CHXFileRecognizer::IsMIDIFile(UCHAR* pData, UINT32 ulSize)
{
    HXBOOL bResult = FALSE;
    if (pData && ulSize > 4)
    {
	UCHAR pMagic[4] = {'M', 'T', 'h', 'd'};				// MIDI header chunk ID
	if (memcmp(pData, pMagic, 4) == 0)
	{
	    bResult = TRUE;
	}
    }
    return bResult;
}

HXBOOL
CHXFileRecognizer::IsWAVEFile(UCHAR* pData, UINT32 ulSize)
{
    HXBOOL bResult = FALSE;
    if (pData && ulSize > 12)
    {
	UCHAR pMagic1[4] = {'R', 'I', 'F', 'F'};			// Chunk ID
	UCHAR pMagic2[4] = {'W', 'A', 'V', 'E'};			// Format
	if (memcmp(pData, pMagic1, 4) == 0 ||
	    memcmp(pData+8, pMagic2, 4) == 0)
	{
	    bResult = TRUE;
	}
    }
    return bResult;
}

HXBOOL
CHXFileRecognizer::IsMPEGFile(UCHAR* pData, UINT32 ulSize)
{
    HXBOOL bResult = FALSE;
    if (pData && ulSize > 4)
    {
	UCHAR pMagic[4] = {0x00, 0x00, 0x01, 0xb3};			// MPEG sequence header start code
	if (memcmp(pData, pMagic, 4) == 0)
	{
	    bResult = TRUE;
	}
    }
    return bResult;
}

HXBOOL
CHXFileRecognizer::IsMP3File(UCHAR* pData, UINT32 ulSize)
{
    HXBOOL bResult = FALSE;
    if (pData && ulSize)
    {
	UCHAR* pStart = pData;
	
	// check for valid frame header
	UINT32 ulHdr;
	UCHAR* pEnd = pData + ulSize - 4;
	do
	{
	    ulHdr = LoadMP3Header(pData);
	    if (IsValidMP3Header(ulHdr))
	    {
		bResult = TRUE;
		break;
	    }
	    pData += 4;
	}
	while(pData < pEnd);
	
	// check for ID3 tags
	if (bResult == FALSE)
	{
	    if (0 == strncmp((const char*)pStart, "ID3", 3))
	    {
		bResult = TRUE;
	    }
	}
    }
    return bResult;
}

UINT32
CHXFileRecognizer::LoadMP3Header(UCHAR* p)
{
    return (UINT32)(((p[0] & 255) << 24) | ((p[1] & 255) << 16) | ((p[2] & 255) <<  8) | ((p[3] & 255)));
}

HXBOOL
CHXFileRecognizer::IsValidMP3Header(UINT32 ulHdr)
{
    return (((getFrameSync(ulHdr)      & 2047)==2047) &&
            ((getVersionIndex(ulHdr)   &    3)!=   1) &&
            ((getLayerIndex(ulHdr)     &    3)!=   0) && 
            ((getBitrateIndex(ulHdr)   &   15)!=   0) &&
            ((getBitrateIndex(ulHdr)   &   15)!=  15) &&
            ((getFrequencyIndex(ulHdr) &    3)!=   3) &&
            ((getEmphasisIndex(ulHdr)  &    3)!=   2)    );
}

HXBOOL
CHXFileRecognizer::IsISOMediaFile(UCHAR* pData, UINT32 ulSize, const char* pBrand)
{
    HXBOOL bResult = FALSE;
    if (pData && ulSize)
    {
	UCHAR* pEnd = (pData + ulSize - 4);

	// locate file type box
	pData += 4;
	HXBOOL bFound = FALSE;
	while (pData < pEnd)
	{
	    // scan for "ftyp"
	    if ((*pData == 'f' && *(pData+1) == 't' && *(pData+2) == 'y' && *(pData+3) == 'p'))
	    {
		bFound = TRUE;
		break;
	    }
	    pData++;
	}

	// match for the given type(pBrand)
	if (bFound)
	{
	    pData += 4;
	    if (0 == strncmp((const char*)pData, pBrand, strlen(pBrand)))
	    {
		bResult = TRUE;
	    }
	}
    }
    return bResult;
}

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
	m_pResponse->GetMimeTypeDone(status, NULL);
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
