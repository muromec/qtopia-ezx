/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: unix_hurl.cpp,v 1.5 2004/07/09 18:23:15 hubbe Exp $
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

#ifndef _UNIX
#error This is the UNIX hypernav stuff!!
#endif

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <stdlib.h>	// for exit() prototype
#include <string.h>
#include <stdio.h>

#include "hxassert.h"
#include "hxstrutl.h"
#include "unix_hurl.h"

#include "cunixprefutils.h"

//Globals for the url passing to child proc in UNIX.
#define MAX_URL_LEN 1024
#define MAX_EXEC_LEN 1024
int zn_anHURLPipe[2] = { -1, -1 };
int zm_nHurlProcID = -1;
int zn_anExecPipe[2] = { -1, -1 };
int zm_nExecProcID = -1;

char g_pURL[MAX_URL_LEN]; /* Flawfinder: ignore */
pid_t g_childPID;

void InitHurlListener()
{
    zn_anHURLPipe[0] = -1;
    zn_anHURLPipe[1] = -1;
    zn_anExecPipe[0] = -1;
    zn_anExecPipe[1] = -1;
}

void browse_child(int status)
{
    int chstatus;
    pid_t childPID;

    while((childPID = waitpid(0, &chstatus, WNOHANG)) > 0)
    {
	if(g_childPID == childPID)
	{
	    g_childPID = 0;

	    if(WEXITSTATUS(chstatus) != 0)
	    {
		if(fork() == 0)
		{
		    CUnixPrefUtils::CleanEnv();
		    execlp("netscape", "netscape", g_pURL, NULL);
		    _exit(0);
		}
	    }
	}
    }
}

void _ListenForHurlRequests()
{
    int  status = 0;
    char szURL[MAX_URL_LEN+1]; /* Flawfinder: ignore */

    char szBuff[MAX_URL_LEN+1]; /* Flawfinder: ignore */

    //If its there grab it.
    szBuff[0]= '\0';
    szURL[0]= '\0';
    status = 1;
    while( status != 0 )
    {
        status = ::read(zn_anHURLPipe[0], szBuff, MAX_URL_LEN);
        
        if( status > 0 )
        {
            if( (strlen(szURL)+status)<MAX_URL_LEN )
            {
                strncat( szURL, szBuff, status ); szURL[status] = '\0'; /* Flawfinder: ignore */
            }
            else
            {
                //URL is too long. We don't want to over run.
                HX_ASSERT( "URL exceeds MAX_URL_LEN" == NULL );
                //Reset everything and keep looping.
                status    = -1;
                errno     = EAGAIN;
                szURL[0]  ='\0';
                szBuff[0] ='\0';
            }
        }
        
        //Did we find an error?
        if( status < 0)
        {
            if( errno != EINTR && errno != EAGAIN )
            {
                //A really bad error....
                HX_ASSERT( "a really bad error" == NULL );
            }
            //otherwise just ignore it. We were just nonblocking or interupted
            //by a signal.
        }
        
        if( status > 0 && szBuff[status-1]=='\0' )
        {
            //We just received a newline for our URL. That means we are
            //ready to hurl it.
            char browsercmd[MAX_URL_LEN+100]; /* Flawfinder: ignore */
            if((g_childPID = fork()) == 0)
            {
                SafeSprintf(browsercmd, MAX_URL_LEN+100, "netscape -remote 'openURL(%s)' >/dev/null  2>&1", szURL);
		CUnixPrefUtils::CleanEnv();
                execlp("sh",  "sh", "-c", browsercmd, NULL);
                _exit(0);
            }
            else
            {
                SafeStrCpy(g_pURL, szURL, MAX_URL_LEN);
                signal(SIGCHLD, browse_child);
            }

            //Reset everything and keep looping.
            status = -1;
            szURL[0]='\0';
            szBuff[0]='\0';
        }
        
    } //while(status)

    ::close( zn_anHURLPipe[0] );
    zn_anHURLPipe[0]=-1;
    _exit(0);
}
void _ListenForExecRequests()
{
    int  status = 0;
    char szExec[MAX_EXEC_LEN+1]; /* Flawfinder: ignore */

    char szBuff[MAX_EXEC_LEN+1]; /* Flawfinder: ignore */

    //If its there grab it.
    szBuff[0]= '\0';
    szExec[0]= '\0';
    status = 1;
    while( status != 0 )
    {
        status = ::read(zn_anExecPipe[0], szBuff, MAX_EXEC_LEN);
        
        if( status > 0 )
        {
            if( (strlen(szExec)+status)<MAX_EXEC_LEN )
            {
                strncat( szExec, szBuff, status ); szExec[status] = '\0'; /* Flawfinder: ignore */
            }
            else
            {
                //URL is too long. We don't want to over run.
                HX_ASSERT( "EXEC string exceeds MAX_EXEC_LEN" == NULL );
                //Reset everything and keep looping.
                status     = -1;
                errno      = EAGAIN;
                szExec[0]  ='\0';
                szBuff[0]  ='\0';
            }
        }
        
        //Did we find an error?
        if( status < 0)
        {
            if( errno != EINTR && errno != EAGAIN )
            {
                //A really bad error....
                HX_ASSERT( "a really bad error" == NULL );
                _exit(0);
            }
            //otherwise just ignore it. We were just nonblocking or interupted
            //by a signal.
        }
        
        if( status > 0 && szBuff[status-1]=='\0' )
        {
            //We just received a newline for our exec string. We can exec now....
            char browsercmd[MAX_EXEC_LEN+100]; /* Flawfinder: ignore */
	    signal(SIGCHLD, browse_child);

            if(fork() == 0)
            {
		CUnixPrefUtils::CleanEnv();
                execlp("sh",  "sh", "-c", szExec, NULL);
                _exit(0);
            }

            //Reset everything and keep looping.
            status = -1;
            szExec[0]='\0';
            szBuff[0]='\0';
        }
        
    } //while(status)

    ::close( zn_anExecPipe[0] );
    zn_anExecPipe[0]=-1;
    _exit(0);
}

