/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: main.cpp,v 1.11 2006/04/03 18:40:54 seansmith Exp $ 
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

#include <stdio.h>
#include <process.h>

#include "hxtypes.h"
#include "debug.h"
#include "hxproc.h"
#include "proc.h"
#include "shmem.h"
#include "acceptor.h"
#include "config.h"
#include "server_engine.h"
//#include "main.h"
#include "core_proc.h"
#include "dispatchq.h"
#include "mutex.h"
#include "service.h"
#include "plgnhand.h"
#include "server_version.h"
#include "mem_cache.h"

#include "shutdown.h"
#include "streamer_container.h"
#include "globals.h"
#include "_main.h"

#ifdef _DEBUG
#include "winsig.h"
#endif

extern int shared_ready;
extern BOOL g_bFastMallocAll;
extern BOOL*   g_bEnableClientConnCleanup;

void terminate(int code);

#ifdef _PAULM_SERVICE_DEBUG
HANDLE hOut = NULL;

void
pprintf(const char* pout ...)
{
    if (!hOut)
    {
	return;
    }
    char temp[1024] = "";
    va_list ap;
    va_start(ap, pout);

    vsprintf(temp, pout, ap);

    const char* pc;
    const char* start = pout;
    DWORD dw;
    //WriteFile(hOut, temp, strlen(temp), &dw, NULL);
    WriteConsole(hOut, (const void*)temp,
	strlen(temp), &dw, 0);
}

void
LaunchConsole()
{
    AllocConsole();
    
    HANDLE hScreenBuffer = CreateConsoleScreenBuffer(
	GENERIC_READ | GENERIC_WRITE, 
	FILE_SHARE_READ | FILE_SHARE_WRITE,
	NULL,
	CONSOLE_TEXTMODE_BUFFER,
	NULL);

    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(
	hScreenBuffer,
	&csbi);
    COORD microsoft_stuff;
    microsoft_stuff = csbi.dwSize;
    microsoft_stuff.Y = 4096;
    SetConsoleScreenBufferSize(hScreenBuffer, microsoft_stuff);
    SetConsoleActiveScreenBuffer(hScreenBuffer);
    hOut = hScreenBuffer;
}

#endif /* _PAULM_SERVICE_DEBUG */


typedef struct _parameter
{
    int	    argc;
    char**  argv;
} PARAMETER, * LPPARAMETER;

#if defined(_WIN32) && !defined(USE_WINSOCK1)
WORD VersionRequested = 0x0202;
#else
WORD VersionRequested = 0x0101;
#endif

DWORD ErrorCode = 0;
BOOL bIsService = FALSE;
SERVICE_STATUS_HANDLE ServiceStatusHandle;
SERVICE_STATUS ServiceStatus;
CHAR ConfigBuffer[256];
DWORD BufferSize = sizeof(ConfigBuffer);
HANDLE ServerDoneEvent = NULL;
int ok_to_terminate = 0;
BOOL	terminated = FALSE;

int _main(int argc, char** argv);

extern DispatchQueue*	g_dq;

int		g_nNewArgc = 0;
char**		g_pNewArgv = NULL;

const char* g_szServiceName = "";

extern PluginHandler* 	g_plugin_handler;

VOID
SocketCleanup()
{
    if (WSACleanup() == SOCKET_ERROR && GetLastError() != WSANOTINITIALISED)
    {
	DPRINTF(D_INFO, ("WSACleanup failed %d", GetLastError()));
    }
}

void killme(int code)
{
    if (bIsService)
    {
        SetEvent(ServerDoneEvent);
    }
    else
    {
        SocketCleanup();
        exit(0);
    }
}

/*
 * niam
 * 
 * Reverse of main.  Shuts everything down.
 */
