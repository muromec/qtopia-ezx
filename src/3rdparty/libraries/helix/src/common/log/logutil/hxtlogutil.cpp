/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#ifdef _WIN32
#pragma warning (disable : 4786)
#endif

#include "hxtlogutil.h"
#include "hlxclib/stdarg.h"
#include "hxplugn.h"
#include "hxassert.h"

#if defined(HELIX_CONFIG_LOGGING_USE_FPRINTF)

void HXLogVfprintf(EHXTLogFuncArea nFuncArea, const char* szMsg, va_list argptr)
{
#if defined(HELIX_CONFIG_LOGGING_USE_FPRINTF_FILENAME)
    FILE* fp = fopen(HELIX_CONFIG_LOGGING_USE_FPRINTF_FILENAME, "a");
#else
    FILE* fp = stdout;
#endif

    // Print out the log message
    vfprintf(fp, szMsg, argptr);
    // HXLOGLx() messages don't have a trailing "\n" so we must write one out
    fprintf(fp, "\n");

    // If we opened a file, then we need to flush and close it
#if defined(HELIX_CONFIG_LOGGING_USE_FPRINTF_FILENAME)
    fclose(fp); // fclose() flushes before closing so no need to fflush()
#endif
}

void HXLogFprintf(EHXTLogFuncArea nFuncArea, const char* szMsg, ...)
{
    va_list VariableArguments;
    va_start(VariableArguments, szMsg);
    HXLogVfprintf(nFuncArea, szMsg, VariableArguments);
    va_end(VariableArguments);
}

void HXLogFprintf(const char* szMsg, ...)
{
    va_list VariableArguments;
    va_start(VariableArguments, szMsg);
    HXLogVfprintf(HXLOG_GENE, szMsg, VariableArguments); // Use generic if no functional area provided
    va_end(VariableArguments);
}

#else /* #if defined(HELIX_FEATURE_LOGGING_USE_FPRINTF) */

#if defined(HELIX_FEATURE_LOG_STATICDLLACCESS)
#include "dllpath.h"
#include "dllacces.h"
#include "hxthread.h"
#include "hxscope_lock.h"
static
HXMutex* CreationMutexInstance()
{
    static HXMutex* pMutex;
    if (!pMutex)
    {
        HXMutex::MakeMutex(pMutex);
    }
    return pMutex;
}

static DLLAccess g_LogDLL;
BOOL             g_bTriedInit = FALSE;
HXMutex*         g_pCreationMutex = CreationMutexInstance();

#endif /* #if defined(HELIX_FEATURE_LOG_STATICDLLACCESS) */
#include "hlxclib/string.h"


#if !defined(HELIX_CONFIG_NOSTATICS)
static IHXDllAccess*            g_pDllAccess = NULL;
static IHXTInternalLogWriter*   g_pLogWriter = NULL;
static IHXTLogSystemContext*    g_pLogSystemContext = NULL;
static IHXTLogSystem*           g_pLogSystem = NULL;
#else
#include "hxglobalmgr_utils.h"

HX_DEFINE_GLOBAL_PTR_FUNCS_COM(IHXTLogSystemContext)
HX_DEFINE_GLOBAL_KEY(g_pLogSystemContext)
#define g_pLogSystemContext MAKE_GLOBAL_PTR_ALIAS_COM(IHXTLogSystemContext, g_pLogSystemContext)

HX_DEFINE_GLOBAL_PTR_FUNCS_COM(IHXTLogSystem)
HX_DEFINE_GLOBAL_KEY(g_pLogSystem)
#define g_pLogSystem MAKE_GLOBAL_PTR_ALIAS_COM(IHXTLogSystem, g_pLogSystem)

HX_DEFINE_GLOBAL_PTR_FUNCS_COM(IHXTInternalLogWriter)
HX_DEFINE_GLOBAL_KEY(g_pLogWriter)
#define g_pLogWriter MAKE_GLOBAL_PTR_ALIAS_COM(IHXTInternalLogWriter, g_pLogWriter)

#if !defined(HELIX_FEATURE_CORE_LOG)
HX_DEFINE_GLOBAL_PTR_FUNCS_COM(IHXDllAccess)
HX_DEFINE_GLOBAL_KEY(g_pDllAccess)
#define g_pDllAccess MAKE_GLOBAL_PTR_ALIAS_COM(IHXDllAccess, g_pDllAccess)
#endif

#endif // HELIX_CONFIG_NOSTATICS

