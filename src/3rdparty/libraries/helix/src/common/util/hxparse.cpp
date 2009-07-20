/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxparse.cpp,v 1.9 2005/03/14 19:36:39 bobclark Exp $
 * 
 * Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
 * 
 * The contents of this file, and the files included with this file,
 * are subject to the current version of the RealNetworks Public
 * Source License (the "RPSL") available at
 * http://www.helixcommunity.org/content/rpsl unless you have licensed
 * the file under the current version of the RealNetworks Community
 * Source License (the "RCSL") available at
 * http://www.helixcommunity.org/content/rcsl, in which case the RCSL
 * will apply. You may also obtain the license terms directly from
 * RealNetworks.  You may not use this file except in compliance with
 * the RPSL or, if you have a valid RCSL with RealNetworks applicable
 * to this file, the RCSL.  Please see the applicable RPSL or RCSL for
 * the rights, obligations and limitations governing use of the
 * contents of the file.
 * 
 * Alternatively, the contents of this file may be used under the
 * terms of the GNU General Public License Version 2 or later (the
 * "GPL") in which case the provisions of the GPL are applicable
 * instead of those above. If you wish to allow use of your version of
 * this file only under the terms of the GPL, and not to allow others
 * to use your version of this file under the terms of either the RPSL
 * or RCSL, indicate your decision by deleting the provisions above
 * and replace them with the notice and other provisions required by
 * the GPL. If you do not delete the provisions above, a recipient may
 * use your version of this file under the terms of any one of the
 * RPSL, the RCSL or the GPL.
 * 
 * This file is part of the Helix DNA Technology. RealNetworks is the
 * developer of the Original Code and owns the copyrights in the
 * portions it created.
 * 
 * This file, and the files included with this file, is distributed
 * and made available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY
 * KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS
 * ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET
 * ENJOYMENT OR NON-INFRINGEMENT.
 * 
 * Technology Compatibility Kit Test Suite(s) Location:
 *    http://www.helixcommunity.org/content/tck
 * 
 * Contributor(s):
 * 
 * ***** END LICENSE BLOCK ***** */

#include "hlxclib/stdlib.h"
//#include "hlxclib/stdio.h"
#include "hlxclib/ctype.h"

#include "hxtypes.h"
#include "hxresult.h"
#include "hxcom.h"
#include "chxxtype.h"
#include "hxwintyp.h"
#include "hxstring.h"
#include "hxstrutl.h"
#include "hxparse.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif


/*
 * our smil/html color string table
 */


static const struct smilColorTable
{
    const char* m_pColorName;
    UINT8 m_ucRed;
    UINT8 m_ucGreen;
    UINT8 m_ucBlue;
} SmilColorTable[] =
{
    {"black", 	0x00, 0x00, 0x00},
    {"silver", 	0xc0, 0xc0, 0xc0},
    {"gray",  	0x80, 0x80, 0x80},
    {"white", 	0xff, 0xff, 0xff},
    {"maroon",	0x80, 0x00, 0x00},
    {"red",   	0xff, 0x00, 0x00},
    {"purple",	0x80, 0x00, 0x80},
    {"fuchsia", 0xff, 0x00, 0xff},
    {"green",	0x00, 0x80, 0x00},
    {"lime",	0x00, 0xff, 0x00},
    {"olive",	0x80, 0x80, 0x00},
    {"yellow",	0xff, 0xff, 0x00},
    {"navy",	0x00, 0x00, 0x80},
    {"blue",	0x00, 0x00, 0xff},
    {"teal",	0x00, 0x80, 0x80},
    {"aqua",	0x00, 0xff, 0xff},
    {0,		0x00, 0x00, 0x00}
};


/****************************************************************************
 *  getColorElement
 *
 *  parses a hex value from the string passed in.
 */

UINT8
getColorElement(const char* pColorFrag, int len)
{
    UINT8 ucValue = 0;

    char* pTmpBuf = new char[len+1];
    strncpy(pTmpBuf, pColorFrag, len); /* Flawfinder: ignore */
    pTmpBuf[len] = 0;

    ucValue = (UINT8)strtol(pTmpBuf, 0, 16);
    delete[] pTmpBuf;
    return ucValue;
}


/****************************************************************************
 *  HXParseColor
 *
 *  Parses a smil/html color string and returns its HXxColor value.  The
 *  string should be in one of the following formats: "#RGB", "#RRGGBB",
 *  or one of the pre-defined strings in the table at the top of this file.
 */