void
niam(BOOL bRestart = FALSE)
{
	if(*g_bEnableClientConnCleanup == FALSE && bRestart == TRUE)
	{
		RestartServer();
		return;
	}
	else if(*g_bEnableClientConnCleanup == FALSE)
	{
		killme(0);
	}

    Process* proc = NULL;
    int processnum = 0;
    int processtokill = 0;
  
    processtokill = Process::numprocs();

    processnum = Process::GetNewProcessNumber();
	
    proc = new Process;
    proc->AssignProcessNumber(processnum);

    StopAliveChecker();
	for (int i = 0; i < processtokill; i++)
	{
		if(i!= 1 && i != PROC_RM_CONTROLLER)
		{
			ShutdownCallback* cb = new ShutdownCallback;  
			cb->m_bRestart = bRestart;
			g_dq->send(proc, cb, i);
		}
	}
}

void
ServiceStart(LPPARAMETER pParameter)
{
    _main(pParameter->argc, pParameter->argv);
}

void
ServiceEnd()
{
    //shut everything down.
    niam();
}

VOID
ServiceControl(DWORD ControlCode)
{
    switch(ControlCode)
    {
    case SERVICE_CONTROL_STOP:
	if (ServiceStatus.dwCurrentState != SERVICE_STOP_PENDING)
	{
	    ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
	    ServiceStatus.dwWaitHint = 3000;

	    if (!SetServiceStatus(ServiceStatusHandle, &ServiceStatus))
	    {
		DPRINTF(D_INFO, ("Unable to set STOP PENDING Service Status %d",
				GetLastError()));
		exit(1);
	    }
	}

	ServiceEnd();
	return;

    default:
	break;
    }
}

void
terminate(int code)
{

    //if we are Win95, bIsService will be FALSE
    if (bIsService && ok_to_terminate)
    {
	DPRINTF(D_INFO, ("Terminating with exit code %d", code));
	ServiceControl(SERVICE_CONTROL_STOP);
	return;
    }


    SocketCleanup();
    exit(code);
}

void
os_init()
{
}

void
StartWinsock()
{
    WSADATA WSAData;

    ErrorCode = WSAStartup(VersionRequested, &WSAData);

    if (ErrorCode != 0)
    {
	DPRINTF(D_INFO, ("Socket initialization failed with error 0x%x\n", ErrorCode));
	terminate(1);
    }
}

void
os_start()
{
}

//
// Function: ParseCommandLine
// 
// Description: Takes a single character string and parses it into distinct
//	arguments which are placed in the pArgv array. Note: the pArgv[0]
//	position is reserved for the executable name, so this function sets
//	it to NULL.
//
// Return: Total number of arguments in pArgv, including the NULL [0] value
//
int
ParseCommandLine(char* pCommandLine, char** pOldArgv, char**& pArgv)
{
    UINT32 ulCurrentArg	  = 1;     // Next available index in pArgv
    BOOL   bInQuotes	  = FALSE; // Are we inside double quotes?
    char   pTempArg[512];	   // Accumulator for next argument
    UINT32 ulArgPos	  = 0;     // Next position to be filled in pTempArg
    char*  pCurrPos	  = NULL;  // Iterator

    // First pass to find out how many arguments there are
    for (pCurrPos = pCommandLine; *pCurrPos; pCurrPos++)
    {
	if (*pCurrPos == '\"')
	{
	    bInQuotes = !bInQuotes;
	}
	else if (*pCurrPos == ' ')
	{
	    if (bInQuotes)
	    {
		ulArgPos++;
	    }
	    else
	    {
		if (ulArgPos > 0)
		{
		    ulCurrentArg++;
		    ulArgPos = 0;
		}
	    }
	}
	else
	{
	    ulArgPos++;
	}
    }
    if (ulArgPos > 0)
	ulCurrentArg++;

    // Allocate the pointer array
    pArgv = new char*[ulCurrentArg];

    // Fill in the first argument with the executable name
    if (pOldArgv[0])
    {
	pArgv[0] = new char[strlen(pOldArgv[0]) + 1];
	strcpy(pArgv[0], pOldArgv[0]);
    }

    // Reinitialize state
    ulCurrentArg = 1;
    bInQuotes = FALSE;
    ulArgPos = 0;

    // Second pass to actually allocate and strcpy each argument
    for (pCurrPos = pCommandLine; *pCurrPos; pCurrPos++)
    {
	if (*pCurrPos == '\"')
	{
	    bInQuotes = !bInQuotes;
	}
	else if (*pCurrPos == ' ')
	{
	    if (bInQuotes)
	    {
		pTempArg[ulArgPos++] = *pCurrPos;
	    }
	    else
	    {
		if (ulArgPos > 0)
		{
		    pTempArg[ulArgPos] = '\0';
		    pArgv[ulCurrentArg] = new char[strlen(pTempArg) + 1];
		    strcpy(pArgv[ulCurrentArg], pTempArg);

		    ulCurrentArg++;
		    ulArgPos = 0;
		}
	    }
	}
	else
	{
	    pTempArg[ulArgPos++] = *pCurrPos;
	}
    }

    if (ulArgPos > 0)
    {
	pTempArg[ulArgPos++] = '\0';
	pArgv[ulCurrentArg] = new char[strlen(pTempArg) + 1];
	strcpy(pArgv[ulCurrentArg], pTempArg);
	ulCurrentArg++;
    }

    return ulCurrentArg;
}