HX_RESULT DoLogWriterInit()
{
    HX_RESULT res = HXR_FAIL;
    if(g_pLogSystem != NULL)
    {
        res = g_pLogSystem->QueryInterface(IID_IHXTLogSystemContext, (void**)&g_pLogSystemContext);
        if (SUCCEEDED(res))
        {       
            IHXTLogWriter* pLogWriter = 0;
            res = g_pLogSystem->GetWriterInterface(&pLogWriter);
            if (SUCCEEDED(res))
            {
                res = pLogWriter->QueryInterface(IID_IHXTInternalLogWriter, (void**)&g_pLogWriter);
                HX_RELEASE(pLogWriter);
            }
        }
    } // End of if(g_pLogSystem != NULL)

    return res;
}

static
HX_RESULT DoLogSystemInterfaceInit(FPCREATELOGSYSTEMINTERFACE fpCreateLogSystem, IUnknown* pContext)
{
    if (!fpCreateLogSystem)
    {
        return HXR_FAIL;
    }

    HX_RESULT res = (*fpCreateLogSystem)(&g_pLogSystem);
    if (SUCCEEDED(res))
    {
#if defined(HELIX_FEATURE_CLIENT)
        // Call InitPlugin() on the log system plugin we just loaded
        IHXPlugin* pPlug = 0;
        res = g_pLogSystem->QueryInterface(IID_IHXPlugin, (void**)&pPlug);
        if (HXR_OK == res)
        {
            HX_ASSERT(pContext);
            res = pPlug->InitPlugin(pContext);
            HX_RELEASE(pPlug);
        }

#endif
        res = DoLogWriterInit();
    }
    return res;
}

// These functions will attempt to create a DLLAccess
// class and therefore you must link against DLLAccess
// in common_system before they will work
#if defined(HELIX_FEATURE_LOG_STATICDLLACCESS)

void Init_Logging(IHXTLogWriter** ppILog)
{	
    HXScopeLock lock(g_pCreationMutex);
    
    g_bTriedInit = TRUE;
    
    if (!g_pLogSystem)
    {
        UINT32 ulNameLen = 255;
        CHXString strDllName;
        DLLAccess::CreateName("log", "log", strDllName.GetBuffer(ulNameLen+1), ulNameLen, 0, 0);
        strDllName.ReleaseBuffer();

        FPCREATELOGSYSTEMINTERFACE fpCreateLogSystem = NULL;
        if (g_LogDLL.isOpen() || g_LogDLL.open(strDllName, DLLTYPE_ENCSDK) == DLLAccess::DLL_OK)
        {
            fpCreateLogSystem = (FPCREATELOGSYSTEMINTERFACE)(g_LogDLL.getSymbol("RMACreateLogSystem"));
        }

        // Note: Application is reponsible should display warning msg that logging 
        // failed to load.  SDK plugins should not printf anything
        DoLogSystemInterfaceInit(fpCreateLogSystem, NULL);
        if (g_pLogWriter)
        {
            g_pLogWriter->QueryInterface(IID_IHXTLogWriter, (void**)ppILog);
        }
    } 
}


void RSLog(UINT32 nMsg, const char* szMsg, ...)
{
    if( !g_bTriedInit )
    {
        Init_Logging(NULL);
    }
    if( g_pLogWriter )
    {
        va_list VariableArguments;
        va_start(VariableArguments, szMsg);
        g_pLogWriter->LogMessage("RealNetworks", LC_DEV_DIAG, NONE, nMsg, szMsg, VariableArguments);
        va_end(VariableArguments);
    }
}

void RSLog(EHXTLogCode nLogCode, UINT32 nMsg, const char* szMsg, ...)
{
    if( !g_bTriedInit )
    {
        Init_Logging(NULL);
    }
    if( g_pLogWriter )
    {
        va_list VariableArguments;
        va_start(VariableArguments, szMsg);
        g_pLogWriter->LogMessage("RealNetworks", nLogCode, NONE, nMsg, szMsg, VariableArguments);
        va_end(VariableArguments);
    }
}

void RSLog(EHXTLogCode nLogCode, EHXTLogFuncArea nFuncArea, UINT32 nMsg, const char* szMsg, ...)
{
    if( !g_bTriedInit )
    {
        Init_Logging(NULL);
    }
    if( g_pLogWriter )
    {
        va_list VariableArguments;
        va_start(VariableArguments, szMsg);
        g_pLogWriter->LogMessage("RealNetworks", nLogCode, nFuncArea, nMsg, szMsg, VariableArguments);
        va_end(VariableArguments);
    }
}

void RSLog(const char* szMsg, ...) 
{	
    if( !g_bTriedInit )
    {
        Init_Logging(NULL);
    }
    if( g_pLogWriter )
    {
        va_list VariableArguments;
        va_start(VariableArguments, szMsg);
        g_pLogWriter->LogMessage("RealNetworks", LC_DEV_DIAG, NONE, MAX_UINT32, szMsg, VariableArguments);
        va_end(VariableArguments);
    }
}

