/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: logoutputs.cpp,v 1.12 2006/05/24 00:24:49 dcollins Exp $
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


///////////////////////////////////////////////////////////////////////////////
// Includes
///////////////////////////////////////////////////////////////////////////////

#include "hlxclib/stdio.h"
#include "hlxclib/stdlib.h"
#include "hlxclib/string.h"
#include "hlxclib/time.h"
#include "hlxclib/errno.h"

#ifdef _UNIX
#include <syslog.h>
#endif

#include "hxtypes.h"

#include <sys/stat.h>

#include "servlist.h"
#include "hxcom.h"     // IUnknown
#include "hxcomm.h"   // IHXCommonClassFactory
#include "hxmon.h"    // IHXRegistry
#include "findfile.h"
#include "ihxpckts.h"
#include "hxplugn.h"
#include "hxerror.h"
#include "hxstrutl.h"  // strcasecmp
#include "netbyte.h"   // IsNumericAddr
#include "hxmap.h"     // PNMapLongToObj
#include "timeval.h"
#include "nptime.h"
#include "debug.h"
#include "hxmutexlock.h"
#include "errmsg_macros.h"

#ifdef WIN32
#include "hlxclib/windows.h"
#endif

#include "logoutputs.h"

#ifdef WIN32
#define vsnprintf _vsnprintf
#define snprintf _snprintf
#endif

// Macro Definitions
#define IfFailGo(x) { if (FAILED(x)) { goto cleanup; } }

#define DAYS_PER_MONTH    30
#define HOURS_PER_DAY     24
#define DAYS_PER_WEEK      7
#define MINUTES_PER_HOURS 60


#if WIN32
#define ADDSLASH(x) { strcat(x, "\\"); }
#else
#define ADDSLASH(x) { strcat(x, "/"); }
#endif

///////////////////////////////////////////////////////////////////////////////
//
// LogOutput Class Methods
//
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//  LogOutput::LogOutput
///////////////////////////////////////////////////////////////////////////////

LogOutput::LogOutput()
    : m_RefCount(0)
{
}


///////////////////////////////////////////////////////////////////////////////
//  LogOutput::~LogOutput
///////////////////////////////////////////////////////////////////////////////

LogOutput::~LogOutput()
{
}

///////////////////////////////////////////////////////////////////////////////
//  IUnknown::AddRef
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
LogOutput::AddRef(void)
{
    return InterlockedIncrement(&m_RefCount);
}


