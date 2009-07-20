/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxstat.h,v 1.7 2004/07/09 18:21:50 hubbe Exp $
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

#ifndef _HXSTATLOG_
#define _HXSTATLOG_

#ifdef _MACINTOSH
#include <stdlib.h>
#endif

#ifdef _WINDOWS
#include <stdio.h>
#endif

#include "hlxclib/stdarg.h"
#include "hxresult.h"
#include "hxtypes.h"
#include "../fileio/pub/chxdataf.h"


#ifdef HELIX_CONFIG_NOSTATICS
# include "globals/hxglobals.h"
#endif


class HXStatLog 
{
  public:
    HXStatLog();
    ~HXStatLog();
    HX_RESULT       Open_Read       (const char * filename);
    HX_RESULT       Open_Write      (const char * filename);
    HX_RESULT       Open_WriteNoAppend (const char * filename);
    HX_RESULT       Close           (void);
    LONG32          Read            (char *buf, ULONG32 max_buf_length);
    LONG32          Write           (char *buf, ULONG32 buf_length);
    HX_RESULT       StatPrintf      (const char* fmt, ...);
    HX_RESULT       ReadLine        (char *buf, ULONG32 buf_length);

    HX_RESULT       Seek            (ULONG32 offset, UINT16 fromWhere);
    ULONG32         Tell            (void);


  private:        
    CHXDataFile* mStatFile;
    char*        mStatBuffer;
    char*        mStatCurrentPosition;
    INT16        mStatBufferLength;
    INT16        mStatBytesLeft;
};


/////
// This is static implementation of the above class so that
// the log info can be written from anywahere in the player to the same
// log file...(opened once for each session)
/////

class HXStaticStatLog 
{
  public:
    static HX_RESULT Open_Read(const char* filename)
    { 
        if (LogFileInstance()) return HXR_ALREADY_OPEN;
        HX_RESULT theErr = HXR_OK;

        LogFileInstance() = new HXStatLog;
        if (!LogFileInstance())
            theErr = HXR_OUTOFMEMORY;

        if (!theErr)
        {
            theErr = LogFileInstance()->Open_Read(filename);
        }

        // error in opening file?
        if (theErr && LogFileInstance())
        {
            delete LogFileInstance();
            LogFileInstance() = NULL;
        }

        return theErr;
    }
                        
    static HX_RESULT Open_Write(const char* filename)
    { 
        if (LogFileInstance()) return HXR_ALREADY_OPEN;
        HX_RESULT theErr = HXR_OK;

        LogFileInstance() = new HXStatLog;
        if (!LogFileInstance())
            theErr = HXR_OUTOFMEMORY;

        if (!theErr)
        {
            theErr = LogFileInstance()->Open_Write(filename);
        }

        // error in opening file?
        if (theErr && LogFileInstance())
        {
            delete LogFileInstance();
            LogFileInstance() = NULL;
        }

        return theErr;
    }
                        
    static HX_RESULT Close(void)
    { 
        if (!LogFileInstance()) return HXR_INVALID_FILE;
        HX_RESULT theErr = HXR_OK;
        theErr = LogFileInstance()->Close();
        delete LogFileInstance();
        LogFileInstance() = NULL;
        return theErr;
    }
                         
    static LONG32 Read(char* buf, ULONG32 max_buf_length)
    { 
        if (!LogFileInstance()) return -1;
        return LogFileInstance()->Read(buf, max_buf_length);
    }
                        
    static LONG32 Write(char* buf, ULONG32 buf_length)
    { 
        if (!LogFileInstance()) return -1;
        return LogFileInstance()->Write(buf, buf_length);
    }
                        
    static HX_RESULT StatPrintf(const char* fmt, ...)
    { 
        if (!LogFileInstance()) return HXR_INVALID_FILE;
                        
        HX_RESULT theErr = HXR_OK;
#ifndef _MACINTOSH
        char buf[8096]; /* Flawfinder: ignore */
        LONG32 bytes_written = 0;
        LONG32 bytes_towrite = 0;
        va_list args;
        va_start(args, fmt);
        bytes_towrite = vsnprintf(buf, sizeof(buf), fmt, args);       // scanf
        va_end(args);
        if (bytes_towrite < 0)
            theErr = HXR_INVALID_PATH;              

        if (!theErr)
        {
            bytes_written = LogFileInstance()->Write(buf, (ULONG32) bytes_towrite);
            if (bytes_written != bytes_towrite)
                theErr = HXR_INVALID_PATH;              
        }
#endif
        return theErr;
    }
                        
    static HX_RESULT ReadLine (char *buf, ULONG32 buf_length)
    { 
        if (!LogFileInstance()) return HXR_INVALID_FILE;
        return LogFileInstance()->ReadLine(buf, buf_length);
    }
                        

    static HX_RESULT Seek (ULONG32 offset, UINT16 fromWhere)
    { 
        if (!LogFileInstance()) return HXR_INVALID_FILE;
        return LogFileInstance()->Seek(offset, fromWhere);
    }
                        
    static ULONG32 Tell (void)
    { 
        if (!LogFileInstance()) return (ULONG32) -1;
        return LogFileInstance()->Tell();
    }
    
    static inline HXStatLog*& LogFileInstance()
    {
#if defined(HELIX_CONFIG_NOSTATICS)    
            static const HXStatLog* const pmLogFile = NULL;
            return (HXStatLog*&)HXGlobalPtr::Get(&pmLogFile, NULL );
#else
            static HXStatLog* pmLogFile = NULL;
            return pmLogFile;
#endif    
     }
};

#endif //_HXSTATLOG_
