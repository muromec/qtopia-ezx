/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: smpte.cpp,v 1.8 2007/07/06 20:39:16 jfinnecy Exp $
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
 * terms of the GNU General Public License Version 2 (the
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

#include "hxtypes.h"
//#include "hlxclib/stdio.h"
#include "safestring.h"
#include "hlxclib/stdlib.h"
#include "hxstring.h"
#include "smpte.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

const UINT32 MSEC_PER_HOUR = (UINT32)60 * 60 * 1000;
const UINT32 MSEC_PER_MIN  = (UINT32)60 * 1000;
const UINT32 MSEC_PER_SEC  = 1000;

SMPTETimeCode::SMPTETimeCode():
    m_hour(0),
    m_minute(0),
    m_second(0),
    m_frame(0),
    m_dropFrame(DROP_FRAME),
    m_framesPerSec(FPS_30),
    m_mSecs(0)
{
}

SMPTETimeCode::SMPTETimeCode(UINT32 mSec):
    m_dropFrame(DROP_FRAME),
    m_framesPerSec(FPS_30),
    m_mSecs(mSec)
{
    fromMSec();
}

SMPTETimeCode::SMPTETimeCode(int hour, int minute, int second, int frame):
    m_hour(hour),
    m_minute(minute),
    m_second(second),
    m_frame(frame),
    m_dropFrame(DROP_FRAME),
    m_framesPerSec(FPS_30)
{
    toMSec();
}

SMPTETimeCode::SMPTETimeCode(const char* pTimeCodeStr):
    m_dropFrame(DROP_FRAME),
    m_framesPerSec(FPS_30)
{
    fromString(pTimeCodeStr);
}

SMPTETimeCode::SMPTETimeCode(const SMPTETimeCode& lhs)
{
    m_hour = lhs.m_hour;
    m_minute = lhs.m_minute;
    m_second = lhs.m_second;
    m_frame = lhs.m_frame;
    m_dropFrame = lhs.m_dropFrame;
    m_framesPerSec = lhs.m_framesPerSec;
    m_mSecs = lhs.m_mSecs;
}

SMPTETimeCode&
SMPTETimeCode::operator=(const SMPTETimeCode& lhs)
{
    m_hour = lhs.m_hour;
    m_minute = lhs.m_minute;
    m_second = lhs.m_second;
    m_frame = lhs.m_frame;
    m_dropFrame = lhs.m_dropFrame;
    m_framesPerSec = lhs.m_framesPerSec;
    m_mSecs = lhs.m_mSecs;
    return *this;
}

int
SMPTETimeCode::compare(const SMPTETimeCode& lhs) const
{
    return HX_SAFEINT(m_mSecs - lhs.m_mSecs);
}

void
SMPTETimeCode::toMSec()
{
    m_mSecs =   m_hour * MSEC_PER_HOUR
                + m_minute * MSEC_PER_MIN
		+ m_second * MSEC_PER_SEC;

    if(m_framesPerSec == FPS_30)
    {
	if(m_frame > 29)
	{
	    m_frame = 0;
	}
	else
	{
	    if(m_dropFrame == DROP_FRAME)
	    {
		// each frame is 33.367 ms (29.97 Hz)
		m_mSecs += (int)(((double)m_frame * 33.367) + 0.5);
	    }
	    else
	    {
		// each frame is 33.333 ms
		m_mSecs += (int)(((double)m_frame * 33.333) + 0.5);
	    }
	}
    }
    else if(m_framesPerSec == FPS_25)
    {
	// each frame is 40ms
	if(m_frame > 24)
	{
	    m_frame = 0;
	}
	else
	{
	    m_mSecs += m_frame * 40;
	}
    }
}

void
SMPTETimeCode::fromMSec()
{
    int fps = (m_framesPerSec == FPS_30) ? 30: 25;
    UINT32 ttlFrames = (m_mSecs * fps) / 1000;
    m_hour = HX_SAFEINT(ttlFrames / (fps * 60 * 60));
    UINT32 hourRem = ttlFrames - (m_hour * fps * 60 * 60);
    m_minute = HX_SAFEINT(hourRem / (fps * 60));
    UINT32 minRem = hourRem - (m_minute * fps * 60);
    m_second = HX_SAFEINT(minRem / fps);
    m_frame = HX_SAFEINT(ttlFrames % fps);
}

const char*
SMPTETimeCode::toString()
{
    char strBuf[12]; /* Flawfinder: ignore */
    SafeSprintf(strBuf, sizeof(strBuf), "%02d:%02d:%02d",
                m_hour, m_minute, m_second);
    if(m_frame > 0)
        SafeSprintf(&strBuf[8], sizeof(strBuf)-8, ".%02d", m_frame);
    m_asString = strBuf;
    return m_asString;
}

void
SMPTETimeCode::fromString(const char* pTimeCodeString)
{
    // format is [H]H:[M]M:[S]S[.FF]

    m_hour = m_minute = m_second = m_frame = 0;
    char* token = 0;
    if(pTimeCodeString && strlen(pTimeCodeString) > 0)
    {
	char *tmpStr = new char[strlen(pTimeCodeString)+1];
	strcpy(tmpStr, pTimeCodeString); /* Flawfinder: ignore */
	token = strtok(tmpStr, ":");
	if(token)
	{
	    m_hour = HX_SAFEINT(strtol(token, 0, 10));
	    token = strtok(NULL, ":");
	    if(token)
	    {
		m_minute = HX_SAFEINT(strtol(token, 0, 10));
		token = strtok(NULL, ".");
		if(token)
		{
		    m_second = HX_SAFEINT(strtol(token, 0, 10));
		    token = strtok(NULL, " ");
		    if(token)
		    {
			m_frame = HX_SAFEINT(strtol(token, 0, 10));
		    }
		}
	    }
	}
    }
    toMSec();
    fromMSec();	// normalize just in case of a badly formatted string
}

SMPTETimeCode::operator UINT32()
{
    return m_mSecs;
}

SMPTETimeCode::operator const char*()
{
    toString();
    return m_asString;
}

SMPTETimeCode&
SMPTETimeCode::operator+=(const SMPTETimeCode& lhs)
{
    // no checking for overflow
    m_mSecs += lhs.m_mSecs;
    fromMSec();
    return *this;
} 

SMPTETimeCode&
SMPTETimeCode::operator-=(const SMPTETimeCode& lhs)
{
    // don't go negative on me...
    if(m_mSecs > lhs.m_mSecs)
    {
	m_mSecs -= lhs.m_mSecs;
    }
    else
    {
	m_mSecs = 0;
    }
    fromMSec();
    return *this;
}

SMPTETimeCode
SMPTETimeCode::operator+(const SMPTETimeCode& lhs)
{
    SMPTETimeCode tCode = *this;
    tCode += lhs;
    return tCode;
}

SMPTETimeCode
SMPTETimeCode::operator-(const SMPTETimeCode& lhs)
{
    SMPTETimeCode tCode = *this;
    tCode -= lhs;
    return tCode;
}