HX_RESULT
HXParseColor(const char* pColorString, REF(HXxColor) theColor)
{
    HX_RESULT theErr = HXR_INVALID_PARAMETER;
    theColor = 0;
    UINT8 ucRed = 0;
    UINT8 ucGreen = 0;
    UINT8 ucBlue = 0;
    if(pColorString[0] == '#')
    {
	if(strlen(pColorString) == 4)
	{
	    /* #rgb, duplicate the numbers */
	    char tmpBuf[6]; /* Flawfinder: ignore */
	    tmpBuf[0] = tmpBuf[1] = pColorString[1];
	    tmpBuf[2] = tmpBuf[3] = pColorString[2];
	    tmpBuf[4] = tmpBuf[5] = pColorString[3];
	    ucRed = getColorElement(&tmpBuf[0], 2);
	    ucGreen = getColorElement(&tmpBuf[2], 2);
	    ucBlue = getColorElement(&tmpBuf[4], 2);
	    theErr = HXR_OK;
	}
	else if(strlen(pColorString) == 7)
	{
	    /* #rrggbb */
	    ucRed = getColorElement(&pColorString[1], 2);
	    ucGreen = getColorElement(&pColorString[3], 2);
	    ucBlue = getColorElement(&pColorString[5], 2);
	    theErr = HXR_OK;
	}
    }
    else if (!strncmp(pColorString, "rgb(", 4))
    {
        // This color is in the form rgb(0,128,255) or
        // in the form rgb(0%,50%,100%)
        //
        // Make a copy of the string, since strtok
        // is destructive
        char* pCopy = new char [strlen(pColorString) + 1];
        if (pCopy)
        {
            strcpy(pCopy, pColorString); /* Flawfinder: ignore */
            // Get past the "rgb("
            UINT32 ulTmp = 0;
            char*  pStr  = strtok(pCopy, "(,)");
            // Get the red string
            pStr = strtok(NULL, "(,)");
            if (pStr)
            {
                // Use HXParseOpacity to parse the red component
                theErr = HXParseOpacity((const char*) pStr, ulTmp);
                if (SUCCEEDED(theErr))
                {
                    // Assign the red component
                    ucRed = (BYTE) ulTmp;
                    // Get the green string
                    pStr = strtok(NULL, "(,)");
                    if (pStr)
                    {
                        // Use HXParseOpacity to parse the green component
                        theErr = HXParseOpacity((const char*) pStr, ulTmp);
                        if (SUCCEEDED(theErr))
                        {
                            // Assign the green component
                            ucGreen = (BYTE) ulTmp;
                            // Get the blue string
                            pStr = strtok(NULL, "(,)");
                            if (pStr)
                            {
                                // Use HXParseOpacity to parse the blue component
                                theErr = HXParseOpacity((const char*) pStr, ulTmp);
                                if (SUCCEEDED(theErr))
                                {
                                    // Assign the blue string
                                    ucBlue = (BYTE) ulTmp;
                                }
                            }
                            else
                            {
                                theErr = HXR_INVALID_PARAMETER;
                            }
                        }
                    }
                    else
                    {
                        theErr = HXR_INVALID_PARAMETER;
                    }
                }
            }
        }
        HX_VECTOR_DELETE(pCopy);
    }
    else
    {
	// string, try to get it from the color table
	int i = 0;
	const char* pColorName = SmilColorTable[i].m_pColorName;
	while(pColorName)
	{
	    if(strcmp(pColorName, pColorString) == 0)
	    {
		ucRed = SmilColorTable[i].m_ucRed;
		ucBlue = SmilColorTable[i].m_ucBlue;
		ucGreen = SmilColorTable[i].m_ucGreen;
		theErr = HXR_OK;
		break;
	    }
	    pColorName = SmilColorTable[++i].m_pColorName;
	}
    }

    // Check and see if it's a system color definition.
    // See http://www.w3.org/TR/REC-CSS2/ui.html#system-colors
    // for more info about these.
#ifdef _WINDOWS
    if (FAILED(theErr))
    {
        HXBOOL  bMatch = FALSE;
        INT32 lIndex = 0;
        if (!strcasecmp(pColorString, "ActiveBorder"))
        {
            lIndex = COLOR_ACTIVEBORDER;
            bMatch = TRUE;
        }
        else if (!strcasecmp(pColorString, "ActiveCaption"))
        {
            lIndex = COLOR_ACTIVECAPTION;
            bMatch = TRUE;
        }
        else if (!strcasecmp(pColorString, "AppWorkspace"))
        {
            lIndex = COLOR_APPWORKSPACE;
            bMatch = TRUE;
        }
        else if (!strcasecmp(pColorString, "Background"))
        {
            lIndex = COLOR_BACKGROUND;
            bMatch = TRUE;
        }
        else if (!strcasecmp(pColorString, "ButtonFace"))
        {
            lIndex = COLOR_BTNFACE;
            bMatch = TRUE;
        }
        else if (!strcasecmp(pColorString, "ButtonHighlight"))
        {
            lIndex = COLOR_BTNHILIGHT;
            bMatch = TRUE;
        }
        else if (!strcasecmp(pColorString, "ButtonShadow"))
        {
            lIndex = COLOR_BTNSHADOW;
            bMatch = TRUE;
        }
        else if (!strcasecmp(pColorString, "ButtonText"))
        {
            lIndex = COLOR_BTNTEXT;
            bMatch = TRUE;
        }
        else if (!strcasecmp(pColorString, "CaptionText"))
        {
            lIndex = COLOR_CAPTIONTEXT;
            bMatch = TRUE;
        }
        else if (!strcasecmp(pColorString, "GrayText"))
        {
            lIndex = COLOR_GRAYTEXT;
            bMatch = TRUE;
        }
        else if (!strcasecmp(pColorString, "Highlight"))
        {
            lIndex = COLOR_HIGHLIGHT;
            bMatch = TRUE;
        }
        else if (!strcasecmp(pColorString, "HighlightText"))
        {
            lIndex = COLOR_HIGHLIGHTTEXT;
            bMatch = TRUE;
        }
        else if (!strcasecmp(pColorString, "InactiveBorder"))
        {
            lIndex = COLOR_INACTIVEBORDER;
            bMatch = TRUE;
        }
        else if (!strcasecmp(pColorString, "InactiveCaption"))
        {
            lIndex = COLOR_INACTIVECAPTION;
            bMatch = TRUE;
        }
        else if (!strcasecmp(pColorString, "InactiveCaptionText"))
        {
            lIndex = COLOR_INACTIVECAPTIONTEXT;
            bMatch = TRUE;
        }
        else if (!strcasecmp(pColorString, "InfoBackground"))
        {
            lIndex = COLOR_INFOBK;
            bMatch = TRUE;
        }
        else if (!strcasecmp(pColorString, "InfoText"))
        {
            lIndex = COLOR_INFOTEXT;
            bMatch = TRUE;
        }
        else if (!strcasecmp(pColorString, "Menu"))
        {
            lIndex = COLOR_MENU;
            bMatch = TRUE;
        }
        else if (!strcasecmp(pColorString, "MenuText"))
        {
            lIndex = COLOR_MENUTEXT;
            bMatch = TRUE;
        }
        else if (!strcasecmp(pColorString, "Scrollbar"))
        {
            lIndex = COLOR_SCROLLBAR;
            bMatch = TRUE;
        }
        else if (!strcasecmp(pColorString, "ThreeDDarkShadow"))
        {
            lIndex = COLOR_3DDKSHADOW;
            bMatch = TRUE;
        }
        else if (!strcasecmp(pColorString, "ThreeDFace"))
        {
            lIndex = COLOR_3DFACE;
            bMatch = TRUE;
        }
        else if (!strcasecmp(pColorString, "ThreeDHighlight"))
        {
            lIndex = COLOR_3DHILIGHT;
            bMatch = TRUE;
        }
        else if (!strcasecmp(pColorString, "ThreeDLightShadow"))
        {
            lIndex = COLOR_3DLIGHT;
            bMatch = TRUE;
        }
        else if (!strcasecmp(pColorString, "ThreeDShadow"))
        {
            lIndex = COLOR_3DSHADOW;
            bMatch = TRUE;
        }
        else if (!strcasecmp(pColorString, "Window"))
        {
            lIndex = COLOR_WINDOW;
            bMatch = TRUE;
        }
        else if (!strcasecmp(pColorString, "WindowFrame"))
        {
            lIndex = COLOR_WINDOWFRAME;
            bMatch = TRUE;
        }
        else if (!strcasecmp(pColorString, "WindowText"))
        {
            lIndex = COLOR_WINDOWTEXT;
            bMatch = TRUE;
        }
        if (bMatch)
        {
            UINT32 ulColor = GetSysColor(lIndex);
            ucRed          = GetRValue(ulColor);
            ucGreen        = GetGValue(ulColor);
            ucBlue         = GetBValue(ulColor);
            theErr         = HXR_OK;

        }
    }
#endif

#ifdef _WINDOWS
    theColor = (HXxColor)(RGB(ucRed, ucGreen, ucBlue));
#else
    theColor = (HXxColor)
	    (ucRed << 16 |
	    ucGreen << 8 |
	    ucBlue);
#endif
    return theErr;
}


