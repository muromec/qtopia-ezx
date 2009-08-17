/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: reglog.h,v 1.2 2003/08/21 18:06:22 bgoldfarb Exp $ 
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

#ifndef REGLOG_H
#define REGLOG_H

// forward declarations
struct IHXBuffer;
struct IHXCommonClassFactory;
struct IHXRegistry;
class CPtrQueue;

/*
 *  Configuartion Registry strings and defaults
 */

//default for DeleteDelay - how to long to wait before deleting
//a log entry from the registry, i.e. how long should the tlogger be 
//given to send the entry to its Output destinations
#define REGLOG_CLEANDELAY       5000 //millisecs
#define REGLOG_CLEANDELAY_MIN   2000 //millisecs
  
/*

CRegLogger A class which uses the registry as a transitory log storage area and then depends on
and exploits the templatized logging system to output log entries using a session template.

Log Entries are stored in a configurable location in the registry using this format:

<LOG_REGROOT>.<LOGID>.Entry

<LOG_REGROOT> is configured by the CRegLogger client. LOGID is initially a random integer and is then
incremented with every new entry.

Multiple CRegLogger clients can use the same <LOG_REGROOT> without fear of collisions.

If CleanEntries is set to TRUE, each entry written to the log will be deleted within no fewer then 
nCleanDelay milliseconds. Theis delay should be long enough to give the templatized logging system
time to respond to the watch notification and complete its output assignments.



usage:

    #include "reglog.h"
    
    CRegLogger* pLogger = new CRegLogger(m_pContext, "Server.MyLogSpace");
    if (pLogger)
    {
        pLogger->AddRef();
        pLogger->LogThis(pBufLogEntry);
    }    
    HX_RELEASE(pLogger);

*/


class CRegLogger : public IHXCallback
{
private:
    INT32		            m_lRefCount;
    CallbackHandle          m_hCallback;
    CPtrQueue*              m_pIdQ;
    UINT32                  m_ulCounter;

    STDMETHOD(SetTimer)(THIS);

protected:
    IUnknown*               m_pContext;
    IHXCommonClassFactory*  m_pCommonClassFactory;
    IHXRegistry*            m_pRegistry;
    IHXBuffer*              m_pLogRootRegKey;
    BOOL                    m_bCleanEntries;
    UINT32                  m_nCleanDelay;

    STDMETHOD_(IHXBuffer*, MakeBuffer)(const char* str);
    STDMETHOD_(IHXBuffer*, MakeBuffer)(UINT32 nSize);

public:
    CRegLogger(IUnknown* pContext, 
        const char* pszLogRootRegKey, 
        BOOL bCleanEntries = TRUE,
        UINT32 nCleanDelay = REGLOG_CLEANDELAY /*ms.*/ );

    ~CRegLogger();

    /*
     *	IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
    	    	    	    	REFIID riid,
    	    	    	    	void** ppvObj);

    STDMETHOD_(UINT32,AddRef)	(THIS);

    STDMETHOD_(UINT32,Release)	(THIS);
    
    /*
     *	Callback methods
     */
    STDMETHOD(Func)	    (THIS);
    
    /*
     * Public Methods
     */
     
    STDMETHOD(LogThis)(THIS_ IHXBuffer* pLogMsg);

    STDMETHOD_(BOOL,GetIsCleanEntriesEnabled)(THIS);
    STDMETHOD_(UINT32,GetCleanDelay)(THIS);
};



/*

CConfigLogger is a subclass of CRegLogger that hardwires the LOG_REGROOT location to
Server.ConfigLog otherwise looks to config.ConfigLog for the "Enabled", "CleanEntries"
and "CleanDelay" settings. It also provides some helper functions for creating log 
entries in the fashion of the Common Access Log format.

usage:

    #include "reglog.h"
    
    CConfigLogger* pLogger = new CConfigLogger(m_pContext);
    if (pLogger)
    {
        pLogger->AddRef();
        pLogger->LogConfigClient(m_pRequest, pszUserId, pszConfigChangeSummary);
    }    
    HX_RELEASE(pLogger);

*/


/*
 *  Configuartion Registry strings and defaults
 */

//path to registry list where our configuration settings live
#define CONFIGLOG_CONFIG_ROOT      "config.ConfigLog."

//path to "Enabled" registry var
#define CONFIGLOG_CONFIG_ENABLED_REGKEY	CONFIGLOG_CONFIG_ROOT "Enabled"
//default is disabled

//path to "Enabled" registry var
#define CONFIGLOG_CONFIG_CLEAN_REGKEY	    CONFIGLOG_CONFIG_ROOT "CleanEntries"
//default is to clean up the entries after the CLEANDELAY has elapsed

//path to "DeleteDelay" registry var
#define CONFIGLOG_CONFIG_CLEANDELAY_REGKEY CONFIGLOG_CONFIG_ROOT "CleanDelay"

//default for RootRegKey - where in the registry do we strore log entries
//this can also be set via SetLogRootRegKey()
#define CONFIGLOG_LOG_ROOTREGKEY    "ConfigLog"

// forward declarations
struct IHXRequest;

class CConfigLogger : public CRegLogger
{
private:
    STDMETHOD(MakeLogEntry)(THIS_ IHXRequest* pRequest, 
        const char* pszUserId, const char* pszLogMsg, 
        REF(IHXBuffer*) pBufEntry);

public:
    CConfigLogger(IUnknown* pContext);
    ~CConfigLogger();

    /*
     * Public Methods
     */

    STDMETHOD_(BOOL,GetIsCleanEntriesEnabled)(THIS);
    STDMETHOD_(UINT32,GetCleanDelay)(THIS);
     
    STDMETHOD(LogConfigClient)(THIS_ IHXRequest* pRequest, const char* pszUserId, 
                          const char* pszLogMsg);
   
    static BOOL IsLoggingEnabled(IHXRegistry* pRegistry);
};


#endif // REGLOG_H
