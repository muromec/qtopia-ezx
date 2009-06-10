/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: timerep.cpp,v 1.18 2007/07/06 20:39:16 jfinnecy Exp $
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

#include "hlxclib/stdlib.h"
#include "hlxclib/string.h"
#include "hlxclib/time.h"
#include "hlxclib/ctype.h"

#include "hxstrutl.h"
#include "timerep.h"

#include "hxassert.h"
#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE
static const char HX_THIS_FILE[] = __FILE__;
#endif

#define SECS_PER_MIN    60
#define SECS_PER_HOUR   (60*SECS_PER_MIN)
#define SECS_PER_DAY    (24*SECS_PER_HOUR)
#define SECS_PER_YEAR   (365*SECS_PER_DAY)

#ifdef _WIN32

#include <olectl.h>

// Conversions between FILETIME and time_t
static const UINT64 EPOCH_DIFF_DAYS = 134774;
static const UINT64 EPOCH_DIFF_SECS = (EPOCH_DIFF_DAYS*SECS_PER_DAY);
static const UINT64 SECS_TO_100NS   = 10000000;

static void TimetToFileTime( time_t t, LPFILETIME pft )
{
    UINT64 u = ((UINT64)t + EPOCH_DIFF_SECS) * SECS_TO_100NS;
    pft->dwLowDateTime = (DWORD)u;
    pft->dwHighDateTime = (DWORD)(u>>32);
}
static time_t FileTimeToTimet( LPFILETIME pft )
{
    UINT64 u = ((UINT64)pft->dwHighDateTime << 32) |
               ((UINT64)pft->dwLowDateTime);
    return (time_t)( u / SECS_TO_100NS - EPOCH_DIFF_SECS );
}

#endif /* _WIN32 */

/*
 * To reproduce g_monthoffset using GNU date(2) and bash(1):
 *   for N in 1 2 3 4 5 6 7 8 9 10 11 12; do
 *     date -u --date="1/$N/1970" +%s
 *   done
 *
 * To reproduce g_monthleapoffset, add SECS_PER_DAY to 2..11
 */
static const time_t g_monthoffset[12] =
    {        0,  2678400,  5097600,  7776000, 10368000, 13046400,
      15638400, 18316800, 20995200, 23587200, 26265600, 28857600 };
static const int g_monthleapoffset[12] =
    {        0,  2678400,  5184000, 7862400,  10454400, 13132800,
      15724800, 18403200, 21081600, 23673600, 26352000, 28944000 };
static const int g_monthdays[12] =
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
static const int g_monthleapdays[12] =
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