HX_RESULT
HXParseDigit(const char* pDigitString, REF(INT32) ulOut)
{
    // validate it first. ([whitespace][sign]digits)
    HX_RESULT ret = HXR_OK;
    const char* pBuf = pDigitString;
    
    // clear white space.
    while (*pBuf && isspace(*pBuf))
    {
	++pBuf;
    }

    // check for sign
    if (*pBuf == '+' || *pBuf == '-')
    {
	++pBuf;
    }

    // validate all else is a digit
    while (*pBuf)
    {
	if (!isdigit(*pBuf))
	{
	    ret = HXR_FAIL;
	    break;
	}
	++pBuf;
    }

    // run the conversion anyway... just in case the user wants
    // to ignor the error.
    ulOut = atol(pDigitString);

    return ret;
}

HX_RESULT
HXParseDouble(const char* pDoubleString, REF(double) dOut)
{
    // validate it first. 
    // ([whitespace][sign][digits][.digits][{d|D|e|E}[sign]digits])
    HX_RESULT ret = HXR_OK;
    const char* pBuf = pDoubleString;
    // clear white space.
    while (*pBuf && isspace(*pBuf))
    {
	++pBuf;
    }

    // check for sign
    if (*pBuf == '+' || *pBuf == '-')
    {
	++pBuf;
    }

    while (isdigit(*pBuf))
    {
	++pBuf;
    }

    if (*pBuf == '.')
    {
	++pBuf;
    }

    while (isdigit(*pBuf))
    {
	++pBuf;
    }

    if (*pBuf == 'd' || *pBuf == 'D' || *pBuf == 'e' || *pBuf == 'E')
    {
	++pBuf;
	if (*pBuf == '+' || *pBuf == '-')
	{
	    ++pBuf;
	}

	while (isdigit(*pBuf))
	{
	    ++pBuf;
	}
    }
    
    // we will allow whitespace at the end of the buffer,
    while (isspace(*pBuf))
    {
	++pBuf;
    }

    // now if we are not at the NULL termination something is wrong with the
    // string.
    if (*pBuf != '\0')
    {
	ret = HXR_INVALID_PARAMETER;
    }

    // run the conversion anyway...  The string might simply have junk at 
    // the end of it, in which case the error *might* want to be ignored.
    dOut = atof(pDoubleString);

    return ret;
}

