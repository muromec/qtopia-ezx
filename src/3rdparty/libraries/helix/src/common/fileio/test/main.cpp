/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: main.cpp,v 1.7 2004/07/09 18:20:32 hubbe Exp $
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

#include <stdio.h>
#include "hlxclib/string.h"
#include "hx_unit_test.h"
#include "hx_ut_debug.h"
#include "hxstrutl.h"

#define DFLT_INPUT_FILENAME "tihxfile.in"
#define DFLT_FILE_MODE "alldir"

char* pModeTable[] =
{
    "ihx",
    "chx",
    0
};


extern HLXUnitTest* BuildTest(const char* pModeString);

int main(int argc, char* argv[])
{
    int ret = 0;
    char pDefFileName[] = DFLT_INPUT_FILENAME;
    char pDefMode[] = DFLT_FILE_MODE;
    char* defargv[4] = {argv[0], pDefFileName, pDefMode, 0};
    int defargc = 3;
    char** appargv = &defargv[0];
    int appargc = defargc;
    int iModeIdx = 0;
    bool bAllModes = false;
    bool bAllDirModes = false;
    char pModeString[10]; /* Flawfinder: ignore */


    if (argc >= 2)
    {
	appargv[1] = argv[1];
	if (argc >= 3)
	{
	    appargc = argc;
	    appargv = argv;
	}
    }

    bAllDirModes = (strcmp(appargv[2], "alldir") == 0);
    bAllModes = bAllDirModes || (strcmp(appargv[2], "all") == 0);

    do
    {
	if (bAllModes)
	{
	    if (pModeTable[iModeIdx])
	    {
		SafeStrCpy(pModeString, pModeTable[iModeIdx++], 10);
		if (bAllDirModes)
		{
		    SafeStrCat(pModeString, "dir", 10);
		}

		appargv[2] = pModeString;
	    }
	    else
	    {
		appargv[2] = NULL;
	    }
	}

	if (!appargv[2])
	{
	    break;
	}

	HLXUnitTest* pUnitTest = BuildTest(appargv[2]);

	if (!pUnitTest->Init(appargc, appargv))
	{
	    DPRINTF(D_ERROR,
		    ("Unit test initialization FAILED\n"));
	    ret = -1;
	}
	else if (!pUnitTest->Start())
	{
	    DPRINTF(D_ERROR,
		    ("%s Unit test FAILED\n", appargv[2]));
	    ret = -1;
	}
	else
	{
	    DPRINTF(D_ERROR,
		    ("%s Unit test PASSED\n", appargv[2]));
	}

	delete pUnitTest;
    } while (bAllModes);

    return ret;
}
