// include
#include "hxtypes.h"
#include "hxwintyp.h"
#include "hxcom.h"
#include "ihxpckts.h"
// pnmisc
#include "hxparse.h"
// pxcomlib
#include "pxtransp.h"

HX_RESULT ParseTransparencyParameters(IHXValues* pValues,
                                      REF(UINT32) rulBgOpacity,
                                      REF(HXBOOL)   rbBgOpacitySpecified,
                                      REF(UINT32) rulMediaOpacity,
                                      REF(HXBOOL)   rbMediaOpacitySpecified,
                                      REF(UINT32) rulChromaKey,
                                      REF(HXBOOL)   rbChromaKeySpecified,
                                      REF(UINT32) rulChromaKeyTolerance,
                                      REF(UINT32) rulChromaKeyOpacity,
                                      REF(HXBOOL)   rbAlphaChannelNeeded)
{
    HX_RESULT retVal = HXR_OK;

    if (pValues)
    {
        // Get the background opacity
        IHXBuffer* pStr = NULL;
        HX_RESULT   rv   = pValues->GetPropertyCString("backgroundOpacity", pStr);
        if (SUCCEEDED(rv))
        {
            UINT32 ulTmp = 0;
            retVal = HXParseOpacity((const char*) pStr->GetBuffer(), ulTmp);
            if (SUCCEEDED(retVal))
            {
                rulBgOpacity         = ulTmp;
                rbBgOpacitySpecified = TRUE;
                if (rulBgOpacity < 255)
                {
                    rbAlphaChannelNeeded = TRUE;
                }
            }
        }
        if (SUCCEEDED(retVal))
        {
            // Get the media opacity
            HX_RELEASE(pStr);
            rv = pValues->GetPropertyCString("mediaOpacity", pStr);
            if (SUCCEEDED(rv))
            {
                UINT32 ulTmp = 0;
                retVal = HXParseOpacity((const char*) pStr->GetBuffer(), ulTmp);
                if (SUCCEEDED(retVal))
                {
                    rulMediaOpacity         = ulTmp;
                    rbMediaOpacitySpecified = TRUE;
                    if (rulMediaOpacity < 255)
                    {
                        rbAlphaChannelNeeded = TRUE;
                    }
                }
            }
        }
        if (SUCCEEDED(retVal))
        {
            // Get the media chromakey
            HX_RELEASE(pStr);
            rv = pValues->GetPropertyCString("chromaKey", pStr);
            if (SUCCEEDED(rv))
            {
                UINT32 ulTmp = 0;
                retVal = HXParseColorUINT32((const char*) pStr->GetBuffer(), ulTmp);
                if (SUCCEEDED(retVal))
                {
                    rulChromaKey         = ulTmp;
                    rbChromaKeySpecified = TRUE;
                    rbAlphaChannelNeeded = TRUE;
                }
            }
        }
        if (SUCCEEDED(retVal))
        {
            // Get the media chromakey tolerance
            HX_RELEASE(pStr);
            rv = pValues->GetPropertyCString("chromaKeyTolerance", pStr);
            if (SUCCEEDED(rv))
            {
                UINT32 ulTmp = 0;
                retVal = HXParseColorUINT32((const char*) pStr->GetBuffer(), ulTmp);
                if (SUCCEEDED(retVal))
                {
                    rulChromaKeyTolerance = ulTmp;
                }
            }
        }
        if (SUCCEEDED(retVal))
        {
            // Get the chroma key opacity
            HX_RELEASE(pStr);
            rv = pValues->GetPropertyCString("chromaKeyOpacity", pStr);
            if (SUCCEEDED(rv))
            {
                UINT32 ulTmp = 0;
                retVal = HXParseOpacity((const char*) pStr->GetBuffer(), ulTmp);
                if (SUCCEEDED(rv))
                {
                    rulChromaKeyOpacity = ulTmp;
                }
            }
        }
        HX_RELEASE(pStr);
    }
    else
    {
        retVal = HXR_FAIL;
    }

    return retVal;
}

HXBOOL DoesChromaKeyChannelMatch(UINT32 ulColor,
                               UINT32 ulChromaKey,
                               UINT32 ulChromaKeyTol)
{
    HXBOOL bRet = FALSE;

    INT32 lDiff = ((INT32) ulColor) - ((INT32) ulChromaKey);
    if (lDiff < 0)
    {
        lDiff = -lDiff;
    }
    if (lDiff <= (INT32) ulChromaKeyTol)
    {
        bRet = TRUE;
    }

    return bRet;
}

HXBOOL DoesChromaKeyMatch(UINT32 ulColor,
                        UINT32 ulChromaKey,
                        UINT32 ulChromaKeyTol)
{
    HXBOOL bRet = FALSE;

    if (DoesChromaKeyChannelMatch(ARGB32_RED(ulColor),
                                  ARGB32_RED(ulChromaKey),
                                  ARGB32_RED(ulChromaKeyTol)) &&
        DoesChromaKeyChannelMatch(ARGB32_GREEN(ulColor),
                                  ARGB32_GREEN(ulChromaKey),
                                  ARGB32_GREEN(ulChromaKeyTol)) &&
        DoesChromaKeyChannelMatch(ARGB32_BLUE(ulColor),
                                  ARGB32_BLUE(ulChromaKey),
                                  ARGB32_BLUE(ulChromaKeyTol)))
    {
        bRet = TRUE;
    }

    return bRet;
}

