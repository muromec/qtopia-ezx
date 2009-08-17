/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: cunixprefutils.cpp,v 1.7 2004/07/09 18:23:15 hubbe Exp $
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

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include "dbcs.h"
//#include "machdep.h"
#if defined (_SOLARIS) || defined (_FREEBSD) || defined (_OPENBSD) || defined (_NETBSD)
#include <dirent.h>
#elif defined (__hpux)
#include <sys/dirent.h>
#else
#include <sys/dir.h>
#endif 
#include "cunixprefutils.h"
#include "hxver.h"

////////////////////////////////////////////////////////////////////////////////////////////
// This function determins the location of the preferences file. If the directory that it
// determins doesn't exist, it will create it.
// /////////////////////////////////////////////////////////////////////////////////////////

void CUnixPrefUtils::GetUserHomeDirectory(uid_t uid, CHXString &dir)
{
    struct passwd *passwd = NULL;
    if ( passwd = getpwuid( uid ) )
    {
	dir = passwd->pw_dir ;
    }
    else
    {
	if (getenv("HOME"))
	{
	    dir = getenv("HOME");
	}
	else
	{
	    dir = "/tmp/";
	}
    }
}

void CUnixPrefUtils::GetPrefPath(char* pszPrefPath, int nLength, const char* pszCompanyName)
{
	//prefs file should be in user's home directory
#if defined(_BEOS) /* PJG -- Can someone take care of integrating this into GetUserHomeDirectory?  I dont have a beos box... */
	BPath   settingsPath;
	if (find_directory(B_COMMON_SETTINGS_DIRECTORY, &settingsPath) == B_OK)
	{
		if( strlen(settingsPath.Path()) < nLength)
	       strcpy( pzPrefPath, settingsPath.Path() ); /* Flawfinder: ignore */
	}
	else
	{
		char* pszTemp = getenv( "HOME" );
		if( strlen(pszTemp) < nLength)
			strcpy( pszPrefPath,  pszTemp); /* Flawfinder: ignore */
	}
#else
	CHXString dir;
	GetUserHomeDirectory(getuid(), dir);
	if( strlen(dir) < nLength)
	    ::strcpy( pszPrefPath, dir ); /* Flawfinder: ignore */
#endif /* _BEOS */
	if(!pszCompanyName)
	{
		if( (strlen(pszPrefPath) + strlen(HXVER_COMMUNITY) + 3) < nLength)
        {
			strcat(pszPrefPath, "/."); /* Flawfinder: ignore */
			strcat(pszPrefPath, HXVER_COMMUNITY); /* Flawfinder: ignore */
        }
    }	
	else if( (strlen(pszPrefPath) + strlen(pszCompanyName) + 2) < nLength)
	{
    	char* pszTempCompName = new char[strlen(pszCompanyName) + 1];
		strcpy(pszTempCompName, pszCompanyName); /* Flawfinder: ignore */
	    //remove any , or space
		char * pComa = (char*) HXFindChar(pszTempCompName, ',');
		if(pComa)
		{
			*pComa = '\0';
		}
	    pComa = (char*) HXFindChar(pszTempCompName, ' ');
        if(pComa)
        {
        	*pComa = '\0';
        }			
		//Let's make the company name caps insensitive since we don't know if it will come in caps or not.
		for(int i=0; i<strlen(pszTempCompName); i++)
	        pszTempCompName[i] = tolower((int)pszTempCompName[i]);
		
		strcat(pszPrefPath, "/."); /* Flawfinder: ignore */
		strcat( pszPrefPath, pszTempCompName ); /* Flawfinder: ignore */
		delete[] pszTempCompName;
	}
	DIR* pDir = NULL;
	pDir = opendir(pszPrefPath);
	if (pDir)
	{
	    closedir(pDir); //Directory exists.
	}
	else
	{
		mkdir(pszPrefPath, 0755);		
	}
}     

void
CUnixPrefUtils::CleanEnv()
 {
        char **ppSrc = environ;
        char **ppDest = environ;
        while (*ppSrc)
        {
            if (*ppDest && !strnicmp(*ppDest, "rmapref_", 8))
            {
                HX_VECTOR_DELETE(*ppDest);
                *ppDest = *ppSrc++;
            }
            else
            {
                *ppDest++ = *ppSrc++;
            }
        }
        *ppDest = 0;
 }