int
servicemain(int argc, char** argv)
{
    HANDLE DoneEvent = NULL;

    if(argc > 0)
    {
        g_szServiceName = argv[0];
    }

    if ((DoneEvent = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL)
    {
	DPRINTF(D_INFO, ("Unable to create DoneEvent 1 %d", GetLastError()));
	terminate(1);
    }

    ServerDoneEvent = DoneEvent;

    ServiceStatusHandle = RegisterServiceCtrlHandler(TEXT(progname), (LPHANDLER_FUNCTION)ServiceControl);

    if (!ServiceStatusHandle)
    {
	DPRINTF(D_INFO, ("Unable to register Service Control Handler %d",
		GetLastError()));
	terminate(1);
    }

    ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    ServiceStatus.dwCurrentState = SERVICE_START_PENDING;
    ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    ServiceStatus.dwWin32ExitCode = NO_ERROR;
    ServiceStatus.dwServiceSpecificExitCode = 0;
    ServiceStatus.dwCheckPoint = 0;
    ServiceStatus.dwWaitHint = 3000;

    if (!SetServiceStatus(ServiceStatusHandle, &ServiceStatus))
    {
	DPRINTF(D_INFO, ("Unable to set START PENDING Service Status %d",
		GetLastError()));
	terminate(1);
    }

    /*
     * NOTE: The bIsService branch of this routine never returns...at
     *	     completion, the server just exits
     */
    HANDLE	ThreadHandle = NULL;
    PARAMETER	Arg;
    DWORD	dwError	     = 0;
    DWORD	dwSize	     = _MAX_PATH;
    HKEY	hkParameters = NULL;
    char**	newArgv	     = NULL;
    int		newArgc	     = 0;
    BOOL	bFoundParams = FALSE;
    char	pServiceKey[_MAX_PATH + 1];
    char	pStartupParams[_MAX_PATH + 1];

    //
    // Obtain the command line parameters for the service from the
    // NT Registry Key "StartupParams" in our service section
    //
    sprintf(pServiceKey, "%s\\%s", REGISTRY_KEY_SERVICE, argv[0]);
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
		     pServiceKey, 
		     0, 
		     KEY_ALL_ACCESS, 
		     &hkParameters) == ERROR_SUCCESS)
    {
	if (GETKEYVALUE(hkParameters, 
			(LPTSTR)"StartupParams", 
			&pStartupParams, 
			&dwSize) == ERROR_SUCCESS)
	{
	    // Parse the command line parameters into standard argv format
	    newArgc = ParseCommandLine(pStartupParams, argv, newArgv);

	    // Save a pointer to the NewArgv so we can clean it up later
	    g_nNewArgc = newArgc;
	    g_pNewArgv = newArgv;

	    Arg.argc = newArgc;
	    Arg.argv = newArgv;

	    bFoundParams = TRUE;
	}

	FREEHKEY(hkParameters);
    }

    if (!bFoundParams)
    {
	Arg.argc = argc;
	Arg.argv = argv;
    }

    ThreadHandle = (HANDLE)_beginthread((VOID(*)(PVOID))ServiceStart,
					0,
					(PVOID)&Arg);
    if ((u_long32)ThreadHandle < 0)
    {
					 
	DPRINTF(D_INFO, ("Unable to start service %d", GetLastError()));
	terminate(1);
    }

    if (WaitForSingleObject(ServerDoneEvent, INFINITE) == WAIT_FAILED)
    {
	DPRINTF(D_INFO, ("Wait for ServerDoneEvent failed %d", GetLastError()));
    }

    DPRINTF(D_INFO, ("%s terminated", ServerVersion::ProductName()));

    SocketCleanup();

    // wait for all the threads are terminated
    int i = 0;
    while (*g_pStreamerCount && i < 5)
    {
	Sleep(1);
	i++;
    }
    // cleanup handles
    CloseHandle(ServerDoneEvent);

    if (g_plugin_handler)
    {
	g_plugin_handler->FreeAllLibraries();
    }

    // don't tell stupid ms service control manager that we're stopped 
    // till we've freed all the libraries, else it gets all freaked
    // out about perfplin
    ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    ServiceStatus.dwWaitHint = 0;
    ++ServiceStatus.dwCheckPoint;
    SetServiceStatus(ServiceStatusHandle, &ServiceStatus);

    return 0;
}

