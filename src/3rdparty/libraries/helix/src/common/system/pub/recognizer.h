#ifndef _RECOGNIZER_H_
#define _RECOGNIZER_H_

#include "hxcom.h"
#include "hxfiles.h"
#include "hxfilerecognizer.h"


class CHXFileRecognizer : public IHXFileRecognizer
			, public IHXFileResponse 
{
public:
    CHXFileRecognizer(IUnknown* pContext);
    virtual ~CHXFileRecognizer();

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				 void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);


    /*
     *	IHXFileRecognizer methods
     */

    /************************************************************************
     *	Method:
     *	    IHXFileRecognizer::GetMimeType
     *	Purpose:
     */
    STDMETHOD(GetMimeType) (THIS_
			    IHXFileObject* /*IN*/ pFile, 
			    IHXFileRecognizerResponse* /*IN*/ pFileRecognizerResponse
	);

    // IHXFileResponse methods
    STDMETHOD(InitDone)  (THIS_ HX_RESULT status);
    STDMETHOD(SeekDone)  (THIS_ HX_RESULT status);
    STDMETHOD(ReadDone)  (THIS_ HX_RESULT status, IHXBuffer *pBuffer);
    STDMETHOD(WriteDone) (THIS_ HX_RESULT status);
    STDMETHOD(CloseDone) (THIS_ HX_RESULT status);

private:
    STDMETHOD(GetMimeType) (THIS_ const char* /* IN */pFileName,
			    IHXBuffer* /* IN */ pBuffer,
			    REF(IHXBuffer*) /* OUT*/ pMimeType);

    HXBOOL  IsSDPFile(IHXBuffer* pBuffer);
    HXBOOL  IsRMFile(UCHAR* pData, UINT32 ulSize);
    HXBOOL  IsMP3File(UCHAR* pData, UINT32 ulSize);
    HXBOOL  IsISOMediaFile(UCHAR* pData, UINT32 ulSize, const char* pBrand);
    HXBOOL  IsMP4File(UCHAR* pData, UINT32 ulSize);
    HXBOOL  Is3GPPFile(UCHAR* pData, UINT32 ulSize);
    HXBOOL  IsWMFile(UCHAR* pData, UINT32 ulSize);
    HXBOOL  IsAVIFile(UCHAR* pData, UINT32 ulSize);
    HXBOOL  IsOGGFile(UCHAR* pData, UINT32 ulSize);
    HXBOOL  IsMIDIFile(UCHAR* pData, UINT32 ulSize);
    HXBOOL  IsWAVEFile(UCHAR* pData, UINT32 ulSize);
    HXBOOL  IsMPEGFile(UCHAR* pData, UINT32 ulSize);
    HXBOOL  IsQuickTimeFile(UCHAR* pData, UINT32 ulSize);
    void    DoFileRecognize(void);

    UINT32  LoadMP3Header(UCHAR* pData);
    HXBOOL  IsValidMP3Header(UINT32 ulHdr);
    INT32   getFrameSync(UINT32 ulHdr)	    { return (int)((ulHdr>>21) & 2047); };
    INT32   getVersionIndex(UINT32 ulHdr)   { return (int)((ulHdr>>19) & 3); };
    INT32   getLayerIndex(UINT32 ulHdr)	    { return (int)((ulHdr>>17) & 3); };
    INT32   getBitrateIndex(UINT32 ulHdr)   { return (int)((ulHdr>>12) & 15); };
    INT32   getFrequencyIndex(UINT32 ulHdr) { return (int)((ulHdr>>10) & 3); };
    INT32   getEmphasisIndex(UINT32 ulHdr)  { return (int)(ulHdr & 3); }; 
    
private:
    LONG32 m_lRefCount;
    IUnknown* m_pContext;
    IHXFileRecognizerResponse* m_pResponse;
    IHXFileObject* m_pFile;
    IHXBuffer* m_pBuffer;
    HXBOOL m_bGetMimeTypeDone;
};

#endif // _RECOGNIZER_H_