static const char* const g_wkday_names[7] =
    { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static const char* const g_weekday_names[7] =
    { "Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday" };
static const char* const g_mon_names[12] =
    { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
static const char* const g_month_names[12] =
    { "January", "February", "March",     "April",   "May",      "June",
      "July",    "August",   "September", "October", "November", "December" };

static inline const char* wkday_to_str(int n)
{
    return ( (n >= 0 && n < 7) ? g_wkday_names[n] : NULL );
}

static inline const char* weekday_to_str(int n)
{
    return ( (n >= 0 && n < 7) ? g_weekday_names[n] : NULL );
}

static inline const char* mon_to_str(int n)
{
    return ( (n >= 0 && n < 12) ? g_mon_names[n] : NULL );
}

static inline const char* month_to_str(int n)
{
    return ( (n >= 0 && n < 12) ? g_month_names[n] : NULL );
}

static int weekday_from_string(const char* pStr)
{
    int n;
    for (n = 0; n < 7; n++)
    {
        if (strncasecmp(g_wkday_names[n], pStr, 3) == 0)
        {
            return n;
        }
    }
    return -1;
}

static int month_from_string(const char* pStr)
{
    int n;
    for (n = 0; n < 12; n++)
    {
        if (strncasecmp(g_mon_names[n], pStr, 3) == 0)
        {
            return n;
        }
    }
    return -1;
}

/*
 * We assume 32-bit signed time_t and disallow years prior to 1970 here.
 * Supporting 64-bit time_t and years prior to 1970 are not trivial, as
 * time_t values are commonly converted to UINT32 (eg. stat results).
 *
 * We also require input values to be within their legal intervals to avoid
 * normalization overhead.  This is not required by mktime(3).
 */
time_t mktime_gmt(struct tm* ptm)
{
    time_t t = 0;
    if (ptm == NULL)
    {
        HX_ASSERT(FALSE);
        goto bail;
    }

    // Calculate seconds to beginning of year (including leap days)
    // eg. one leap day for 1973..1976, two for 1977..1980, etc.
    // Year 2000 was a leap year so the leap calculation is easy
    if (ptm->tm_year < 70 || ptm->tm_year > 137)
    {
        goto bail;
    }
    t = SECS_PER_YEAR * (ptm->tm_year-70) +
        SECS_PER_DAY * ((ptm->tm_year-69)/4);

    // Add seconds to beginning of day (including leap day, if needed)
    if (ptm->tm_mon < 0 || ptm->tm_mon > 11)
    {
        goto bail;
    }
    if (ptm->tm_year%4)
    {
        t += g_monthoffset[ptm->tm_mon];
        if (ptm->tm_mday < 1 || ptm->tm_mday > g_monthdays[ptm->tm_mon])
        {
            goto bail;
        }
    }
    else
    {
        t += g_monthleapoffset[ptm->tm_mon];
        if (ptm->tm_mday < 1 || ptm->tm_mday > g_monthleapdays[ptm->tm_mon])
        {
            goto bail;
        }
    }
    t += SECS_PER_DAY * (ptm->tm_mday - 1);

    // Add hours, minutes, seconds
    // Note: we allow up to 62 seconds per minute for "leap seconds"
    if (ptm->tm_hour < 0 || ptm->tm_hour > 23 ||
        ptm->tm_min  < 0 || ptm->tm_min  > 59 ||
        ptm->tm_sec  < 0 || ptm->tm_sec  > 61)
    {
        goto bail;
    }

    t += ptm->tm_hour*SECS_PER_HOUR + ptm->tm_min*SECS_PER_MIN + ptm->tm_sec;

    return t;

bail:
    return INVALID_TIME_T;
}

// Win16 and WinCE lack strftime() so implement it here...
#if defined(_WIN16) || defined(WIN32_PLATFORM_PSPC)

/*
 * In all of the following make_xxx functions, the following hold true:
 *   - If the destination length is too small to hold the result, the
 *     buffer is not touched and pDest is not advanced.
 *   - nLen is always updated using the length of the source string.
 *
 * Thus, we can detect overflow by checking nLen against nDestLen and the
 * NULL terminator is always placed at pDest with no overflow worries.
 */

static void make_wkday(char*& pDest, size_t nDestLen, int n, size_t& nLen)
{
    const char* pStr = wkday_to_str(n);
    if (pStr != NULL)
    {
        size_t nStrLen = strlen(pStr);
        if (nLen + nStrLen < nDestLen)
        {
            strcpy(pDest, pStr); /* Flawfinder: ignore */
            pDest += nStrLen;
        }
        nLen += nStrLen;
    }
}

static void make_weekday(char*& pDest, size_t nDestLen, int n, size_t& nLen)
{
    const char* pStr = weekday_to_str(n);
    if (pStr != NULL)
    {
        size_t nStrLen = strlen(pStr);
        if (nLen + nStrLen < nDestLen)
        {
            strcpy(pDest, pStr); /* Flawfinder: ignore */
            pDest += nStrLen;
        }
        nLen += nStrLen;
    }
}

static void make_mon(char*& pDest, size_t nDestLen, int n, size_t& nLen)
{
    const char* pStr = mon_to_str(n);
    if (pStr != NULL)
    {
        size_t nStrLen = strlen(pStr);
        if (nLen + nStrLen < nDestLen)
        {
            strcpy(pDest, pStr); /* Flawfinder: ignore */
            pDest += nStrLen;
        }
        nLen += nStrLen;
    }
}

static void make_month(char*& pDest, size_t nDestLen, int n, size_t& nLen)
{
    const char* pStr = month_to_str(n);
    if (pStr != NULL)
    {
        size_t nStrLen = strlen(pStr);
        if (nLen + nStrLen < nDestLen)
        {
            strcpy(pDest, pStr); /* Flawfinder: ignore */
            pDest += nStrLen;
        }
        nLen += nStrLen;
    }
}

static void make_value(char*& pDest, size_t nDestLen, int n, size_t& nLen)
{
    char szBuffer[16]; /* Flawfinder: ignore */
    _itoa(n, szBuffer, 10);
    size_t nStrLen = strlen(szBuffer);
    if (nLen + nStrLen < nDestLen)
    {
        strcpy(pDest, szBuffer); /* Flawfinder: ignore */
        pDest += nStrLen;
    }
    nLen += nStrLen;
}

size_t strftime16(char* pDest, size_t nDestLen, const char* pFormat, const struct tm* ptm)
{
    size_t nLen = 0;
    if (pDest == NULL)
    {
        HX_ASSERT(FALSE);
        return 0;
    }
    pDest[0] = '\0';
    if (pFormat == NULL)
    {
        return 0;
    }

    while (*pFormat && nLen < nDestLen)
    {
        if (*pFormat == '%' && *(pFormat+1) != '\0')
        {
            pFormat++;
            switch (*pFormat)
            {
            case 'a':       // abbreviated weekday name
                make_wkday(pDest, nDestLen, ptm->tm_wday, nLen);
                break;
            case 'A':       // full weekday name
                make_weekday(pDest, nDestLen, ptm->tm_wday, nLen);
                break;
            case 'b':       // abbreviated month name
                make_mon(pDest, nDestLen, ptm->tm_wday, nLen);
                break;
            case 'B':       // full month name
                make_month(pDest, nDestLen, ptm->tm_wday, nLen);
                break;
            case 'd':       // day of month in decimal
                make_value(pDest, nDestLen, ptm->tm_mday, nLen);
                break;
            case 'H':       // hour in 24-hr format
                make_value(pDest, nDestLen, ptm->tm_hour, nLen);
                break;
            case 'm':       // month as decimal number
                make_value(pDest, nDestLen, ptm->tm_mon+1, nLen);
                break;
            case 'M':       // minutes as decimal number
                make_value(pDest, nDestLen, ptm->tm_min, nLen);
                break;
            case 'S':       // seconds as decimal number
                make_value(pDest, nDestLen, ptm->tm_sec, nLen);
                break;
            case 'y':       // year without century
                make_value(pDest, nDestLen, ptm->tm_year, nLen);
                break;
            case 'Y':       // year with century
                make_value(pDest, nDestLen, ptm->tm_year+1900, nLen);
                break;
            default:       // add more options if needed
                break;
            }
        }
        else
        {
            if (nLen+1 < nDestLen)
            {
                *pDest = *pFormat;
                pDest++;
            }
            nLen++;
        }
        pFormat++;
    }
    *pDest = '\0';
    return nLen;
}

#endif /* _WIN16 || WIN32_PLATFORM_PSPC */

UTCTimeRep::UTCTimeRep(void)
{
    m_szTime[0] = '\0';
    fromUTCTime(time(NULL));
}

UTCTimeRep::UTCTimeRep(const char* pStr)
{
    m_szTime[0] = '\0';
    fromString(pStr);
}

UTCTimeRep::UTCTimeRep(time_t t, HXBOOL bUTC /* = TRUE */)
{
    m_szTime[0] = '\0';
    bUTC ? fromUTCTime(t) : fromTime(t);
}

UTCTimeRep::~UTCTimeRep(void)
{
    // Empty
}

const char*
UTCTimeRep::asRFC1123String(void)
{
    m_szTime[0] = '\0';
    struct tm* ptm = NULL;

    // If our time is valid, fetch a struct tm in GMT.
    struct tm tmgmt;
    if (m_tTime != INVALID_TIME_T)
    {
        ptm = hx_gmtime_r(&m_tTime, &tmgmt);
    }

    // If the struct tm is good, create the string.
    if (ptm != NULL)
    {
#if defined(_WIN16) || defined(WIN32_PLATFORM_PSPC)
        if (strftime16(m_szTime, MAX_UTC_TIME_LEN, "%a, %d %b %Y %H:%M:%S GMT", ptm) == 0)
#else
        if (strftime(m_szTime, MAX_UTC_TIME_LEN, "%a, %d %b %Y %H:%M:%S GMT", ptm) == 0)
#endif
        {
            // If there is an error, the string is undefined so empty it.
            m_szTime[0] = '\0';
        }
    }

    return m_szTime;
}

const char*
UTCTimeRep::asRFC850String(void)
{
    m_szTime[0] = '\0';
    struct tm tmgmt;

    // If our time is valid, fetch a struct tm in GMT.
    if (m_tTime != INVALID_TIME_T)
    {
        hx_gmtime_r(&m_tTime, &tmgmt);
    }

    // If the struct tm is good, create the string.
#if defined(_WIN16) || defined(WIN32_PLATFORM_PSPC)
    if (strftime16(m_szTime, MAX_UTC_TIME_LEN, "%A, %d-%b-%Y %H:%M:%S GMT", &tmgmt) == 0)
#else
    if (strftime(m_szTime, MAX_UTC_TIME_LEN, "%A, %d-%b-%Y %H:%M:%S GMT", &tmgmt) == 0)
#endif
    {
        m_szTime[0] = '\0';
    }
    return m_szTime;
}

const char*
UTCTimeRep::asUTCString(void)
{
    m_szTime[0] = '\0';
    struct tm tmgmt;

    // If our time is valid, fetch a struct tm in GMT.
    if (m_tTime != INVALID_TIME_T)
    {
        hx_gmtime_r(&m_tTime, &tmgmt);
    }

    // If the struct tm is good, create the string.
#if defined(_WIN16) || defined(WIN32_PLATFORM_PSPC)
    if (strftime16(m_szTime, MAX_UTC_TIME_LEN, "%Y%m%dT%H%M%SZ", &tmgmt) == 0)
#else
    if (strftime(m_szTime, MAX_UTC_TIME_LEN, "%Y%m%dT%H%M%SZ", &tmgmt) == 0)
#endif
    {
        m_szTime[0] = '\0';
    }
    return m_szTime;
}

time_t
UTCTimeRep::asUTCTimeT(void)
{
    return m_tTime;
}

void UTCTimeRep::SetLocalTime(time_t t)
{
    fromTime(t);
}

void UTCTimeRep::SetUTCTime(time_t t)
{
    fromUTCTime(t);
}

void UTCTimeRep::SetTime(const char* pStr)
{
    fromString(pStr);
}

/*
 * Convert time_t value from localtime to gmt.  First we call gmtime() to get
 * a struct tm in localtime.  This works because gmtime() does no timezone
 * conversions -- the result is in the same timezone as the input.  Then we
 * call mktime() which converts a struct tm in localtime to a time_t in GMT.
 *
 * Win32 always assumes GMT for mktime(), so we use LocalFileTimeToFileTime()
 * and a pair of conversion functions.
 */
int
UTCTimeRep::fromTime(time_t t)
{
#ifdef _WIN32
    FILETIME lft, ft;
    TimetToFileTime(t, &lft);
    LocalFileTimeToFileTime(&lft, &ft);
    m_tTime = FileTimeToTimet(&ft);
    return 0;
#else /* ! _WIN32 */
    struct tm tmlocal;
    hx_gmtime_r(&t, &tmlocal);
    m_tTime = mktime(&tmlocal);
    return 0;
#endif /* _WIN32 */
}

int
UTCTimeRep::fromUTCTime(time_t t)
{
    m_tTime = t;
    return 0;
}

int
UTCTimeRep::fromTm(struct tm* ptm)
{
#ifdef _WIN32
    // See note above in fromTime()
    time_t t = mktime_gmt(ptm);
    if (t != INVALID_TIME_T)
    {
        fromTime(t);
        return 0;
    }
    return -1;
#else /* ! _WIN32 */
    if (ptm != NULL)
    {
        m_tTime = mktime(ptm);
        return 0;
    }
    m_tTime = INVALID_TIME_T;
    return -1;
#endif /* _WIN32 */
}

int
UTCTimeRep::fromUTCTm(struct tm* ptm)
{
    m_tTime = mktime_gmt(ptm);
    return ( (m_tTime == INVALID_TIME_T) ? -1 : 0 );
}

// Find next alnum field in string or bail if not found
#define NEXT_ALNUM_FIELD(p) \
    while ( isalnum(*p)) { p++; if (*p == '\0') goto bail; } \
    while (!isalnum(*p)) { p++; if (*p == '\0') goto bail; }

int
UTCTimeRep::fromString(const char* pStr)
{
    unsigned int n;
    struct tm stm;

    memset(&stm, 0, sizeof(stm));
    if (pStr == NULL || pStr[0] == '\0')
    {
        goto bail;
    }

    /*
     * RFC1123: wkday "," SP DD SP month SP YYYY HH:MM:SS (Wed, 02 Jun 1982)
     * RFC850 : weekday "," SP DD-month-YY HH:MM:SS       (Wednesday, 02-Jun-82)
     * asctime: wkday SP month SP dD HH:MM:SS YYYY        (Wed Jun  2 09:00:00 1982)
     * UTC    : YYYYMMDD'T'HHMMSS'Z'
     */
    if (isdigit(pStr[0]))
    {
        // UTC
        if(strlen(pStr) != 16)
        {
            goto bail;
        }

        for (n = 0; n < 8; n++)
        {
            if (!isdigit(pStr[n]))
            {
                goto bail;
            }
        }
        stm.tm_year = (pStr[0]-'0')*1000 + (pStr[1]-'0')*100 +
                      (pStr[2]-'0')*10 + (pStr[3]-'0') - 1900;
        stm.tm_mon = (pStr[4]-'0')*10 + (pStr[5]-'0');
        stm.tm_mday = (pStr[6]-'0')*10 + (pStr[7]-'0');

        for (n = 9; n < 15; n++)
        {
            if (!isdigit(pStr[n]))
            {
                goto bail;
            }
        }
        stm.tm_hour = (pStr[9]-'0')*10 + (pStr[10]-'0');
        stm.tm_min = (pStr[11]-'0')*10 + (pStr[12]-'0');
        stm.tm_sec = (pStr[13]-'0')*10 + (pStr[14]-'0');

        // We have a struct tm in GMT
        return fromUTCTm(&stm);
    }
    else
    {
        if (weekday_from_string(pStr) < 0)
        {
            goto bail;
        }
        NEXT_ALNUM_FIELD(pStr);
        if (isdigit(pStr[0]))
        {
            // RFC1123 or RFC850
            if (!isdigit(pStr[1]) || isalnum(pStr[2]))
            {
                goto bail;
            }
            stm.tm_mday = (pStr[0]-'0')*10 + (pStr[1]-'0');
            NEXT_ALNUM_FIELD(pStr);
            if ((stm.tm_mon = month_from_string(pStr)) < 0)
            {
                goto bail;
            }
            NEXT_ALNUM_FIELD(pStr);
            if (!isdigit(pStr[0]) || !isdigit(pStr[1]))
            {
                goto bail;
            }
            if (!isdigit(pStr[2]))
            {
                // RFC850
                stm.tm_year = (pStr[0]-'0')*10 + (pStr[1]-'0');
            }
            else if (isdigit(pStr[3]) && !isdigit(pStr[4]))
            {
                // RFC1123
                stm.tm_year = (pStr[0]-'0')*1000 + (pStr[1]-'0')*100 +
                              (pStr[2]-'0')*10 + (pStr[3]-'0') - 1900;
            }
            else
            {
                goto bail;
            }
            NEXT_ALNUM_FIELD(pStr);
            for (n = 0; n < 8; n++)
            {
                if (n == 2 || n == 5)
                {
                    if (pStr[n] != ':')
                    {
                        goto bail;
                    }
                }
                else if (!isdigit(pStr[n]))
                {
                    goto bail;
                }
            }
            stm.tm_hour = (pStr[0]-'0')*10 + (pStr[1]-'0');
            stm.tm_min = (pStr[3]-'0')*10 + (pStr[4]-'0');
            stm.tm_sec = (pStr[6]-'0')*10 + pStr[7]-'0';
            pStr += 8;

            NEXT_ALNUM_FIELD(pStr);
            if (strncasecmp(pStr, "GMT", 3) != 0)
            {
                goto bail;
            }

            // We have a struct tm in GMT
            return fromUTCTm(&stm);
        }
        else
        {
            // asctime
            NEXT_ALNUM_FIELD(pStr);
            if ((stm.tm_mon = month_from_string(pStr)) == -1)
            {
                goto bail;
            }
            NEXT_ALNUM_FIELD(pStr);
            if (isdigit(pStr[0]) && pStr[1] == ' ')
            {
                stm.tm_mday = (pStr[0]-'0');
            }
            else if (isdigit(pStr[0]) && isdigit(pStr[1]) && pStr[2] == ' ')
            {
                stm.tm_mday = (pStr[0]-'0')*10 + (pStr[1]-'0');
            }
            else
            {
                goto bail;
            }
            NEXT_ALNUM_FIELD(pStr);
            for (n = 0; n < 8; n++)
            {
                if (n == 2 || n == 5)
                {
                    if (pStr[n] != ':')
                    {
                        goto bail;
                    }
                }
                else if (!isdigit(pStr[n]))
                {
                    goto bail;
                }
            }
            stm.tm_hour = (pStr[0]-'0')*10 + (pStr[1]-'0');
            stm.tm_min = (pStr[3]-'0')*10 + (pStr[4]-'0');
            stm.tm_sec = (pStr[6]-'0')*10 + pStr[7]-'0';
            pStr += 8;
            NEXT_ALNUM_FIELD(pStr);
            if (!isdigit(pStr[0]) || !isdigit(pStr[1]) ||
                !isdigit(pStr[2]) || !isdigit(pStr[3]) ||
                isdigit(pStr[4]))
            {
                goto bail;
            }
            stm.tm_year = (pStr[0]-'0')*1000 + (pStr[1]-'0')*100 +
                          (pStr[2]-'0')*10 + (pStr[3]-'0') - 1900;

            // We have a struct tm in localtime
            return fromTm(&stm);
        }
    }
    /* NOTREACHED */
bail:
    m_tTime = INVALID_TIME_T;
    return -1;
}
