#include <apgcli.h> // RApaLsSession
#include <apmrec.h> // CApaDataRecognizerType
#include <apmstd.h>
#include <utf.h>

#include "hxresult.h"
#include "hxbuffer.h"
#include "hxprefutil.h"
#include "pckunpck.h"
#include "recognizer.h"

static TInt GetMimeTypeFromOS(const TDesC& fileName, const TDesC8& fileBuf, TDataType &type)
{
    RApaLsSession ls;
    TInt err;
    if ( (err=ls.Connect()) == KErrNone)
    {
	TDataRecognitionResult rr;
	err = ls.RecognizeData(fileName, fileBuf, rr);
	    
	if (err == KErrNone && rr.iConfidence != CApaDataRecognizerType::ENotRecognized)
	{
	    type = rr.iDataType;
	}

	ls.Close();
    }

    return err;
}

static HXBOOL Is3GPPHeader(const TDesC& fileName,
                          const TDesC8& fileBuf,
                          TDataType &type)
{
    //  known atom types.
    _LIT8(K3GPType , "ftyp3gp"  );  
    _LIT8(K3GPRel6 , "ftyp3gp6" );  // Release 6
    _LIT8(K3GPProg , "ftyp3gr6" );  // Progressive Download.
    _LIT8(K3GPStrm , "ftyp3gs6" );  // streaming
    _LIT8(KMP4Type , "ftypmp4"  );  // mp4 format
    _LIT8(KM4AType , "ftypM4A"  );  // mp4 audio
    _LIT8(K3G2Type , "ftyp3g2"  );  // 3g2 format

    // mime types can be returned
    _LIT8(K3GPPVideo    ,  "video/3gpp"   );  
    _LIT8(K3GPPAudio    ,  "audio/3gpp"   );  
    _LIT8(K3GPP2Video   ,  "video/3gpp2"  );  
    _LIT8(KMP4Video     ,  "video/mp4"    );  
    _LIT8(KMP4Audio     ,  "audio/mp4"    );  
    

    TInt offset = 4;     // first 4 bytes are size; 
                         // no need for extended size atom.
    TInt maxHeaderSize = 8;

    if (fileBuf.Length() < offset + maxHeaderSize )
    {
        return FALSE;
    }

    HXBOOL bRet = TRUE;
    TPtrC8 buffer = fileBuf.Mid(offset, maxHeaderSize);

    if (buffer.FindF(K3GPType)    != KErrNotFound
        || buffer.FindF(K3GPRel6) != KErrNotFound
        || buffer.FindF(K3GPProg) != KErrNotFound
        || buffer.FindF(K3GPStrm) != KErrNotFound
       )
    {
        type = TDataType(K3GPPVideo);
    }
    else if(buffer.FindF(KMP4Type) != KErrNotFound)
    {
        type = TDataType(KMP4Video);
    }
    else if (buffer.FindF(KM4AType) != KErrNotFound)
    {
        type = TDataType(KMP4Audio);
    }
    else if (buffer.FindF(K3G2Type) != KErrNotFound) 
    {
        type = TDataType(K3GPP2Video);
    }
    else
    {
        bRet = FALSE;
    }

    return bRet;
}

static HXBOOL IsRealMediaHeader(const TDesC& fileName,
                               const TDesC8& fileBuf,
                               TDataType &type)
{
    //  known header.
    _LIT8(KRMType , ".RMF" );  

    // mime types can be returned.
    _LIT8(KRealMedia , "application/x-pn-realmedia" );

    if (fileBuf.Length() < 4)
    {
        return FALSE;
    }

    HXBOOL bRet = TRUE;
    
    TPtrC8 buffer = fileBuf.Mid(0, 4);
    if (fileBuf.FindF(KRMType) != KErrNotFound) 
    {
        type = TDataType(KRealMedia);
    }
    else
    {
        bRet = FALSE;
    }
    return bRet;
}

static TInt DoGetMimeType(const TDesC& fileName, 
                          const TDesC8& fileBuf,
                          TDataType &type,
                          IUnknown* pContext)
{
    TInt ret = KErrNone;

    if (Is3GPPHeader(fileName, fileBuf, type))
    {
        return ret;
    }
    else if (IsRealMediaHeader(fileName, fileBuf, type))
    {
        return ret;
    }
    else 
    {
        // check if system recognizer has been disabled.
        HXBOOL bDisableSystemRecognizer = FALSE;
        if (pContext)
        {
        ReadPrefBOOL(pContext, "DisableSystemRecognizer", bDisableSystemRecognizer);
        }

        if (!bDisableSystemRecognizer)
        {
        // we can't recognize this. Try OS recognizer.
        ret = GetMimeTypeFromOS(fileName, fileBuf, type);
        }
        else
        {
            ret = KErrNotSupported;
        }
    }
    
    return ret;
}

STDMETHODIMP CHXFileRecognizer::GetMimeType(const char* pFileName,
					    IHXBuffer* pBuffer,
					    REF(IHXBuffer*) pMimeType)
{
    HX_RESULT ret = HXR_FAILED;

    if (pBuffer && pBuffer->GetSize() > 0)
    {
	TInt err = KErrNoMemory;

        if (IsSDPFile(pBuffer))
        {
            // convert descriptor MIME type to IHXBuffer
	    CreateBufferCCF(pMimeType, m_pContext);
            if (pMimeType)
            {
                int len = strlen("application/sdp");
	        pMimeType->Set((const UCHAR*)"application/sdp", len + 1);
	        ((char*)pMimeType->GetBuffer())[len] = '\0';
	        return HXR_OK;
            }
        }

	// convert file name to descriptor
	TPtrC8 from((TUint8*)pFileName);
	HBufC* pUCFileName = HBufC::New(from.Length());

	if (pUCFileName)
	{
	    TPtr to = pUCFileName->Des();
	    err = CnvUtfConverter::ConvertToUnicodeFromUtf8(to, from);
	}

	if (KErrNone == err)
	{
	    // convert the file buffer to a descriptor
	    TPtrC8 fileBuf((TUint8*)pBuffer->GetBuffer(), 
			   pBuffer->GetSize());
		
	    TDataType dataType;
	    err = DoGetMimeType(*pUCFileName, fileBuf, dataType, m_pContext);

	    if (KErrNone == err)
	    {
		// convert descriptor MIME type to IHXBuffer
		CreateBufferCCF(pMimeType, m_pContext);
		if (pMimeType)
		{
		    TPtrC8 p = dataType.Des8();
		    int len = p.Length();
		    pMimeType->Set((const UCHAR*)p.Ptr(), len + 1);
		    ((char*)pMimeType->GetBuffer())[len] = '\0';
		    ret = HXR_OK;
		}
	    }
	}
	delete pUCFileName;
    }

    return ret;
}