void RSLog(EHXTLogCode nLogCode, const char* szMsg, ...) 
{
    if( !g_bTriedInit )
    {
        Init_Logging(NULL);
    }
    if( g_pLogWriter )
    {
        va_list VariableArguments;
        va_start(VariableArguments, szMsg);
        g_pLogWriter->LogMessage("RealNetworks", nLogCode, NONE, MAX_UINT32, szMsg, VariableArguments);
        va_end(VariableArguments);
    }
}

void RSLog(EHXTLogCode nLogCode, EHXTLogFuncArea nFuncArea, const char* szMsg, ...) 
{
    if( !g_bTriedInit )
    {
        Init_Logging(NULL);
    }
    if( g_pLogWriter )
    {
        va_list VariableArguments;
        va_start(VariableArguments, szMsg);
        g_pLogWriter->LogMessage("RealNetworks", nLogCode, nFuncArea, MAX_UINT32, szMsg, VariableArguments);
        va_end(VariableArguments);
    }
}

void HXTLog_Push_Context(const char* szContext)
{
    if( !g_pLogSystemContext )
    {
        Init_Logging(NULL);
    }
    
    if(g_pLogSystemContext)
    {
        g_pLogSystemContext->PushContext(szContext);
    }
}

void HXTLog_Pop_Context()
{
    if( !g_pLogSystemContext )
    {
        Init_Logging(NULL);
    }
    
    if(g_pLogSystemContext)
    {
        g_pLogSystemContext->PopContext();
    }
}

void HXTLog_Set_FileAndLine(const char * szFilename, int nLineNum)
{
    if( !g_pLogSystemContext )
    {
        Init_Logging(NULL);
    }
    
    if(g_pLogSystemContext)
    {
        g_pLogSystemContext->SetThreadFileAndLine(szFilename, nLineNum);
    }
}

void HXTLog_SetThreadJobName(const char* szJobName, UINT32 nThreadId)
{
    if( !g_pLogSystemContext )
    {
        Init_Logging(NULL);
    }
    
    if(g_pLogSystemContext)
    {
        g_pLogSystemContext->SetThreadJobName(szJobName, nThreadId);
    }
}

void HXTLog_CreateChildThread(UINT32 nParentThreadId, UINT32 nChildThreadId)
{
    if( !g_pLogSystemContext )
    {
        Init_Logging(NULL);
    }
    
    if(g_pLogSystemContext)
    {
        g_pLogSystemContext->SetParentChildRelationship(nParentThreadId, nChildThreadId);
    }
}

void HXTLog_EndThread(UINT32 nThreadId)
{
    if( !g_pLogSystemContext )
    {
        Init_Logging(NULL);
    }
    
    if(g_pLogSystemContext)
    {
        g_pLogSystemContext->EndThread(nThreadId);
    }
}

#endif /* #if defined(HELIX_FEATURE_LOG_STATICDLLACCESS) */

HXBOOL 
HXLoggingEnabled() 
{ 
    return (g_pLogWriter && g_pLogWriter->IsEnabled()) ? TRUE : FALSE;
}

// Each DLL must call HXEnableLogging() at least once
//
void HXEnableLogging(IUnknown* pContext)
{
    if (pContext && !g_pLogSystem)
    {
#if defined(HELIX_FEATURE_CORE_LOG)
        HX_RESULT res = HXR_FAIL;
        res = pContext->QueryInterface(IID_IHXTLogSystem, (void**)&g_pLogSystem);
        if (SUCCEEDED(res))
        {
            res = DoLogWriterInit();
        }
#else
        IHXCommonClassFactory* pCCF = NULL;
        pContext->QueryInterface(IID_IHXCommonClassFactory, (void**) &pCCF);
        if (pCCF)
        {
            pCCF->CreateInstance(CLSID_IHXDllAccess, (void**) &g_pDllAccess);
            if (g_pDllAccess)
            {
                // Create the dll name
                // MAXDLLSUFFIXLEN is defined in hxtlogutil.h
                char szDLLName[3 + HXLOG_MAXDLLSUFFIXLEN + 1];  /* Flawfinder: ignore */
                strcpy(szDLLName, "log"); /* Flawfinder: ignore */
                strcat(szDLLName, HXLOG_DLLSUFFIX); /* Flawfinder: ignore */

                // Attempt to get the RMAGetLogSystemInterface entry point
                g_pDllAccess->Open(szDLLName, HXDLLTYPE_PLUGIN);
                if(g_pDllAccess->IsOpen())
                {
                    FPCREATELOGSYSTEMINTERFACE fpCreateLogSystem =
                        (FPCREATELOGSYSTEMINTERFACE)(g_pDllAccess->GetSymbol("RMACreateLogSystem"));
                    DoLogSystemInterfaceInit(fpCreateLogSystem, pContext);
                }
            }
            HX_RELEASE(pCCF);
        }
#endif
    } 
}