void
MaybeNotifyServiceUp()
{
    if (bIsService)
    {
	ServiceStatus.dwCurrentState = SERVICE_RUNNING;
	ServiceStatus.dwCheckPoint = 1;

	if (!SetServiceStatus(ServiceStatusHandle, &ServiceStatus))
	{
	    DPRINTF(D_INFO, ("Unable to set RUNNING Service Status %d",
		    GetLastError()));
	    terminate(1);
	}
    }
}


/*
 * ConsoleCtrlHandler(DWORD dwCtrlType)
 *
 * Function to catch ^C.
 */
BOOL
ConsoleCtrlHandler(DWORD dwCtrlType)
{
    switch(dwCtrlType)
    {
	case CTRL_C_EVENT:
	case CTRL_CLOSE_EVENT:
	    niam();
	    
            if (WaitForSingleObject(ServerDoneEvent, INFINITE) == WAIT_FAILED)
            {
	        DPRINTF(D_INFO, ("Wait for ServerDoneEvent failed %d", GetLastError()));
            }
            Sleep(10);
	    exit(0);

	    return TRUE;
    }
    return FALSE;
}



int g_argc;
char** g_argv;

#ifdef _DEBUG
WinSigHandler g_wsh;
void
ClearAllocTrace(void)
{
}

void
InitAllocTrace(void)
{
}

void
ReportAllocTrace(FILE*)
{
}

HX_RESULT
HandleSignal(UINT32 ulSigNum)
{
    if (ulSigNum == 0)
    {
	ClearAllocTrace();
    }
    if (ulSigNum == 1)
    {
	ReportAllocTrace(stdout);
    }
    return HXR_OK;
}

#endif


void
SetupMemleakTracking()
{
#ifdef _DEBUG
    InitAllocTrace();
    g_wsh.Init();
    g_wsh.AddSigHandler(HandleSignal);
    printf("Mem leak checking enabled...\n");

    printf("Server pid: %lu\n", GetCurrentProcessId());
#endif
}


void
Flatten(int argc, char** argv, char* p)
{
    *p = 0;
    BOOL bNeedsQuote = 0;
    for (int i = 0; i < argc; i++)
    {
	/*
	 * If this params contains a space and is not quoted,
	 * then quote it.
	 */
	if (strchr(argv[i], ' ') && argv[i][0] != '\"')
	{
	    bNeedsQuote = 1;
	    strcat(p, "\"");
	}
	strcat(p, argv[i]);
	if (bNeedsQuote)
	{
	    strcat(p, "\"");
	    bNeedsQuote = 0;
	}
	strcat(p, " ");
    }
}