HX_RESULT HXParseColorUINT32(const char* pszStr, REF(UINT32) rulValue)
{
    HX_RESULT retVal = HXR_OK;

    if (pszStr)
    {
        HXxColor cColor;
        retVal = HXParseColor(pszStr, cColor);
        if (SUCCEEDED(retVal))
        {
            rulValue = (UINT32) cColor;
#if defined(_WINDOWS)
            // HXParseColor produces 0x00BBGGRR on Windows - 
            // do the swap back to 0x00RRGGBB
            rulValue = ((rulValue & 0x00FF0000) >> 16)  |
                        (rulValue & 0x0000FF00)         |
                       ((rulValue & 0x000000FF) << 16);
#endif
        }
    }
    else
    {
        retVal = HXR_FAIL;
    }

    return retVal;
}

HX_RESULT HXParsePercent(const char* pszStr, REF(double) rdValue)
{
    HX_RESULT retVal = HXR_FAIL;

    if (pszStr)
    {
        char*  pEndPtr = NULL;
        double dVal    = strtod(pszStr, &pEndPtr);
        if (pEndPtr && *pEndPtr == '%')
        {
            rdValue = dVal;
            retVal  = HXR_OK;
        }
    }

    return retVal;
}

HX_RESULT HXParseUINT32(const char* pszStr, REF(UINT32) rulValue)
{
    HX_RESULT retVal = HXR_OK;

    if (pszStr)
    {
        INT32 lVal = 0;
        retVal     = HXParseDigit(pszStr, lVal);
        if (SUCCEEDED(retVal))
        {
            if (lVal >= 0)
            {
                rulValue = (UINT32) lVal;
            }
            else
            {
                retVal = HXR_FAIL;
            }
        }
    }
    else
    {
        retVal = HXR_FAIL;
    }

    return retVal;
}

HX_RESULT HXParseOpacity(const char* pszStr, REF(UINT32) rulValue)
{
    HX_RESULT retVal = HXR_OK;

    if (pszStr)
    {
        // First attempt to parse as a percent
        INT32  lValue   = 0;
        double dPercent = 0.0;
        retVal = HXParsePercent(pszStr, dPercent);
        if (SUCCEEDED(retVal))
        {
            // Scale from 0-100% to 0-255 with rounding
            lValue = (INT32) (dPercent * 255.0 / 100.0 + 0.5);
        }
        else
        {
            // It wasn't a percent, so try and parse as digits
            retVal = HXParseDigit(pszStr, lValue);
        }
        if (SUCCEEDED(retVal))
        {
            // Clamp to 0-255
            if (lValue < 0)   lValue = 0;
            if (lValue > 255) lValue = 255;
            // Assign the out parameter
            rulValue = (UINT32) lValue;
        }
    }
    else
    {
        retVal = HXR_FAIL;
    }

    return retVal;
}
