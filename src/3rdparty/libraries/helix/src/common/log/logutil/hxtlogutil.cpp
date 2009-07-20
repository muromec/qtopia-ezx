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

#if defined(_SYMBIAN)
#include "symbian_gm_inst.h"
#endif


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

#include "hlxclib/string.h"

// This array maps the Producer SDK functional areas
// (which start at 0 (NONE) and go up to 23 (PUB)) to
// client 4cc codes
static const EHXTLogFuncArea g_eProducerSDKTo4ccMap[PUB + 1] = 
{
    HXLOG_ENON, HXLOG_EACT, HXLOG_EAUC, HXLOG_EAUP,
    HXLOG_EBCA, HXLOG_ECAP, HXLOG_ECMD, HXLOG_EFLO,
    HXLOG_EFLR, HXLOG_EGUI, HXLOG_EJOB, HXLOG_ELIC,
    HXLOG_EPOS, HXLOG_EREM, HXLOG_ECON, HXLOG_EENC,
    HXLOG_ECOR, HXLOG_EFLT, HXLOG_ESTA, HXLOG_EVIC,
    HXLOG_EVIP, HXLOG_EVIR, HXLOG_EMED, HXLOG_EPUB
};
const EHXTLogFuncArea kDefaultFuncArea = HXLOG_ENON;

#if !defined(HELIX_CONFIG_NOSTATICS)

extern IHXTInternalLogWriter*   g_pLogWriter = NULL;
static IHXTLogSystemContext*    g_pLogSystemContext = NULL;
static IHXTLogSystem*           g_pLogSystem = NULL;

#else /* #if !defined(HELIX_CONFIG_NOSTATICS) */

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

#endif /* #if !defined(HELIX_CONFIG_NOSTATICS) #else */

HX_RESULT DoLogWriterInit()
{
    HX_RESULT retVal = HXR_FAIL;

    if(g_pLogSystem != NULL)
    {
        retVal = g_pLogSystem->QueryInterface(IID_IHXTLogSystemContext, (void**) &g_pLogSystemContext);
        if (SUCCEEDED(retVal))
        {       
            IHXTLogWriter* pLogWriter = NULL;
            retVal = g_pLogSystem->GetWriterInterface(&pLogWriter);
            if (SUCCEEDED(retVal))
            {
                retVal = pLogWriter->QueryInterface(IID_IHXTInternalLogWriter, (void**) &g_pLogWriter);
            }
            HX_RELEASE(pLogWriter);
        }
    } // End of if(g_pLogSystem != NULL)

    return retVal;
}

EHXTLogFuncArea MapFunctionalArea(EHXTLogFuncArea eFuncArea)
{
    EHXTLogFuncArea eRet = eFuncArea;
    if (eFuncArea <= PUB)
    {
        eRet = g_eProducerSDKTo4ccMap[eFuncArea];
    }
    return eRet;
}

void RSLog(UINT32 nMsg, const char* szMsg, ...)
{
    if (g_pLogWriter)
    {
        va_list VariableArguments;
        va_start(VariableArguments, szMsg);
        g_pLogWriter->LogMessage("RealNetworks", LC_DEV_DIAG, kDefaultFuncArea, nMsg, szMsg, VariableArguments);
        va_end(VariableArguments);
    }
}

void RSLog(EHXTLogCode nLogCode, UINT32 nMsg, const char* szMsg, ...)
{
    if( g_pLogWriter )
    {
        va_list VariableArguments;
        va_start(VariableArguments, szMsg);
        g_pLogWriter->LogMessage("RealNetworks", nLogCode, kDefaultFuncArea, nMsg, szMsg, VariableArguments);
        va_end(VariableArguments);
    }
}

void RSLog(EHXTLogCode nLogCode, EHXTLogFuncArea nFuncArea, UINT32 nMsg, const char* szMsg, ...)
{
    if( g_pLogWriter )
    {
//        nFuncArea = MapFunctionalArea(nFuncArea);
        va_list VariableArguments;
        va_start(VariableArguments, szMsg);
        g_pLogWriter->LogMessage("RealNetworks", nLogCode, nFuncArea, nMsg, szMsg, VariableArguments);
        va_end(VariableArguments);
    }
}

void RSLog(const char* szMsg, ...) 
{	
    if( g_pLogWriter )
    {
        va_list VariableArguments;
        va_start(VariableArguments, szMsg);
        g_pLogWriter->LogMessage("RealNetworks", LC_DEV_DIAG, kDefaultFuncArea, MAX_UINT32, szMsg, VariableArguments);
        va_end(VariableArguments);
    }
}

void RSLog(EHXTLogCode nLogCode, const char* szMsg, ...) 
{
    if( g_pLogWriter )
    {
        va_list VariableArguments;
        va_start(VariableArguments, szMsg);
        g_pLogWriter->LogMessage("RealNetworks", nLogCode, kDefaultFuncArea, MAX_UINT32, szMsg, VariableArguments);
        va_end(VariableArguments);
    }
}

