/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: livekeyframe.cpp,v 1.6 2008/03/28 05:00:44 jzeng Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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

#include <stdio.h>
#include "hxtypes.h"
#include "hlxclib/string.h"
#include "hxmime.h"
#include "hxcom.h"
#include "hxtypes.h"
#include "hxcomm.h"
#include "hxengin.h"
#include "hxcore.h"
#include "hxmon.h"

#include "livekeyframe.h"

#define VIDEO_KEYFRAME_MIME_TYPE_LIST "config.LiveReducedStartupDelay.VideoKeyFrameMimeType"
#define AUDIO_KEYFRAME_MIME_TYPE_LIST "config.LiveReducedStartupDelay.AudioKeyFrameMimeType"
#define REALEVENT_KEYFRAME_MIME_TYPE_LIST "config.LiveReducedStartupDelay.RealEventMimeType" 

char** g_pVideoKeyFrameMimeTypes = NULL;
char** g_pAudioKeyFrameMimeTypes = NULL;
char** g_pRealEventMimeTypes = NULL;

const char* g_pDefaultpVideoKeyFrameMimeTypes[] = 
{ 
    REALVIDEO_MIME_TYPE, 
    REALVIDEO_ENCRYPTED_MIME_TYPE,
    "video/MP4V-ES", 
    "video/H263-2000", 
    "video/H264", 
    REALVIDEO_MULTIRATE_MIME_TYPE,
    REALVIDEO_ENCRYPTED_MULTIRATE_MIME_TYPE,
    NULL
};

const char* g_pDefaultAudioKeyFrameMimeTypes[] = 
{ 
    REALAUDIO_MIME_TYPE, 
    REALAUDIO_ENCRYPTED_MIME_TYPE,
    REALAUDIO_MULTIRATE_MIME_TYPE,
    REALAUDIO_ENCRYPTED_MULTIRATE_MIME_TYPE,
    REALAUDIO_MULTIRATE_LIVE_MIME_TYPE,
    REALAUDIO_ENCRYPTED_MULTIRATE_LIVE_MIME_TYPE,
    NULL
};

const char* g_pDefaultRealEventMimeTypes[] = 
{ 
    REALEVENT_MIME_TYPE, 
    NULL
};

void InitKeyframeMimeType(IHXRegistry* pReg, char* szRegList, const char** szDefaultList, REF(char**) szList)
{
    HX_RESULT hResult = HXR_OK;
    IHXValues* pList = NULL;
    UINT32 ulNewCount = 0;
    UINT32 ulDefaultCount = 0;
    UINT32 id;
    const char* name;
    UINT32 i = 0;

    for(i = 0; szDefaultList[i] != NULL; i++)
    {
        ulDefaultCount++;
    }

    hResult = pReg->GetPropListByName(szRegList, pList);
    if(SUCCEEDED(hResult))
    {
        HX_RESULT ret = pList->GetFirstPropertyULONG32(name, id);
        while(ret == HXR_OK)
        {
            ulNewCount++;
            ret = pList->GetNextPropertyULONG32(name, id);
        }
    }

    szList = new char*[ulNewCount + ulDefaultCount + 1];

    for(i = 0; i < ulDefaultCount; i++)
    {
        szList[i] = (char*)szDefaultList[i];
    }

    //if we get entries from config file
    if(SUCCEEDED(hResult))
    {
        hResult = pList->GetFirstPropertyULONG32(name, id);
        while(hResult == HXR_OK)
        {
            IHXBuffer* pBuf = 0;
            if(HXR_OK == pReg->GetStrById(id, pBuf))
            {
                szList[i] = new_string((const char *)pBuf->GetBuffer());
                i++;
                HX_RELEASE(pBuf);
            }
            hResult = pList->GetNextPropertyULONG32(name, id); 
        }
    }

    szList[i] = NULL;
}


BOOL
IsVideoKeyframeStream(IHXRegistry* pReg, const char* szMimeType)
{
    if(!g_pVideoKeyFrameMimeTypes)
    {
        if(!pReg)
{
            return FALSE;
        }
        InitKeyframeMimeType(pReg, VIDEO_KEYFRAME_MIME_TYPE_LIST,
                    g_pDefaultpVideoKeyFrameMimeTypes, g_pVideoKeyFrameMimeTypes);
    }

    for (int i = 0; g_pVideoKeyFrameMimeTypes[i] != NULL; i++)
    {
        if (strcasecmp(szMimeType, g_pVideoKeyFrameMimeTypes[i]) == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
IsAudioKeyframeStream(IHXRegistry* pReg, const char* szMimeType)
{
    if(!g_pAudioKeyFrameMimeTypes)
{
        if(!pReg)
        {
            return FALSE;
        }
        InitKeyframeMimeType(pReg, AUDIO_KEYFRAME_MIME_TYPE_LIST,
                    g_pDefaultAudioKeyFrameMimeTypes, g_pAudioKeyFrameMimeTypes);
    }

    for (int i = 0; g_pAudioKeyFrameMimeTypes[i] != NULL; i++)
    {
        if (strcasecmp(szMimeType, g_pAudioKeyFrameMimeTypes[i]) == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL
IsRealEventStream(IHXRegistry* pReg, const char* szMimeType)
{
    if(!g_pRealEventMimeTypes)
{
        if(!pReg)
        {
            return FALSE;
        }
        InitKeyframeMimeType(pReg, REALEVENT_KEYFRAME_MIME_TYPE_LIST,
                    g_pDefaultRealEventMimeTypes, g_pRealEventMimeTypes);
    }

    for (int i = 0; g_pRealEventMimeTypes[i] != NULL; i++)
    {
        if (strcasecmp(szMimeType, g_pRealEventMimeTypes[i]) == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL IsRealVideoStream(const char* szMimeType)
{
    return  !strcasecmp(szMimeType, REALVIDEO_MIME_TYPE) ||
            !strcasecmp(szMimeType, REALVIDEO_ENCRYPTED_MIME_TYPE) ||
            !strcasecmp(szMimeType, REALVIDEO_MULTIRATE_MIME_TYPE) ||
            !strcasecmp(szMimeType, REALVIDEO_ENCRYPTED_MULTIRATE_MIME_TYPE);
}