void ShutdownHurlListener()
{
    if( zn_anHURLPipe[0] != -1 )
        close( zn_anHURLPipe[0] );
    if( zn_anHURLPipe[1] != -1 )
        close( zn_anHURLPipe[1] );
    if( zn_anExecPipe[0] != -1 )
        close( zn_anExecPipe[0] );
    if( zn_anExecPipe[1] != -1 )
        close( zn_anExecPipe[1] );
    
    zn_anHURLPipe[0] = -1;
    zn_anHURLPipe[1] = -1; 
    zn_anExecPipe[0] = -1;
    zn_anExecPipe[1] = -1;
}

void StartHurlListener()
{
    if ( 0 != pipe(zn_anHURLPipe) )
    {
	//Can't create pipe.
	zn_anHURLPipe[0] = -1;
	zn_anHURLPipe[1] = -1;
        HX_ASSERT( "Can't create pipe for hurling...." == NULL );
    }

    //First create the hurling process.
    if( 0 > (zm_nHurlProcID = fork()))
    {
	//Error trying to fork.
	//What should we do?
        HX_ASSERT( "Can't fork for hurling....." == NULL );
    }
    if( 0 == zm_nHurlProcID )
    {
	//This is the child proc. Its life is the life of the parent so
        //we don't need to do a waitpid unless we want to.

	//Close the write end of the pipe.
	if ( 0 != ::close( zn_anHURLPipe[1]) )
	{
	    //close, error. Kill this child proc.
            //XXXGfw non fatal for now.
            HX_ASSERT( "Can't close pipe in child." == NULL );
	    //_exit(1);
	}
        zn_anHURLPipe[1]=-1;
        //
        // Enter a loop here that just looks for hurling requests
        // on the pipe and does a fork/exec for each one.
        //
        // zn_anHURLPipe[0] == Read end of pipe.
        // zn_anHURLPipe[1] == Write end of pipe.
        //
        // All urls are null terminated.
        // All reads are blocking. No reason no to be.

        //We never return from the following.
        _ListenForHurlRequests();
    }
    //Leave the exec pipe open for..........
    //The parent just returns after closing the read end of the pipe....
    //Close the read end of the pipe.
    if ( 0 != ::close( zn_anHURLPipe[0]) )
    {
        HX_ASSERT( "Can't close pipe in parent." == NULL );
    }
    zn_anHURLPipe[0]=-1;
    if ( 0 != pipe(zn_anExecPipe) )
    {
	//Can't create pipe.
	zn_anExecPipe[0] = -1;
	zn_anExecPipe[1] = -1;
        HX_ASSERT( "Can't create pipe for execing...." == NULL );
    }

    
    //Now create the process for execing.....
    if( 0 > (zm_nExecProcID = fork()))
    {
	//Error trying to fork.
	//What should we do?
        HX_ASSERT( "Can't fork for execing....." == NULL );
    }
    if( 0 == zm_nExecProcID )
    {
	//This is the child proc. Its life is the life of the parent so
        //we don't need to do a waitpid unless we want to.

	//Close the write end of the pipe.
	if ( 0 != ::close( zn_anExecPipe[1]) )
	{
	    //close, error. Kill this child proc.
            //XXXGfw non fatal for now.
            HX_ASSERT( "Can't close pipe in exec child." == NULL );
	    //_exit(1);
	}
        zn_anExecPipe[1]=-1;
        //
        // zn_anExecPipe[0] == Read end of pipe.
        // zn_anExecPipe[1] == Write end of pipe.
        //
        // All execs are null terminated.
        // All reads are blocking. No reason no to be.

        //We never return from the following.
        _ListenForExecRequests();
    }

    if ( 0 != ::close( zn_anExecPipe[0]) )
    {
        HX_ASSERT( "Can't close pipe in parent." == NULL );
    }
    zn_anExecPipe[0]=-1;
}

void SendHurlRequest(const char *pszURL)
{
    //We have started a child proc to do the hurling for us. If this is
    //the core thread we just send the request to the child proct via the
    //open pipe. If this is the child proc then we actually do it.
    if( pszURL && strlen(pszURL) && zn_anHURLPipe[1]>=0 )
        ::write( zn_anHURLPipe[1], pszURL, strlen(pszURL)+1 );
}
void SendExecRequest(const char *pszExec)
{
    //We have started a child proc to do the Exec for us. If this is
    //the core thread we just send the request to the child proct via the
    //open pipe. If this is the child proc then we actually do it.
    if( pszExec && strlen(pszExec) && zn_anExecPipe[1]>=0 )
    {
        ::write( zn_anExecPipe[1], pszExec, strlen(pszExec)+1 );
    }
}
