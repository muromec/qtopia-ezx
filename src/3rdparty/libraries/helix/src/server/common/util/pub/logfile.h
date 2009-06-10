/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: logfile.h,v 1.2 2006/03/22 19:45:37 tknox Exp $
 *
 * Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved.
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
#include <string.h>
#include <errno.h>

#include "hxtypes.h"

///////////////////////////////////////////////////////////////////////////////
// LogFile class
///////////////////////////////////////////////////////////////////////////////

#define MAX_FILE_NAME_SIZE 255
#define MAX_ERROR_STRING_LENGTH 255

class LogFile
{

public:

    LogFile();
    ~LogFile();

    BOOL        IsOK() const { return m_bOK; };

    const char* GetFileName() const { return m_pszFileName; };
    void        SetFileName(const char *);

    INT32       Write(const char *message);
    void        Flush();

    UINT32      GetLogFileSize();

    const char* GetLastErrorStr()  const { return m_pszLastError; };
    UINT32      GetLastErrorCode() const { return m_ulLastErrorCode; };

private:

    void        SetOK(BOOL bOK) { m_bOK = bOK; };
    void        SaveError();

    BOOL        m_bOK;
    char        m_pszFileName[MAX_FILE_NAME_SIZE + 1];
    char        m_pszLastError[MAX_ERROR_STRING_LENGTH + 1];
    UINT32      m_ulLastErrorCode;
    FILE*       m_fpLogFile;
};