void RSLog(EHXTLogCode nLogCode, EHXTLogFuncArea nFuncArea, const char* szMsg, ...) 
{
    if( g_pLogWriter )
    {
//        nFuncArea = MapFunctionalArea(nFuncArea);
        va_list VariableArguments;
        va_start(VariableArguments, szMsg);
        g_pLogWriter->LogMessage("RealNetworks", nLogCode, nFuncArea, MAX_UINT32, szMsg, VariableArguments);
        va_end(VariableArguments);
    }
}

HXBOOL 
HXLoggingEnabled() 
{ 
    return (g_pLogWriter && g_pLogWriter->IsEnabled()) ? TRUE : FALSE;
}

//
// Function invoked to get core log system 
// reference from global manager. Functionality enabled
// only for symbian builds.
//
void HXEnableCoreLogging()
{
#if defined(HELIX_FEATURE_CORE_LOG) && defined (_SYMBIAN)

    if(g_pLogSystem == NULL)
    {
        
        HXGlobalManager* pGM = HXGlobalManager::Instance();
        IHXTLogSystem** pInstance = NULL;
        IUnknown* pContext = NULL;
        HX_RESULT res = HXR_FAIL;
        
        if(pGM != NULL)
        {
            pInstance = reinterpret_cast<IHXTLogSystem**>(pGM->Get((const *)SYMBIAN_GLOBAL_LOGSYSTEM_ID));
            if(pInstance != NULL)
            {
                pContext = *pInstance;
            }
            
            if(pContext != NULL)
            {
                res = pContext->QueryInterface(IID_IHXTLogSystem, (void**)&g_pLogSystem);
                if (SUCCEEDED(res))
                {
                    res = DoLogWriterInit();
                }
            }
        } // End of     if(pGM != NULL)
    } // End of if(g_pLogSystem != NULL)
#endif // End of #if defined(HELIX_FEATURE_CORE_LOG) && defined (_SYMBIAN)
}

// Each DLL must call HXEnableLogging() at least once
//
void HXEnableLogging(IUnknown* pContext)
{
    if (pContext && !g_pLogSystem)
    {
        IHXLogSystemManager* pLogSystemManager = NULL;
        HX_RESULT rv = pContext->QueryInterface(IID_IHXLogSystemManager, (void**) &pLogSystemManager);
        if (SUCCEEDED(rv))
        {
            // Get the log system from the log system manager
            rv = pLogSystemManager->GetLogSystem(g_pLogSystem);
            if (SUCCEEDED(rv))
            {
                rv = DoLogWriterInit();
            }
        }
        HX_RELEASE(pLogSystemManager);
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
        g_pLogWriter->LogMessage("RealNetworks", LC_CLIENT_LEVEL1, kDefaultFuncArea, MAX_UINT32, szMsg, VariableArguments);
        va_end(VariableArguments);
    }
}

void HXLog2(const char* szMsg, ...)
{
    if(g_pLogWriter && g_pLogWriter->IsEnabled())
    {
        va_list VariableArguments;
        va_start(VariableArguments, szMsg);
        g_pLogWriter->LogMessage("RealNetworks", LC_CLIENT_LEVEL2, kDefaultFuncArea, MAX_UINT32, szMsg, VariableArguments);
        va_end(VariableArguments);
    }
}

void HXLog3(const char* szMsg, ...)
{
    if(g_pLogWriter && g_pLogWriter->IsEnabled())
    {
        va_list VariableArguments;
        va_start(VariableArguments, szMsg);
        g_pLogWriter->LogMessage("RealNetworks", LC_CLIENT_LEVEL3, kDefaultFuncArea, MAX_UINT32, szMsg, VariableArguments);
        va_end(VariableArguments);
    }
}

void HXLog4(const char* szMsg, ...)
{
    if(g_pLogWriter && g_pLogWriter->IsEnabled())
    {
        va_list VariableArguments;
        va_start(VariableArguments, szMsg);
        g_pLogWriter->LogMessage("RealNetworks", LC_CLIENT_LEVEL4, kDefaultFuncArea, MAX_UINT32, szMsg, VariableArguments);
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

void HXLog_SetThreadJobName(const char* szJobName, UINT32 nThreadId)
{
    if(g_pLogSystemContext)
    {
        g_pLogSystemContext->SetThreadJobName(szJobName, nThreadId);
    }
}

void HXLog_CreateChildThread(UINT32 nParentThreadId, UINT32 nChildThreadId)
{
    if(g_pLogSystemContext)
    {
        g_pLogSystemContext->SetParentChildRelationship(nParentThreadId, nChildThreadId);
    }
}

void HXLog_EndThread(UINT32 nThreadId)
{
    if(g_pLogSystemContext)
    {
        g_pLogSystemContext->EndThread(nThreadId);
    }
}

#endif /* #if defined(HELIX_FEATURE_LOGGING_USE_FPRINTF) */