// This should be called once by the application to shutdown
// logging for all DLLs.
//
void HXDisableLogging(HXBOOL bShutdown)
{
    // clean up globals
    HX_RELEASE(g_pLogWriter);
    HX_RELEASE(g_pLogSystemContext);
    if (g_pLogSystem)
    { 
        if(bShutdown)
        {
            g_pLogSystem->Shutdown();
        }
        HX_RELEASE(g_pLogSystem);
    }
#if !defined(HELIX_FEATURE_CORE_LOG)
    HX_RELEASE(g_pDllAccess);
#endif
    
}

void HXLog1(EHXTLogFuncArea nFuncArea, const char* szMsg, ...)
{
    if(g_pLogWriter && g_pLogWriter->IsEnabled())
    {
        va_list VariableArguments;
        va_start(VariableArguments, szMsg);
        g_pLogWriter->LogMessage("RealNetworks", LC_CLIENT_LEVEL1, nFuncArea, MAX_UINT32, szMsg, VariableArguments);
        va_end(VariableArguments);
    }
}

void HXLog2(EHXTLogFuncArea nFuncArea, const char* szMsg, ...)
{
    if(g_pLogWriter && g_pLogWriter->IsEnabled())
    {
        va_list VariableArguments;
        va_start(VariableArguments, szMsg);
        g_pLogWriter->LogMessage("RealNetworks", LC_CLIENT_LEVEL2, nFuncArea, MAX_UINT32, szMsg, VariableArguments);
        va_end(VariableArguments);
    }
}

void HXLog3(EHXTLogFuncArea nFuncArea, const char* szMsg, ...)
{
    if(g_pLogWriter && g_pLogWriter->IsEnabled())
    {
        va_list VariableArguments;
        va_start(VariableArguments, szMsg);
        g_pLogWriter->LogMessage("RealNetworks", LC_CLIENT_LEVEL3, nFuncArea, MAX_UINT32, szMsg, VariableArguments);
        va_end(VariableArguments);
    }
}

void HXLog4(EHXTLogFuncArea nFuncArea, const char* szMsg, ...)
{
    if(g_pLogWriter && g_pLogWriter->IsEnabled())
    {
        va_list VariableArguments;
        va_start(VariableArguments, szMsg);
        g_pLogWriter->LogMessage("RealNetworks", LC_CLIENT_LEVEL4, nFuncArea, MAX_UINT32, szMsg, VariableArguments);
        va_end(VariableArguments);
    }
}

void HXLog1(const char* szMsg, ...)
{
    if(g_pLogWriter && g_pLogWriter->IsEnabled())
    {
        va_list VariableArguments;
        va_start(VariableArguments, szMsg);
        g_pLogWriter->LogMessage("RealNetworks", LC_CLIENT_LEVEL1, NONE, MAX_UINT32, szMsg, VariableArguments);
        va_end(VariableArguments);
    }
}

void HXLog2(const char* szMsg, ...)
{
    if(g_pLogWriter && g_pLogWriter->IsEnabled())
    {
        va_list VariableArguments;
        va_start(VariableArguments, szMsg);
        g_pLogWriter->LogMessage("RealNetworks", LC_CLIENT_LEVEL2, NONE, MAX_UINT32, szMsg, VariableArguments);
        va_end(VariableArguments);
    }
}

void HXLog3(const char* szMsg, ...)
{
    if(g_pLogWriter && g_pLogWriter->IsEnabled())
    {
        va_list VariableArguments;
        va_start(VariableArguments, szMsg);
        g_pLogWriter->LogMessage("RealNetworks", LC_CLIENT_LEVEL3, NONE, MAX_UINT32, szMsg, VariableArguments);
        va_end(VariableArguments);
    }
}

void HXLog4(const char* szMsg, ...)
{
    if(g_pLogWriter && g_pLogWriter->IsEnabled())
    {
        va_list VariableArguments;
        va_start(VariableArguments, szMsg);
        g_pLogWriter->LogMessage("RealNetworks", LC_CLIENT_LEVEL4, NONE, MAX_UINT32, szMsg, VariableArguments);
        va_end(VariableArguments);
    }
}

void HXLog_Push_Context(const char* szContext)
{
    if(g_pLogSystemContext)
    {
        g_pLogSystemContext->PushContext(szContext);
    }
}

void HXLog_Pop_Context()
{
    if(g_pLogSystemContext)
    {
        g_pLogSystemContext->PopContext();
    }
}

void HXLog_Set_FileAndLine(const char *szFilename, int nLineNum)
{
    if(g_pLogSystemContext)
    {
        g_pLogSystemContext->SetThreadFileAndLine(szFilename, nLineNum);
    }
}

#endif /* #if defined(HELIX_FEATURE_LOGGING_USE_FPRINTF) */
