/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: multilog.cpp,v 1.8 2005/03/14 19:36:27 bobclark Exp $
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

// system
#include "hlxclib/stdio.h"
#include "hlxclib/stdlib.h"
#include "hlxclib/stdarg.h"
#include "hlxclib/string.h"
// include
#include "hxtypes.h"
#include "hxcom.h"
#include "hxerror.h"
#include "hxprefs.h"
#include "ihxpckts.h"
#include "hxstrutl.h"
#include "hxprefutil.h"
// pndebug
#include "debugout.h"
#include "hxassert.h"
#include "multilog.h"

#ifdef _OPENWAVE
#include <op_fs.h>
#endif /* _OPENWAVE */

#define MULTILOG_BUFFER_SIZE  1024

void MultiLog(HXBOOL               bOutputToFile,
              HXBOOL               bOutputToDebugger,
              HXBOOL               bOutputToCoreDebug,
              const char*        pszFileName,
              IHXErrorMessages* pErrorMessages,
              UINT32             ulCoreDebugUserCode,
              const char*        pszRegKey,
              const char*        pszMsg,
              va_list            varargs)
{
    if (bOutputToFile || bOutputToDebugger || bOutputToCoreDebug)
    {
        // Check if the string contains variable arguments.
        // We also have to allow for the literal '%' percent
        // character which would be "%%".
        //
        // Check for any '%' characters
        char* pPct = (char*)strchr(pszMsg, '%');
        // Make sure that this isn't a "%%".
        while(pPct && *(pPct + 1) == '%')
        {
            pPct = (char*)strchr((pPct + 2), '%');
        }
        // Create the log message
        char szDbgStr[MULTILOG_BUFFER_SIZE]; /* Flawfinder: ignore */
        if(pPct)
        {
            vsnprintf(szDbgStr, sizeof(szDbgStr), pszMsg, varargs);
        }
        else
        {
            // There are no substitutions to be done,
            // so we can just strcpy the string
            SafeStrCpy(szDbgStr, pszMsg, MULTILOG_BUFFER_SIZE);
        }
        // If a file one of the outputs?
        if (bOutputToFile)
        {
            // Do we have a filename?
            if (pszFileName)
            {
                // Open the file
#ifdef _OPENWAVE
                OpFsFd fd = OpFsOpen(pszFileName,
                                     kOpFsFlagWrOnly | kOpFsFlagCreate,
                                     0);
                if (fd != kOpFsErrAny)
                {
                    OpFsSeek(fd, 0, kOpFsSeekEnd);
                    OpFsWrite(fd, szDbgStr, strlen(szDbgStr));
                    OpFsClose(fd);
                }
#else
                FILE* fp = fopen(pszFileName, "a+");
                if (fp)
                {
                    fprintf(fp, "%s", szDbgStr);
                    fflush(fp);
                    fclose(fp);
                }
#endif /* _OPENWAVE */
            }
        }
        // Is debugger window one of the outputs?
        if (bOutputToDebugger)
        {
            HXOutputDebugString(szDbgStr);
        }
        // Is the Core Debug window one of the targets?
        if (bOutputToCoreDebug)
        {
            // Do we have an IHXErrorMessages pointer?
            if (pErrorMessages)
            {
                // Initially we say that we will output to core debug
                HXBOOL bDoOutput = TRUE;
                // Do we have a regkey string?
                if (pszRegKey)
                {
                    // By putting a regKey string, the user is saying
                    // that they only want to see this be output to
                    // core debug if the regKey is set to "1". So now
                    // the default is that we will NOT output.
                    bDoOutput = FALSE;
                    // QI IHXErrorMessages for IHXPreferences
                    IHXPreferences* pPref = NULL;
                    pErrorMessages->QueryInterface(IID_IHXPreferences, (void**) &pPref);
                    if (pPref)
                    {
			ReadPrefBOOL(pPref, pszRegKey, bDoOutput);
		    }
                    HX_RELEASE(pPref);
                }
                // Now, after potentially checking the regKey,
                // are we *still* supposed to output?
                if (bDoOutput)
                {
                    // The core debug window prints out a funky
                    // character when it sees '\n'. However, it
                    // seems to be fine when it sees "\r\n". So
                    // here we go through and replace any lone
                    // '\n' with "\r\n", unless '\n' is the last
                    // character in the string.
                    char* pLF = (char*)strchr(szDbgStr, '\n');
                    if (pLF)
                    {
                        // We have at least one '\n' character. Is
                        // the first '\n' character the last one
                        // in the string?
                        if (*(pLF + 1) == '\0')
                        {
                            // We have a '\n' character, but there's only
                            // one and it's the last character in the string.
                            // Therefore, we can just NULL it out
                            *pLF = '\0';
                        }
                        else
                        {
                            // No, we have a '\n' and it's not at the
                            // end of the string. So we must go through
                            // the string and replace '\n' with "\r\n".
                            char szTmpStr[MULTILOG_BUFFER_SIZE];
                            char* pSrc = &szDbgStr[0];
                            char* pDst = &szTmpStr[0];
                            while ((pSrc - szDbgStr) < MULTILOG_BUFFER_SIZE && *pSrc && (pDst - szTmpStr) < MULTILOG_BUFFER_SIZE)
                            {
                                if (*pSrc == '\n')
                                {
                                    // Is this the last character in
                                    // the string? If so, we don't copy it
                                    if (*(pSrc + 1) != '\0')
                                    {
                                        // Is the character before it
                                        // already a '\r"? If so, then
                                        // we don't need to do the replacement
                                        if (*(pSrc - 1) != '\r')
                                        {
                                            // Replace the '\n' with a "\r\n"
                                            *pDst++ = '\r';
                                            if ((pDst - szTmpStr) < MULTILOG_BUFFER_SIZE) *pDst++ = *pSrc++;
                                        }
                                        else
                                        {
                                            *pDst++ = *pSrc++;
                                        }
                                    }
                                    else
                                    {
                                        // Advance the source
                                        pSrc++;
                                    }
                                }
                                else
                                {
                                    // This character is not the '\n',
                                    // so just copy it.
                                    *pDst++ = *pSrc++;
                                }
                            }
                            // Copy NULL at end
                            if ((pDst - szTmpStr) < MULTILOG_BUFFER_SIZE) *pDst++ = '\0';
                            // Now copy the temp string over the old one
                            SafeStrCpy(szDbgStr, szTmpStr, MULTILOG_BUFFER_SIZE);
                        }
                    }

                    // Now we can finally call IHXErrorMessages::Report
                    pErrorMessages->Report(HXLOG_DEBUG,         // severity
                                           HXR_OK,              // RMA Code
                                           ulCoreDebugUserCode, // user code
                                           szDbgStr,            // string
                                           NULL);               // More Info URL
                }
            }
        }
    }
}