void perform_restart()
{
    if (!bIsService)
    {
	UINT32 ul = 0;
	int i;

	/*
	 * Get a flat command line.
	 */
	char* pCommandLine;
	for (i = 0; i < g_argc; i++)
	{
	    ul += strlen(g_argv[i]);
	}
	pCommandLine = new char[ul + 40]; //30 for wPID
	Flatten(g_argc, g_argv, pCommandLine);

	/*
	 * Append -wPID to command line.
	 */
	ul = GetCurrentProcessId();
	sprintf(&(pCommandLine[strlen(pCommandLine)]),
	    "-w%lu", ul);
    
	/*
	 * CreateProcess with the w command line.
	 */
	STARTUPINFO si;
	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	PROCESS_INFORMATION pi;

	BOOL bProcCreated = 
	CreateProcess(NULL, //don't need proc name, using command line
		      pCommandLine,
		      NULL, //processs attributes
		      NULL, //Thread attributes
		      FALSE, //Don't inherit my handles
		      0, //Creation flags
		      NULL, //Use my environment
		      NULL, //Use my current directory
		      &si,
		      &pi);

	if (bProcCreated)
	{
	    terminate(0);
	}
	return;
    }
    else
    {
	STARTUPINFO si;
	memset(&si, 0, sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	PROCESS_INFORMATION pi;

        char* cmdadd = " -reservice:";
        char* pNewCommandLine = new char[strlen(g_argv[0]) + strlen(cmdadd) 
                                            + strlen(g_szServiceName) + 3];
        sprintf(pNewCommandLine, "%s%s\"%s\"", g_argv[0], cmdadd, 
                g_szServiceName);
 
	BOOL bProcCreated = 
	CreateProcess(NULL, //don't need proc name, using command line
		      pNewCommandLine,
		      NULL, //processs attributes
		      NULL, //Thread attributes
		      TRUE, //Don't inherit my handles
		      0, //Creation flags
		      NULL, //Use my environment
		      NULL, //Use my current directory
		      &si,
		      &pi);
	delete[] pNewCommandLine;
	return;
    }
}

BOOL
ShutdownServer(BOOL bManually)
{
    if(bManually)
    {
        niam(FALSE);
    }
    else
    {
        terminate(0);
    }
    return TRUE;
}

BOOL
RestartServer(BOOL bManually)
{
    if(bManually)
    {
        niam(bManually);
    }
    else
    {
        perform_restart();
    }
    return TRUE;
}

int
WaitForOld(int argc, char** argv)
{
    UINT32 ulPid = 0;
    if (!strncmp(argv[argc-1], "-w", 2))
    {
	ulPid = atol(&(argv[argc-1][2]));
	HANDLE hProc = NULL;
	hProc = OpenProcess(SYNCHRONIZE,
			    FALSE,
			    ulPid);
	if (hProc)
	{
	    WaitForSingleObject(hProc,
		5000);
	    CloseHandle(hProc);
	}
	return 1;
    }

    if (!strncmp(argv[argc-1], "-reservice:", 11))
    {
        const char* szServiceName = argv[argc-1] + 11;
        StopService(szServiceName);
        StartService(szServiceName);
        exit(0);
    }

    return 0;
}

extern "C" int
lib_main(int argc, char **argv)
{
    g_argc = argc;
    g_argv = argv;

    int  i = 0;
    int  j = -1;
    BOOL bDashNFound = FALSE;
    int DashP = -1;
    int scount = -1;
    BOOL bInstallService = FALSE;
    BOOL bRemoveService = FALSE;
    char pServiceName[MAX_DISPLAY_NAME + 1];
    char pServiceParams[_MAX_PATH + 1];

    //make stdout/stderr line-buffered
    char* pBuf1 = new char[4096];
    char* pBuf2 = new char[4096];
    setvbuf(stdout, pBuf1, _IOLBF, 4096);
    setvbuf(stderr, pBuf2, _IOLBF, 4096);

    if (WaitForOld(argc, argv))
    {
	argc--;
	g_argc--;
    }
    strcpy(pServiceName, ServerVersion::ProductName());
    strcpy(pServiceParams, "");

    HANDLE DoneEvent;
    if ((DoneEvent = CreateEvent(NULL, TRUE, FALSE, NULL)) == NULL)
    {
	DPRINTF(D_INFO, ("Unable to create DoneEvent 1 %d", GetLastError()));
	terminate(1);
    }
    ServerDoneEvent = DoneEvent;

    /*
     * Check to see if we are even on an os that supports services.
     * (ie. NT, not 95) -paulm
     */
    BOOL bServiceCapable = TRUE; //assume so
    OSVERSIONINFO winver;
    winver.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(GetVersionEx(&winver))
    {
	if(winver.dwPlatformId == VER_PLATFORM_WIN32s ||
           winver.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
	{
	    bServiceCapable = FALSE;
	}
    }

    
    for (i = 0; i < argc; i++)
    {
	if (!strcmp(argv[i], "-scount"))
	{
	    scount = i;
	    LoadLibrary("sock_count");
	}
    }

    StartWinsock();

    if(bServiceCapable)
    {
	bIsService = TRUE;

        static char* szServiceName = 0;
        if (!szServiceName)
        {
            szServiceName = new char[256];
            sprintf(szServiceName, "%s %d.%d",
            ServerVersion::ProductName(), ServerVersion::MajorVersion(),
            ServerVersion::MinorVersion());
        }
	SERVICE_TABLE_ENTRY DispatchTable [] =
	{
	    {
                TEXT(szServiceName),
                (LPSERVICE_MAIN_FUNCTION) servicemain
	    },
	    {NULL, NULL}
	};

	//
	// Check if installing, uninstalling, or running as a service
	//
	for (i = 1; i < argc; i++)
	{
	    if ((strlen(argv[i]) > 7) &&
		(strncmp(argv[i], "-install", 8) == 0))
	    {
		bInstallService = TRUE;
		// Find out if they specified a Name
		if (strlen(argv[i]) > 9)
		{
		    strcpy(pServiceName, &argv[i][9]);
		}

		// Find out the service parameters
		if (i + 1 >= argc)
		{
		    fprintf(stderr, "Error: A quoted parameter list must "
			"be specified when installing %s as a Service.\n",
                        ServerVersion::ProductName());
		    terminate(1);
		}
		else
		{
		    strcpy(pServiceParams, argv[i + 1]);
		    i++;
		}
	    }
	    else if ((strlen(argv[i]) > 6) &&
		    (strncmp(argv[i], "-remove", 7) == 0))
	    {
		bRemoveService = TRUE;
		// Find out if they specified a Name
		if (strlen(argv[i]) > 8)
		{
		    strcpy(pServiceName, &argv[i][8]);
		}
	    }
	    else if (lstrcmp(argv[i], "-v") == 0)
	    {
		bIsService = FALSE;
	    }
	    else if (lstrcmp(argv[i], "/?") == 0)
	    {
		bIsService = FALSE;
	    }
	    else if (lstrcmp(argv[i], "-N") == 0)
	    {
		bDashNFound = TRUE;
		bIsService = FALSE;
		j = i;
	    }
	    else if (lstrcmp(argv[i], "-P") == 0)
	    {
		DashP = i;
		SetupMemleakTracking();
	    }
	}

	/*
	 * If they gave us params, it is not a service.
	 */
	if (argc > 1)
	{
	    bIsService = FALSE;
	}

	if (bInstallService)
	{
	    // Install ourselves as a service
	    if (InstallService(NULL, pServiceName, 
			       pServiceParams, NULL, NULL))
	    {
		fprintf(stderr, "Successfully installed the %s Service.\n", 
		    pServiceName);
	    }
	    else
	    {
		fprintf(stderr, "Error: Unable to install the %s Service.\n", 
		    pServiceName);
	    }
	    terminate(1);
	}

	if (bRemoveService)
	{
	    // Uninstall ourselves as a service
	    if (RemoveService(pServiceName))
	    {
		fprintf(stderr, "Successfully removed the %s Service.\n", 
		    pServiceName);
	    }
	    else
	    {
		fprintf(stderr, "Error: Unable to remove the %s Service.\n", 
		    pServiceName);
	    }
	    terminate(1);
	}

	if(bIsService == TRUE)
	{
	    //
	    // Run as a service
	    //
	    if (!StartServiceCtrlDispatcher(DispatchTable)) 
	    {
		// terminate the dispatched thread
		if (ServerDoneEvent)
		{
		    SetEvent(ServerDoneEvent);
		}
		
		ErrorCode = GetLastError();

		// 
		// If program not running as a service, then run as an application
		//
		if (ErrorCode == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT)
		{
		    bIsService = FALSE;
		    SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleCtrlHandler, TRUE);
		    _main(argc, argv);
		}

		//
		// For some reason the Dispatcher returns ERROR_INVALID_FUNCTION
		// on what appears to be normal termination
		//
		else if (ErrorCode != ERROR_INVALID_FUNCTION)
		{
		    DPRINTF(D_INFO, ("Unable to start Service Control Dispatcher %d", GetLastError()));
		}
	    }
	}
	else
	{ 
	    /*
	     * Set up the ^C handler.
	     */
	    SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleCtrlHandler, TRUE);

	    // Check if a -N parameter was passed
	    if (bDashNFound || DashP != -1 || scount != -1)
	    {
		// Exclude the -N before passing back to the common main
		char **newargv = new char* [argc - 1];
		int iTo = 0, iFrom;
		for (iFrom = 0; iFrom < argc; iFrom++)
		{    
		    if(iFrom != j && iFrom != DashP && iFrom != scount)
		    {
			newargv[iTo] = new char[strlen(argv[iFrom]) + 1];
			strcpy(newargv[iTo], argv[iFrom]);
			iTo++;
		    }
		}

		// Save a pointer to the newargv so we can clean it up later
		g_nNewArgc = iTo;
		g_pNewArgv = newargv;

		_main(iTo, newargv);
	    }
	    else
	    {
		// We had an implied -N because they passed -v or /?, but we
		// don't need to explicitly remove a -N parameter
		_main(argc, argv);
	    }
	}

    }//if(bServiceCapable)
    else
    {
	SetConsoleCtrlHandler((PHANDLER_ROUTINE)ConsoleCtrlHandler, TRUE);
	_main(argc, argv);
    }


    return 0;
}

