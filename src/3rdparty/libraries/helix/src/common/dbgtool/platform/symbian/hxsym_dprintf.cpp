/*****************************************************************************
 * hxsym_log_util.cpp
 * ---------------
 *
 *
 * Target:
 * Symbian OS
 *
 *
 * (c) 1995-2003 RealNetworks, Inc. Patents pending. All rights reserved.
 *
 *****************************************************************************/

// Symbian includes... 
#include <e32base.h>
#include <e32def.h>
#include <e32svr.h>
#include <string.h>
#include <stdarg.h>

#include "safestring.h"
#include "hxtypes.h"
#include "hxtime.h"
#include "hxcom.h"
#include "hxprefs.h"
#include "hxprefutil.h"
#include "hxassert.h"
#include "hxsym_console_util.h"
#include "hxsym_dprintf.h"

#if !defined (HELIX_FEATURE_DPRINTF)
#error file should not be included in build
#endif


namespace
{

// called by dprintf
void dprintfHelper(const char* fmt, va_list args)
{ 
    const UINT32 BUF_SIZE = 4096;

    CHXString str;
    char *pszBuf = str.GetBuffer(BUF_SIZE);
    if (!pszBuf)
    {
	return;
    }

    TInt idxBuf = 0;

    DPrintfData* pData = dprintfGetData();
    if (!pData)
    {
        return;
    }

    //
    // print dprint header
    //

    // get date and time (assume we'll be printing)
    HXTime now;
    gettimeofday(&now, 0);
    struct tm* tm = localtime((const time_t *)&now.tv_sec);

    if(pData->printFlags & PRINT_DATE)
    {
        idxBuf += strftime(pszBuf + idxBuf, BUF_SIZE - idxBuf, "%d-%b-%y ", tm);
    }
    if(pData->printFlags & PRINT_TIME)
    {
        if(pData->printFlags & PRINT_TIME_INCLUDE_MS)
        {
            idxBuf += strftime(pszBuf + idxBuf, BUF_SIZE - idxBuf, "%H:%M:%S", tm);
            idxBuf += SafeSprintf(pszBuf + idxBuf, BUF_SIZE - idxBuf, ":%03lu ", now.tv_usec/1000);
        }
        else
        {
            idxBuf += strftime(pszBuf + idxBuf, BUF_SIZE - idxBuf, "%H:%M:%S ", tm);
        }
    }
    if(pData->printFlags & PRINT_TID)
    {
        TThreadId tid = RThread().Id();
        idxBuf += SafeSprintf(pszBuf + idxBuf, BUF_SIZE - idxBuf, "(%d) ", tid);
    }

    //
    // print text
    //
    HX_ASSERT(BUF_SIZE - idxBuf > 0);
    vsnprintf(pszBuf + idxBuf, BUF_SIZE - idxBuf, fmt, args);

    CHXString strName = pData->sinkName;
    if(!strName.IsEmpty())
    {
        if( 0 == strName.CompareNoCase("console") )
        {
            // send to console
            PrintConsole(pszBuf);
        }
        else
        {
            // send to logfile
            FILE* file = fopen(strName, "a+");
            if( file )
            {
                fprintf(file, pszBuf);
                fclose(file);
            }
        }
    } 
}

} // locals

static void destroyDPrintfData(void* pObj)
{
    DPrintfData* pData = (DPrintfData*)pObj;
    delete pData;
}

DPrintfData* dprintfGetData()
{
    static const INT32 key = 0;

    HXGlobalManager* pGM = HXGlobalManager::Instance();

    DPrintfData** ppData = reinterpret_cast<DPrintfData**>(pGM->Get(&key));
    DPrintfData* pRet = NULL;
    
    if (!ppData)
    {
	    pRet = new DPrintfData();

	    if (pRet)
	    {
	        pGM->Add(&key, pRet, &destroyDPrintfData);
	    }
    }
    else
    {
	    pRet = *ppData;
    }

    return pRet;
}

UINT32& debug_level()
{
    return dprintfGetData()->mask;
}

UINT32& debug_func_level()
{
    return dprintfGetData()->funcTraceMask;
}

UINT32 dprintfGetMask()
{
    UINT32 mask = 0;
    DPrintfData* pData = dprintfGetData();
    if( pData )
    {
        mask = pData->mask;
    }
    return mask;
}

void dprintfInit(IHXPreferences* pPrefs)
{
    if (pPrefs)
    {
        CHXString sink;
        UINT32 mask = 0;
        ReadPrefCSTRING(pPrefs, "LogSink", sink);
        ReadPrefUINT32(pPrefs, "DebugMask", mask);

        DPrintfData* pData = dprintfGetData();
        if (pData)
        {
            pData->mask = mask;
            pData->sinkName = sink;
            pData->printFlags = (PRINT_TIME | PRINT_TIME_INCLUDE_MS | PRINT_TID);
        }
    }
}

//
// called by DPRINTF macro
//
void dprintf(const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    dprintfHelper(fmt, args);
    va_end(args);
}


