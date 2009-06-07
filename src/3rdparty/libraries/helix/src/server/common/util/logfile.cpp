/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: logfile.cpp,v 1.2 2006/03/22 19:45:36 tknox Exp $
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


///////////////////////////////////////////////////////////////////////////////
// Includes
///////////////////////////////////////////////////////////////////////////////

#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#ifdef _UNIX
#include <unistd.h>
#endif

#include "logfile.h"

LogFile::LogFile()
    : m_bOK(TRUE)
    , m_ulLastErrorCode(0UL)
    , m_fpLogFile(NULL)
{
    m_pszFileName[0] = m_pszLastError[0] = '\0';
}

LogFile::~LogFile()
{
    if (m_fpLogFile)
    {
        fclose(m_fpLogFile);
    }
}

///////////////////////////////////////////////////////////////
// Actually, this function checks if there is an open log file,
// and if it has the same name as the new name. If they are
// different, or if no log file is open, it (if needed) closes
// the existing log file, then opens the new one, and sets the
// log file name to the new name.
///////////////////////////////////////////////////////////////
void
LogFile::SetFileName(const char *newFileName)
{
    SetOK(TRUE);  // Assume we are healthy until we find otherwise

    if (strncmp(newFileName, m_pszFileName, MAX_FILE_NAME_SIZE))
    {
        if (m_fpLogFile)
        {
            m_fpLogFile = freopen(newFileName, "a", m_fpLogFile);
        }
        else
        {
            m_fpLogFile = fopen (newFileName, "a");
        }

        if (!m_fpLogFile)
        {
            SaveError();
        }

        if (IsOK())
        {
            strncpy(m_pszFileName, newFileName, MAX_FILE_NAME_SIZE);
            m_pszFileName[MAX_FILE_NAME_SIZE] = '\0';
        }
    }
}

INT32
LogFile::Write(const char *message)
{
    if (!IsOK())
    {
        return 0;
    }

    INT32 result = fprintf(m_fpLogFile, "%s", message);
    if (result < 0)
    {
        SaveError();
    }

    return result;
}

void
LogFile::Flush()
{
    if (!IsOK())
    {
        return;
    }

    int result = fflush(m_fpLogFile);
    if (result < 0)
    {
        SaveError();
    }
}

UINT32
LogFile::GetLogFileSize()
{
    struct stat st;
    UINT32 retVal = 0;

    if (!IsOK() || !m_fpLogFile)
    {
        return 0;
    }

    if (fstat(fileno(m_fpLogFile), &st) == 0)
    {
        retVal = st.st_size;
    }
    else
    {
        SaveError();
    }

    return retVal;
}

void
LogFile::SaveError()
{
    SetOK(FALSE);
    m_ulLastErrorCode =  errno;
    strncpy(m_pszLastError, strerror(errno), MAX_ERROR_STRING_LENGTH);
    m_pszLastError[MAX_ERROR_STRING_LENGTH] = '\0';
}
