/*
 *   This routine should contain RTSP related utility functions.
 */


#include "hxtypes.h"	
#include "hxresult.h"	
#include "hxcom.h"  
#include "hxcomm.h"
#include "hxcore.h"
#include "hxfiles.h"

// pncont
#include "hxstring.h"
#include "chxpckts.h"

#include "mimescan.h"

#include "rtpproxy.h"

HXBOOL proxiesSupportFlash4(IHXBuffer* pViaStr)
{
    HXBOOL cachesFlash4 = TRUE;

    // check via header to verify that al proxies support RS 8.0 features
    // (ie latest flash datatype)
    const char* pHeaderValues = (const char*) pViaStr->GetBuffer();

    MIMEInputStream input(pHeaderValues, strlen(pHeaderValues));
    MIMEScanner scanner(input);
    CHXString strViaFinal;

    MIMEToken nextTok = scanner.nextToken(",");
    while(nextTok.hasValue() || (nextTok.lastChar() != MIMEToken::T_EOL) && (nextTok.lastChar() != MIMEToken::T_EOF))
    {
        if (nextTok.hasValue())
        {
            CHXString strViaToken = nextTok.value();

            // per spec there should be a protocol/version string there, ie "RTSP/1.0"
            // or "1.0". Old RealProxies used non-compliant psuedonym only!
            int nVerOffset = strViaToken.Find("RealProxy Version");
            if(nVerOffset == -1)
            {
                // oops, there is a proxy here that does not support flash 4
                cachesFlash4 = FALSE;
                break;
            }
        }
        
        nextTok = scanner.nextToken(",");
    }

    return cachesFlash4;
}

void AddNoCacheHeader(IUnknown* pContext, IHXRequest* pRequest)
{
    // Add the "Cache-Control: no-cache" response header so that Proxies
    // don't think they can cache our dynamically generated content
    IHXCommonClassFactory* pCCF;
    if (pContext && pRequest)
    {
	pContext->QueryInterface(IID_IHXCommonClassFactory,
		    (void**)&pCCF);
        IHXValues* pResHeaders = NULL;

        pRequest->GetResponseHeaders(pResHeaders);
        if (!pResHeaders)
        {
            IUnknown* pUnknown = NULL;

            pCCF->CreateInstance(CLSID_IHXKeyValueList,
                (void**)&pUnknown);

            pUnknown->QueryInterface(IID_IHXValues,
                (void**)&pResHeaders);

            pRequest->SetResponseHeaders(pResHeaders);

            HX_RELEASE(pUnknown);
        }

        IHXBuffer* pNoCache = NULL;
        pCCF->CreateInstance(CLSID_IHXBuffer,
            (void**)&pNoCache);
        pNoCache->Set((UCHAR*)"no-cache", 9);

        pResHeaders->SetPropertyCString("Pragma", pNoCache);

        HX_RELEASE(pNoCache);
        HX_RELEASE(pResHeaders);
	HX_RELEASE(pCCF);
    }
}

