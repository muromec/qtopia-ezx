/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: logoutputs.h,v 1.5 2006/03/22 19:45:37 tknox Exp $
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

#include "base_callback.h"
#include "servlist.h"
#include "hxmon.h"
#include "hxerror.h"
#include "hxcomm.h"
#include "hxlogoutputs.h"

#include "logfile.h"

// Forward declarations

class Process;

///////////////////////////////////////////////////////////////////////////////
// LogOutput class
///////////////////////////////////////////////////////////////////////////////

class LogOutput
    : public HXListElem
    , public IHXLogOutput
{

protected:

    UINT32 m_RefCount;  // Object's reference count
    UINT32 m_ulRegId;
    BOOL m_bDeleted;    // True if this output was deleted.
    BOOL m_bModified;    // True if this output was modified.

public:


    LogOutput();
    virtual ~LogOutput();


    // IUnknown COM Interface Methods
    STDMETHODIMP QueryInterface(REFIID ID,
                                void** ppInterfaceObj);

    STDMETHODIMP_(UINT32) AddRef();
    STDMETHODIMP_(UINT32) Release();

    STDMETHOD(Output)           (THIS_
                                 IHXBuffer* pBuffer);
    STDMETHOD(Output)           (THIS_
                                 const char *pString);


    void SetRegId(UINT32 ulId) { m_ulRegId = ulId; }
    UINT32 GetRegId() { return m_ulRegId; }

    void SetDeleted(BOOL bDeleted) { m_bDeleted = bDeleted; }
    BOOL IsDeleted() { return m_bDeleted; }

    void SetModified(BOOL bModified) { m_bModified = bModified; }
    BOOL IsModified() { return m_bModified; }
};


///////////////////////////////////////////////////////////////////////////////
// LogFileOutput Class
///////////////////////////////////////////////////////////////////////////////

enum RollTypes
{
    ABSOLUTE_TIME = 0,
    RELATIVE_TIME,
    FILE_SIZE,
    NO_ROLL
};

#define DEFAULT_ABSOLUTE_ROLLTIME  "00:00"
#define DEFAULT_BASE_FILENAME      "logs"

class LogFileOutput
    : public LogOutput
    , public IHXLogFileOutput
{
    class LogRollCallback : public BaseCallback
    {
    public:

        LogRollCallback(LogFileOutput *pParent);
        ~LogRollCallback();

        // IUnknown Methods
        STDMETHODIMP QueryInterface(REFIID ID, void** ppInterfaceObj);
        STDMETHODIMP_(UINT32) AddRef();
        STDMETHODIMP_(UINT32) Release();

        // Callback Func
        STDMETHODIMP Func();
    protected:
        LogFileOutput *m_pParent;
        UINT32 m_RefCount;
    };

protected:

    char* m_szFilenameBase;     // Base log filename
    char* m_szFilenamePath;     // Log directory and base filename
    char* m_szCurrentOpenFile;  // Current open file (full path)
    char* m_szLogDirectory;     // Log directory
 
    HX_MUTEX    m_FileMutex;   // Used to protect m_szCurrentOpenFile

    INT32 m_nByteCount;
    INT32 m_nRollTimeOffset;    // Time offset at which to roll-- in MINUTES.
    HXTimeval m_tRollTimeAbs;   // Time (absolute) at which to roll-- "hh:mm"
    char* m_szRollTimeAbs;      // char* representation of the Abs Roll Time
    INT32 m_nRollSize;          // Size at which to roll-- in KILOBYTES.
    UINT32 m_ulPruneThreshold;   // Maximum disk consumption-- in KILOBYTES
                                // threshold value for pruning old logs
    RollTypes m_RollType;

    IHXScheduler* m_pScheduler;
    CallbackHandle m_ulRollTimeCallbackHandle;

    LogRollCallback *m_pRollCallback;
    IHXErrorMessages* m_pErrorLog;

public:

    LogFileOutput();
    LogFileOutput(const char* pFilename, const char* szLogDirectory, IHXScheduler* pScheduler, IUnknown* pContext);

    ~LogFileOutput();

    // IUnknown Methods
    STDMETHODIMP QueryInterface(REFIID ID, void** ppInterfaceObj);
    STDMETHODIMP_(UINT32) AddRef();
    STDMETHODIMP_(UINT32) Release();


    // IHXLogOutput Methods
    STDMETHODIMP Output(IHXBuffer* pBuffer);

    STDMETHODIMP Output(const char* pString);

    STDMETHODIMP CrashOutput(const char *pString); 

    UINT32 GetLogDiskUsage();
    HX_RESULT DeleteOldestLog(UINT32 *ulSize);    
    
    // Log Pruning Functions
    STDMETHODIMP_(UINT32) GetPruneThreshold() { return m_ulPruneThreshold; }
    STDMETHODIMP SetPruneThreshold(UINT32 ulPruneThreshold);

    // Size Rolling Functions
    STDMETHODIMP_(INT32) GetRollSize() { return m_nRollSize; }
    STDMETHODIMP SetRollSize(UINT32 nRollSize);

    // Relative Time Rolling Functions
    STDMETHODIMP_(INT32) GetRollTimeOffset() { return m_nRollTimeOffset; }
    STDMETHODIMP SetRollTimeOffset(UINT32 nRollTimeOffset);
    STDMETHODIMP SetRollTimeOffset(IHXBuffer* pRollTimeOffset);
    void ScheduleRelativeRoll(INT32 tRollTimeOffset);

    // Absolute Time Rolling Functions
    STDMETHODIMP_(const char*) GetRollTimeAbs() { return m_szRollTimeAbs; }
    STDMETHODIMP SetRollTimeAbs(const char *pRollTimeAbs);
    void ScheduleAbsoluteRoll(HXTimeval tRollTimeAbs);

    // No Rolling
    STDMETHODIMP SetNoRoll();

    // Set BaseFilename
    STDMETHODIMP_(const char*) GetBaseFilename();
    STDMETHODIMP SetBaseFilename(const char *szBaseFilename);

    // Set Log Directory
    STDMETHODIMP_(const char*) GetLogDirectory();
    STDMETHODIMP SetLogDirectory(const char *szLogDirectory);

    HX_MUTEX GetMutex();
    void Roll();
    void RemoveScheduledRoll();
    BOOL WantToRotate(const char *pString);
    void Rotate();


    HX_RESULT SetCurrentFile();

    const char* GetLastErrorStr() const { return m_lfLogFile.GetLastErrorStr(); }
    UINT32 GetLastErrorCode() const { return m_lfLogFile.GetLastErrorCode(); }

private:
    LogFile     m_lfLogFile;
};
