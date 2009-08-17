/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: winsig.cpp,v 1.3 2004/05/13 18:57:48 tmarshall Exp $ 
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
#include "hxtypes.h"
#include "hlxclib/windows.h"
#include "hxresult.h"
#include "hxslist.h"
#include "winsig.h"

HANDLE WinSigHandler::c_hThread = 0;
HANDLE WinSigHandler::c_hPipe = 0;
CRITICAL_SECTION* WinSigHandler::cs = 0;
CHXSimpleList* WinSigHandler::c_Handlers = 0;
 
DWORD
WinSigHandler::WinSigHandlerThread(DWORD*)
{
    /*
     * Get Pid.
     */
    DWORD Pid = GetCurrentProcessId();

    /* 
     * Create Pipe.
     */
    char PipeName[256];
    sprintf(PipeName, "\\\\.\\pipe\\HX_Signal_Pipe%lu", Pid);

    SECURITY_DESCRIPTOR *pSD;
    SECURITY_ATTRIBUTES sa;

    pSD = (SECURITY_DESCRIPTOR *)new unsigned char[SECURITY_DESCRIPTOR_MIN_LENGTH];
    InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION);
    /*
     *  Setting dacl to NULL to allow all access from all users.
     */
    SetSecurityDescriptorDacl(pSD, TRUE, (PACL)NULL, FALSE);
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = TRUE;

    c_hPipe = CreateNamedPipe(
	PipeName,
	PIPE_ACCESS_INBOUND,
	PIPE_READMODE_BYTE | PIPE_TYPE_BYTE | PIPE_WAIT,
	1, //instances
	20, //out buffer size
	20, //in buffer size
	0,  //default wait timeout.
	&sa);

    delete pSD;

    HX_ASSERT(c_hPipe != INVALID_HANDLE_VALUE);
	    
    UINT32 ulSigNum;
    UINT32 ulRead;
    while(1)
    {
	/*
	 * Loop on pipe.
	 */
	ConnectNamedPipe(c_hPipe, 0);
	ReadFile(c_hPipe,
	    &ulSigNum,
	    4,
	    &ulRead,
	    0);
	DisconnectNamedPipe(c_hPipe);

	/*
	 * Dispatch Signal.
	 */
	EnterCriticalSection(cs);
	CHXSimpleList::Iterator i;
	for(i = c_Handlers->Begin(); i != c_Handlers->End(); ++i)
	{
	    WIN_SIG_HANDLER_PROC p = (WIN_SIG_HANDLER_PROC)*i;
	    p(ulSigNum);
	}
	LeaveCriticalSection(cs);
    }
    
    CloseHandle(c_hPipe);
    c_hPipe = 0;
    return 0;
}

WinSigHandler::WinSigHandler()
{
}

WinSigHandler::~WinSigHandler()
{
}

HX_RESULT
WinSigHandler::Init()
{
    /*
     * Create the sighandle thread.
     */
    if(c_hThread || c_Handlers || cs)
    {
	return HXR_FAIL;
    }
    cs = new CRITICAL_SECTION;
    InitializeCriticalSection(cs);
    DWORD dwId;
    c_Handlers = new CHXSimpleList;
    c_hThread = CreateThread(0, 0,
	(LPTHREAD_START_ROUTINE)WinSigHandlerThread, 0, 0, &dwId);
    return HXR_OK;
}

HX_RESULT
WinSigHandler::Done()
{
    delete cs;
    cs = 0;
    delete c_Handlers;
    c_Handlers = 0;
    CloseHandle(c_hThread);
    c_hThread = 0;
    /*
     * Some day I will care about shutting down.
     */
    return HXR_NOTIMPL;
}

HX_RESULT
WinSigHandler::AddSigHandler(WIN_SIG_HANDLER_PROC p)
{
    if(!p)
    {
	return HXR_FAIL;
    }
    EnterCriticalSection(cs);
    c_Handlers->AddTail(p);
    LeaveCriticalSection(cs);
    return HXR_OK;
}

HX_RESULT
WinSigHandler::RemoveSigHandler(WIN_SIG_HANDLER_PROC p)
{
    LISTPOSITION pos;
    pos = c_Handlers->Find(p);
    if(!pos)
    {
	return HXR_FAIL;
    }
    EnterCriticalSection(cs);
    c_Handlers->RemoveAt(pos);
    LeaveCriticalSection(cs);
    return HXR_OK;
}