///////////////////////////////////////////////////////////////////////////////
//  IUnknown::Release
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
LogOutput::Release(void)
{
    if (InterlockedDecrement(&m_RefCount) > 0)
    {
        return m_RefCount;
    }

    delete this;
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
//  IUnknown::QueryInterface
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
LogOutput::QueryInterface(REFIID interfaceID,
                          void** ppInterfaceObj)
{
    // By definition all COM objects support the IUnknown interface
    if (IsEqualIID(interfaceID, IID_IUnknown))
    {
        AddRef();
        *ppInterfaceObj = (IUnknown*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(interfaceID, IID_IHXLogOutput))
    {
        AddRef();
        *ppInterfaceObj = (void*)this;
        return HXR_OK;
    }

    // No other interfaces are supported
    *ppInterfaceObj = NULL;
    return HXR_NOINTERFACE;
}

// This function gets overridden
STDMETHODIMP
LogOutput::Output(IHXBuffer* pOutput)
{
    return HXR_OK;
}

// This function gets overridden
STDMETHODIMP
LogOutput::Output(const char *pString)
{
    return HXR_OK;
}




///////////////////////////////////////////////////////////////////////////////
//
// LogFileOutput Class Methods
//
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// LogFileOutput::LogFileOutput  (use this one for stdout)
///////////////////////////////////////////////////////////////////////////////

LogFileOutput::LogFileOutput()
    : LogOutput()
    , m_szFilenameBase(NULL)
    , m_szFilenamePath(NULL)
    , m_szCurrentOpenFile(NULL)
    , m_szLogDirectory(NULL)
    , m_szRollTimeAbs(NULL)
    , m_pRollCallback(NULL)
    , m_pScheduler(NULL)
    , m_ulRollTimeCallbackHandle(0)
    , m_nRollTimeOffset(0)
    , m_nRollSize(0)
    , m_ulPruneThreshold(0)
    , m_nByteCount(0)
    , m_lfLogFile()
    , m_pErrorLog(NULL)
{
}


///////////////////////////////////////////////////////////////////////////////
// LogFileOutput::LogFileOutput   (use this one for logging to a file)
///////////////////////////////////////////////////////////////////////////////

LogFileOutput::LogFileOutput(const char* szFilename,
                             const char* szLogDirectory,
                             IHXScheduler *pScheduler,
                             IUnknown* pContext)
    : LogOutput()
    , m_pScheduler(pScheduler)
    , m_ulRollTimeCallbackHandle(0)
    , m_nRollTimeOffset(0)
    , m_szCurrentOpenFile(NULL)
    , m_szRollTimeAbs(NULL)
    , m_pRollCallback(NULL)
    , m_nRollSize(0)
    , m_ulPruneThreshold(0)
    , m_nByteCount(0)
    , m_lfLogFile()
    , m_pErrorLog(NULL)
{
    m_tRollTimeAbs.tv_sec = 0;
    m_tRollTimeAbs.tv_usec = 0;

    if (pContext)
    {
        pContext->QueryInterface(IID_IHXErrorMessages, (void**)&m_pErrorLog);
        m_pErrorLog->AddRef();
    }
    else
    {
        printf ("Unable to get error logging context!\n");
    }

    // Make sure we have valid filenames and paths
    if (szFilename)
    {
        m_szFilenameBase    = new char[strlen(szFilename) + 1];
        strcpy(m_szFilenameBase, szFilename);
    }
    else
    {
        m_szFilenameBase    = new char[strlen(DEFAULT_BASE_FILENAME) + 1];
        strcpy(m_szFilenameBase, DEFAULT_BASE_FILENAME);
    }
   
    struct stat logdir;
    if (szLogDirectory
        && !stat(szLogDirectory, &logdir)
        && logdir.st_mode & S_IFDIR)
    {
        m_szLogDirectory    = new char[strlen(szLogDirectory) + 1];
        strcpy(m_szLogDirectory, szLogDirectory);
    }
    else
    {
        m_szLogDirectory    = new char[strlen(".") + 1];
        strcpy(m_szLogDirectory, ".");
    }

    m_szFilenamePath    = new char[strlen(m_szLogDirectory) + strlen(m_szFilenameBase) + 2];
    
    // Assemble absolute filename
    strcpy(m_szFilenamePath, m_szLogDirectory);
    ADDSLASH(m_szFilenamePath);
    strcat(m_szFilenamePath, m_szFilenameBase);

    if (pScheduler)
        m_pScheduler->AddRef();

    m_FileMutex = HXCreateMutex();

    m_pRollCallback = new LogRollCallback(this);
    m_pRollCallback->AddRef();

    SetCurrentFile();
}


///////////////////////////////////////////////////////////////////////////////
// LogFileOutput::~LogFileOutput
///////////////////////////////////////////////////////////////////////////////

LogFileOutput::~LogFileOutput()
{
    // If we're destructing, make sure there isn't a roll scheduled
    RemoveScheduledRoll();
    HX_RELEASE(m_pRollCallback);

    HX_RELEASE(m_pScheduler);
    HX_VECTOR_DELETE(m_szFilenameBase);
    HX_VECTOR_DELETE(m_szCurrentOpenFile);
    HX_VECTOR_DELETE(m_szLogDirectory);
    HX_VECTOR_DELETE(m_szFilenamePath);
}

///////////////////////////////////////////////////////////////////////////////
// IUnknown Methods
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP_(UINT32)
LogFileOutput::AddRef(void)
{
    return InterlockedIncrement(&m_RefCount);
}


STDMETHODIMP_(UINT32)
LogFileOutput::Release(void)
{
    if (InterlockedDecrement(&m_RefCount) > 0)
    {
        return m_RefCount;
    }

    delete this;
    return 0;
}


STDMETHODIMP
LogFileOutput::QueryInterface(REFIID interfaceID,
                          void** ppInterfaceObj)
{
    // By definition all COM objects support the IUnknown interface
    if (IsEqualIID(interfaceID, IID_IUnknown))
    {
        AddRef();
        *ppInterfaceObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(interfaceID, IID_IHXLogOutput))
    {
        AddRef();
        *ppInterfaceObj = this;
        return HXR_OK;
    }
    else if (IsEqualIID(interfaceID, IID_IHXLogFileOutput))
    {
        AddRef();
        *ppInterfaceObj = this;
        return HXR_OK;
    }


    // No other interfaces are supported
    *ppInterfaceObj = NULL;
    return HXR_NOINTERFACE;
}


///////////////////////////////////////////////////////////////////////////////
// LogFileOutput::Roll
//
// Called when it's time to roll the logs if we're rolling based on time
///////////////////////////////////////////////////////////////////////////////

void
LogFileOutput::Roll()
{
    // Schedule the next roll if needed
    switch (m_RollType)
    {
    case ABSOLUTE_TIME:
        // Add a day...
        m_tRollTimeAbs.tv_sec += 24 * 60 * 60;
        ScheduleAbsoluteRoll(m_tRollTimeAbs);
        break;
    case RELATIVE_TIME:
        ScheduleRelativeRoll(m_nRollTimeOffset);
        break;
    case NO_ROLL:
    default:
        return;
        break;
    }

    Rotate();
}

///////////////////////////////////////////////////////////////////////////////
//  LogFileOutput::SetRollTimeAbs
//
//  Convert a string "hh:mm" to time_t time value.
//  Schedules a roll based on that time value.
//  Assumes military time in the local time zone.
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
LogFileOutput::SetRollTimeAbs(const char *pRollTime)
{
    HX_VECTOR_DELETE(m_szRollTimeAbs);
    
    if (pRollTime)
    {
        m_szRollTimeAbs = new char[strlen(pRollTime)+1];
        strcpy(m_szRollTimeAbs, pRollTime);
    }
    else
    {
        // If someone gave us a bogus roll time, use the default...
        m_szRollTimeAbs = new char[strlen(DEFAULT_ABSOLUTE_ROLLTIME)+1];
        strcpy(m_szRollTimeAbs, DEFAULT_ABSOLUTE_ROLLTIME);
    }

    HX_ASSERT(m_szRollTimeAbs);

    struct tm rolltime;
    HXTimeval HXcurtime = m_pScheduler->GetCurrentSchedulerTime();
    time_t curtime = HXcurtime.tv_sec;
    hx_localtime_r(&curtime, &rolltime);

    // Set the roll time...
    if (strstr(m_szRollTimeAbs, ":") == NULL)
    {
        rolltime.tm_hour = 0;    
        rolltime.tm_min  = 0;
    }
    else
    {
        rolltime.tm_hour = atoi(m_szRollTimeAbs);    
        rolltime.tm_min  = atoi(strstr(m_szRollTimeAbs, ":")+1);
    }
    rolltime.tm_sec  = 0;

    // If we've already passed the roll time today...
    if (mktime(&rolltime) < curtime)
    {
        // add a day, and set again...
        curtime += 24 * 60 * 60;
        hx_localtime_r(&curtime, &rolltime);
        rolltime.tm_hour = atoi(m_szRollTimeAbs);
        rolltime.tm_min  = atoi(strstr(m_szRollTimeAbs, ":")+1);
        rolltime.tm_sec  = 0;
    }

    // Now, convert into seconds...
    HXTimeval tVal;
    tVal.tv_usec = 0;
    tVal.tv_sec = mktime(&rolltime);

    m_tRollTimeAbs = tVal;

    RemoveScheduledRoll();
    ScheduleAbsoluteRoll(m_tRollTimeAbs);

    m_RollType = ABSOLUTE_TIME;

    return HXR_OK;
}

void
LogFileOutput::ScheduleAbsoluteRoll(HXTimeval tRollTimeAbs)
{
    if (m_pScheduler)
    {
        m_ulRollTimeCallbackHandle = m_pScheduler->AbsoluteEnter(m_pRollCallback, tRollTimeAbs);
    }
}

void
LogFileOutput::ScheduleRelativeRoll(INT32 tRollTimeOffset)
{
    if (m_pScheduler)
    {
        m_ulRollTimeCallbackHandle = m_pScheduler->RelativeEnter(m_pRollCallback, tRollTimeOffset * 60 * 1000);
    }
}

void
LogFileOutput::RemoveScheduledRoll()
{
    if (m_pScheduler && m_ulRollTimeCallbackHandle)
    {
        m_pScheduler->Remove(m_ulRollTimeCallbackHandle);
    }
}        


///////////////////////////////////////////////////////////////////////////////
//  LogFileOutput::SetRollTimeOffset
//
// Converts a string time amount to a UINT representing the amount of time
// in ms.
// Code borrowed from hxsmil/smlparse.cpp.
// Removed support for npt/smtpe time.
// Supported formats are: "hh:mm:ss", "#h", "#min", "#s", "#ms".
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
LogFileOutput::SetRollTimeOffset(IHXBuffer* pRollTimeOffset)
{
    HX_ASSERT(pRollTimeOffset);

    const char* szTime = (const char*)pRollTimeOffset->GetBuffer();

    INT32 nHours = 0;
    INT32 nDays = 0;
    INT32 nWeeks = 0;
    INT32 nMonths = 0;
    INT32 nCount = 0;

    if (!szTime)
    {
        m_nRollTimeOffset = 0;
        return HXR_INVALID_PARAMETER;
    }

    while (*szTime)
    {
        char ch = tolower(*szTime);

        if ((ch == 'h' || ch == 'd' || ch == 'w' || ch == 'm') &&
            (nHours || nDays || nWeeks || nMonths))
        {
            // Already encountered nHours, nDays, nWeeks, or nMonths unit
            m_nRollTimeOffset = 0;
            return HXR_INVALID_PARAMETER;
        }

        switch(ch)
        {
            case 'h':
                nHours = nCount;
                nCount = 0;
                break;

            case 'd':
                nDays = nCount;
                nCount = 0;
                break;

            case 'w':
                nWeeks = nCount;
                nCount = 0;
                break;

            case 'm':
                nMonths = nCount;
                nCount = 0;
                break;

            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                nCount = nCount * 10 + (*szTime - '0');
                break;

            default:
                m_nRollTimeOffset = 0;
                return HXR_OK;
        }
        szTime++;
    }

    if (!nHours && !nDays && !nWeeks && !nMonths)
    {
        m_nRollTimeOffset = 0;
        return HXR_INVALID_PARAMETER;
    }

    m_nRollTimeOffset = ((nMonths * DAYS_PER_MONTH * HOURS_PER_DAY)
        + (nWeeks * DAYS_PER_WEEK * HOURS_PER_DAY)        
        + (nDays * HOURS_PER_DAY)
        + nHours) * MINUTES_PER_HOURS;

    m_RollType = RELATIVE_TIME;

    // Schedule the first roll
    if (m_nRollTimeOffset)
    {
        RemoveScheduledRoll();
        ScheduleRelativeRoll(m_nRollTimeOffset);
    }

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
//  LogFileOutput::SetRollTimeOffset
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
LogFileOutput::SetRollTimeOffset(UINT32 nRollTimeOffset)
{
    m_nRollTimeOffset = nRollTimeOffset;

    m_RollType = RELATIVE_TIME;

    if (m_nRollTimeOffset)
    {
        RemoveScheduledRoll();
        ScheduleRelativeRoll(m_nRollTimeOffset);
    }

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
//  LogFileOutput::SetRollSize
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
LogFileOutput::SetRollSize(UINT32 nRollSize)
{
    m_RollType = FILE_SIZE;
    m_nRollSize = nRollSize;

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
//  LogFileOutput::SetNoRoll
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
LogFileOutput::SetNoRoll()
{
    m_RollType = NO_ROLL;

    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
//  LogFileOutput::SetMaxLogSpace
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
LogFileOutput::SetPruneThreshold(UINT32 ulPruneThreshold)
{
    m_ulPruneThreshold = ulPruneThreshold;
    return HXR_OK;
}


///////////////////////////////////////////////////////////////////////////////
//  LogFileOutput::GetBaseFilename
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(const char*)
LogFileOutput::GetBaseFilename()
{
    return m_szFilenameBase;
}

///////////////////////////////////////////////////////////////////////////////
//  LogFileOutput::SetBaseFilename
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
LogFileOutput::SetBaseFilename(const char *szBaseFilename)
{
    if (!szBaseFilename)
    {
        return HXR_INVALID_PARAMETER;
    }

    HX_VECTOR_DELETE(m_szFilenameBase);
    m_szFilenameBase = new char[strlen(szBaseFilename) + 1];
    strcpy(m_szFilenameBase, szBaseFilename);

    HX_VECTOR_DELETE(m_szFilenamePath);
    m_szFilenamePath    = new char[strlen(m_szLogDirectory) + strlen(m_szFilenameBase) + 2];
    strcpy(m_szFilenamePath, m_szLogDirectory);
    ADDSLASH(m_szFilenamePath);
    strcat(m_szFilenamePath, m_szFilenameBase);

    Rotate();

    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
//  LogFileOutput::GetBaseFilename
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(const char*)
LogFileOutput::GetLogDirectory()
{
    return m_szLogDirectory;
}


///////////////////////////////////////////////////////////////////////////////
//  LogFileOutput::SetLogDirectory
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
LogFileOutput::SetLogDirectory(const char *szLogDirectory)
{
    if (!szLogDirectory)
    {
        return HXR_INVALID_PARAMETER;
    }

    struct stat logdir;
    if (szLogDirectory
        && !stat(szLogDirectory, &logdir)
        && logdir.st_mode & S_IFDIR)
    {
        HX_VECTOR_DELETE(m_szLogDirectory);
        m_szLogDirectory    = new char[strlen(szLogDirectory) + 1];
        strcpy(m_szLogDirectory, szLogDirectory);
    }
    else
    {
        return HXR_INVALID_PARAMETER;
    }

    HX_VECTOR_DELETE(m_szFilenamePath);
    m_szFilenamePath = new char[strlen(m_szLogDirectory) + strlen(m_szFilenameBase) + 2];
    strcpy(m_szFilenamePath, m_szLogDirectory);
    ADDSLASH(m_szFilenamePath);
    strcat(m_szFilenamePath, m_szFilenameBase);

    Rotate();   

    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
// LogFileOutput::WantToRotate
///////////////////////////////////////////////////////////////////////////////

BOOL
LogFileOutput::WantToRotate(const char *pString)
{
    if (!m_szFilenameBase)
        return FALSE;

    if (m_nRollSize && ((INT32)(m_nByteCount + strlen(pString)) >= m_nRollSize * 1024))
        return TRUE;

    return FALSE;
}


///////////////////////////////////////////////////////////////////////////////
// LogFileOutput::Rotate
///////////////////////////////////////////////////////////////////////////////

void
LogFileOutput::Rotate()
{
    // If we're dumping to stdout don't worry about rotating...
    if (m_szFilenameBase == NULL)
        return;

    // If we're concerned with max disk consumption...
    if (m_ulPruneThreshold)
    {
        UINT32 ulTotalSize = GetLogDiskUsage();
        UINT32 ulSize;
        int i = 0;

        while (ulTotalSize / (double)1024 > (double)m_ulPruneThreshold)
        {
            // Delete logs until we're under the threshold
            if (SUCCEEDED(DeleteOldestLog(&ulSize)))
            {
                ulTotalSize -= ulSize;
            }
            else
            {
                goto abort_pruning;
            }
        }
    }

abort_pruning:

    SetCurrentFile();

    m_nByteCount = 0;
}

///////////////////////////////////////////////////////////////////////////////
//  LogFileOutput::GetMutex
//
//  Used for processes to add the mutex to the managed mutex list
///////////////////////////////////////////////////////////////////////////////
HX_MUTEX
LogFileOutput::GetMutex()
{
    return m_FileMutex;
}


///////////////////////////////////////////////////////////////////////////////
//  LogFileOutput::GetLogDiskUsage()
///////////////////////////////////////////////////////////////////////////////

UINT32
LogFileOutput::GetLogDiskUsage()
{
    UINT32 ulTotalSize = 0;
    char *szFilenamePattern = new char[strlen(m_szFilenameBase)+2];
    // ...+2]: 1 extra for '\0', 1 for '*'
    strcpy(szFilenamePattern, m_szFilenameBase);
    strcat(szFilenamePattern, "*");
    CFindFile *pFindFile = CFindFile::CreateFindFile(m_szLogDirectory, 0, szFilenamePattern);
    char *pFilename;
    char *pAbsoluteFilename;
    struct stat filestats;


    pFilename = pFindFile->FindFirst();

    while (pFilename)
    {
        pAbsoluteFilename = new char[strlen(m_szLogDirectory) + strlen(pFilename) + 2];
        strcpy(pAbsoluteFilename, m_szLogDirectory);
        ADDSLASH(pAbsoluteFilename);
        strcat(pAbsoluteFilename, pFilename);

        if (!stat(pAbsoluteFilename, &filestats))
        {
            ulTotalSize += filestats.st_size;
        }

        HX_VECTOR_DELETE(pAbsoluteFilename);

        pFilename = pFindFile->FindNext();
    }

    return ulTotalSize;
}

///////////////////////////////////////////////////////////////////////////////
//  LogFileOutput::DeleteOldestLog()
//
//  Delete the oldest log.  Sets pFreedSpace equal to the amount of
//  space freed up by the file.
///////////////////////////////////////////////////////////////////////////////

HX_RESULT
LogFileOutput::DeleteOldestLog(UINT32 *pFreedSpace)
{
    char *szFilenamePattern = new char[strlen(m_szFilenameBase)+2];
    // ...+2]: 1 extra for '\0', 1 for '*'
    strcpy(szFilenamePattern, m_szFilenameBase);
    strcat(szFilenamePattern, "*");

    CFindFile *pFindFile = CFindFile::CreateFindFile(m_szLogDirectory, 0, szFilenamePattern);
    char *pFilename;
    char *pOldest = NULL;
    char *pAbsoluteFilename;
    struct stat filestats;
    struct stat oldeststats;

    *pFreedSpace = 0;

    HX_ASSERT(pFindFile);

    pFilename = pFindFile->FindFirst();

    while (pFilename)
    {
        // if we don't have a base filename, get out
        if (!m_szFilenameBase)
        {
            goto return_failed;
        }

        pAbsoluteFilename = new char[strlen(m_szLogDirectory) + strlen(pFilename) + 2];
        strcpy(pAbsoluteFilename, m_szLogDirectory);
        ADDSLASH(pAbsoluteFilename);
        strcat(pAbsoluteFilename, pFilename);

        if (!stat(pAbsoluteFilename, &filestats))
        {
            // (if the current file is older,
            //  or this is the first "log" file we've found (ie: pOlder == NULL))
            // and current file is NOT a directory
            if ( (filestats.st_mtime < oldeststats.st_mtime
                  || pOldest == NULL)
                 && !(filestats.st_mode & S_IFDIR))
            {
                HX_VECTOR_DELETE(pOldest);

                pOldest = new char[strlen(pAbsoluteFilename)+1];
                if (pOldest == NULL)
                {
                    goto return_failed;
                }

                strcpy(pOldest, pAbsoluteFilename);
                oldeststats = filestats;
            }
        }
        else
        {
            goto return_failed;
        }

        HX_VECTOR_DELETE(pAbsoluteFilename);

        pFilename = pFindFile->FindNext();
    }

    // Just to be sure we're not deleting a directory...
    if (pOldest && !(oldeststats.st_mode & S_IFDIR))
    {
        *pFreedSpace = oldeststats.st_size;
        remove(pOldest);
    }

    HX_VECTOR_DELETE(pAbsoluteFilename);
    HX_VECTOR_DELETE(pOldest);
    return HXR_OK;

return_failed:

    HX_VECTOR_DELETE(pAbsoluteFilename);
    HX_VECTOR_DELETE(pOldest);
    return HXR_FAILED;
}


///////////////////////////////////////////////////////////////////////////////
//  LogFileOutput::SetCurrentFile
//  
//  Change m_szCurrentOpenFile to the new file...
///////////////////////////////////////////////////////////////////////////////

HX_RESULT
LogFileOutput::SetCurrentFile()
{
    if (!m_szFilenameBase)
    {
        return HXR_FAIL;
    }

    HXMutexLock(m_FileMutex, TRUE);   
    HX_VECTOR_DELETE(m_szCurrentOpenFile);
    m_szCurrentOpenFile = new char[strlen(m_szFilenamePath) + 80];
    strcpy(m_szCurrentOpenFile, m_szFilenamePath);

    char szExtension[80];
    struct tm tm;
    time_t now;
    ::time(&now);
    hx_localtime_r(&now, &tm);
    strftime(szExtension, sizeof(szExtension), ".%Y%m%d%H%M%S", &tm);

    strcat(m_szCurrentOpenFile, szExtension);
    HXMutexUnlock(m_FileMutex);

    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
// LogFileOutput::Output
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
LogFileOutput::Output(const char *pString)
{
    // If we're dumping to stdout, just printf it
    if (m_szFilenameBase == NULL)
    {
        printf("%s", pString);
        return HXR_OK;
    }

    // Check filesize for rotation
    if (WantToRotate(pString) == TRUE)
    {
        Rotate();
    }

    // (re)open the logfile, if required
    HXMutexLock(m_FileMutex, TRUE);   
    if (m_szCurrentOpenFile)
    {
        m_lfLogFile.SetFileName(m_szCurrentOpenFile);
    }
    HXMutexUnlock(m_FileMutex);

    m_lfLogFile.Write(pString);
    m_lfLogFile.Flush();
    m_nByteCount = m_lfLogFile.GetLogFileSize();

    if (!m_lfLogFile.IsOK())
    {
        char msg[MAX_ERROR_STRING_LENGTH];
        snprintf (msg, sizeof(msg), "RSS log error: (%lu) %s\n",
            m_lfLogFile.GetLastErrorCode(), m_lfLogFile.GetLastErrorStr());
        msg[sizeof(msg) - 1] = '\0';

        if (m_pErrorLog)
        {
            ERRMSG(m_pErrorLog, msg);
        }
        else
        {
            printf(msg);
        }

        return HXR_FAIL;
    }

    return HXR_OK;
}

///////////////////////////////////////////////////////////////////////////////
// LogFileOutput::Output
///////////////////////////////////////////////////////////////////////////////

STDMETHODIMP
LogFileOutput::Output(IHXBuffer* pOutput)
{
    return Output((const char*)pOutput->GetBuffer());
}

///////////////////////////////////////////////////////////////////////////////
// LogFileOutput::CrashOutput
//
// This method is for getting stack traces and heartbeat failures in the
// RSS logs.
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
LogFileOutput::CrashOutput(const char *pString)
{
    return Output(pString);
}


///////////////////////////////////////////////////////////////////////////////
//  LogFileOutput::LogRollCallback::LogRollCallback
//
//  Callback for handling log rolls
///////////////////////////////////////////////////////////////////////////////

LogFileOutput::LogRollCallback::LogRollCallback(LogFileOutput *pParent)
    : m_RefCount(0)
{
    m_pParent = pParent;
}

LogFileOutput::LogRollCallback::~LogRollCallback()
{
}

STDMETHODIMP
LogFileOutput::LogRollCallback::Func()
{
    m_pParent->Roll(); 
    
    return HXR_OK;
}


STDMETHODIMP_(UINT32)
LogFileOutput::LogRollCallback::AddRef(void)
{
    return InterlockedIncrement(&m_RefCount);
}


STDMETHODIMP_(UINT32)
LogFileOutput::LogRollCallback::Release(void)
{
    if (InterlockedDecrement(&m_RefCount) > 0)
    {
        return m_RefCount;
    }

    delete this;
    return 0;
}


STDMETHODIMP
LogFileOutput::LogRollCallback::QueryInterface(REFIID interfaceID,
                          void** ppInterfaceObj)
{
    // By definition all COM objects support the IUnknown interface
    if (IsEqualIID(interfaceID, IID_IUnknown))
    {
        AddRef();
        *ppInterfaceObj = (IUnknown*)this;
        return HXR_OK;
    }

    // No other interfaces are supported
    *ppInterfaceObj = NULL;
    return HXR_NOINTERFACE;
}