#if defined HELIX_FEATURE_SERVER_HELIX_HEAP
void*
operator new(size_t size)
{
    if (g_pMemCacheList && g_bFastMallocAll)
    {
        // We can just use 0 since CacheNew() looks up and calls
        // the correct, thread/process-specific, MemCache object.
        MemCache* pMemCache = g_pMemCacheList[0];
        if (pMemCache)
        {
            return (void*)(pMemCache->CacheNew(size));
        }
    }

    if (shared_ready)
        return SharedMemory::malloc(size);
    else
        return (void *)malloc(size);
}

void
operator delete(void *ptr)
{
    if (ptr == NULL)
        return;

    if (g_pMemCacheList && g_bFastMallocAll)
    {
        MemCache* pMemCache = g_pMemCacheList[0];
        if (pMemCache)
        {
            pMemCache->CacheDelete((char*)ptr);
            return;
        }
    }

    if (shared_ready)
        SharedMemory::free((char*)ptr);
    else
        free(ptr);
}

void*
operator new [] (size_t size)
{
    if (g_pMemCacheList && g_bFastMallocAll)
    {
        MemCache* pMemCache = g_pMemCacheList[0];
        if (pMemCache)
            return (void*)(pMemCache->CacheNew(size));
    }

    if (shared_ready)
	return SharedMemory::malloc(size);
    else
	return (void *)malloc(size);
}

void
operator delete [] (void *ptr)
{
    if (ptr == NULL)
        return;

    if (g_pMemCacheList && g_bFastMallocAll)
    {
        MemCache* pMemCache = g_pMemCacheList[0];
        if (pMemCache)
        {
            pMemCache->CacheDelete((char*)ptr);
            return;
        }
    }

    if (shared_ready)
        SharedMemory::free((char *)ptr);
    else
        free(ptr);
}
#endif //defined HELIX_FEATURE_SERVER_HELIX_HEAP

