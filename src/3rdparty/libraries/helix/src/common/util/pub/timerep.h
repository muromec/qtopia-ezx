/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: timerep.h,v 1.7 2005/03/14 19:36:41 bobclark Exp $
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

/*
 * UTCTimeRep holds a UTC timestamp in internal (time_t) representation.  It
 * can convert to/from RFC-1123, RFC-850, ANSI 'asctime', and UTC strings.
 *
 * Dates prior to 1970 are not supported because (1) time_t is unsigned on
 * some systems, and (2) time_t is frequently cast to UINT32 in Helix code.
 *
 * Dates after 2037 are not supported because time_t is usually signed.
 */

#include "hlxclib/time.h"

#ifndef _UTCTimeRep_H_
#define _UTCTimeRep_H_

#ifdef _WIN16
size_t strftime16(char* pDest, size_t nDestLen, const char* pFormat, const struct tm* ptm);
#endif /* _WIN16 */

#define MAX_UTC_TIME_LEN 80

#define INVALID_TIME_T (time_t)(-1)

class UTCTimeRep
{
public:
    UTCTimeRep(void);                       // Create from current time
    UTCTimeRep(const char* pStr);           // Create from string
    UTCTimeRep(time_t t, HXBOOL bUTC=TRUE);   // Create from time_t value
    ~UTCTimeRep(void);

    const char* asRFC1123String(void);
    const char* asRFC850String(void);
    const char* asUTCString(void);
    time_t      asUTCTimeT(void);

    void SetLocalTime(time_t t);
    void SetUTCTime(time_t t);
    void SetTime(const char* pStr);

private:
    int fromTime(time_t t);             // Set from time_t in localtime
    int fromUTCTime(time_t t);          // Set from time_t in GMT
    int fromTm(struct tm* ptm);         // Set from struct tm in localtime
    int fromUTCTm(struct tm* ptm);      // Set from struct tm in GMT
    int fromString(const char* pStr);   // Set from RFC1123/RFC850/asctime/UTC

    //XXXTDM: we could remove this if asXXXString() took input strings
    char        m_szTime[MAX_UTC_TIME_LEN+1]; /* Flawfinder: ignore */
    time_t      m_tTime;
};

#endif
