/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: ngtmetafile.cpp,v 1.2 2006/08/17 01:03:33 milko Exp $
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

/****************************************************************************
 * Defines
 */
#define NGT_UNPACK_UINT16(pData)    ((pData[0] << 8) + pData[1])
#define NGT_UNPACK_UINT32(pData)    ((pData[0] << 24) + (pData[1] << 16) + (pData[2] << 8) + pData[3]) 

#define NGT_PACK_UINT16(pData, uNum)	{pData[0] = (UINT8) (((uNum) >> 8) & 0x00FF); \
					 pData[1] = (UINT8) ((uNum) & 0x00FF);}
#define NGT_PACK_UINT32(pData, ulNum)	{pData[0] = (UINT8) (((ulNum) >> 24) & 0x000000FF);	\
					 pData[1] = (UINT8) (((ulNum) >> 16) & 0x000000FF);	\
					 pData[2] = (UINT8) (((ulNum) >> 8) & 0x000000FF);	\
					 pData[3] = (UINT8) ((ulNum) & 0x000000FF);}


/****************************************************************************
 * Includes
 */
#include "ngtmetafile.h"
#include "hlxclib/string.h"
#include "safestring.h"
#include "hxassert.h"

#include "hxstrutl.h"


/****************************************************************************
 * Constants
 */


/****************************************************************************
 *  CNGTMetaFileHeader
 */
CNGTMetaFileHeader::CNGTMetaFileHeader()
    : m_uFileVersion(NGTMETAFILE_CURRENT_VERSION)
    , m_uFileBodySize(0)
{
    ;
}

CNGTMetaFileHeader::~CNGTMetaFileHeader()
{
    ;
}


HX_RESULT CNGTMetaFileHeader::Unpack(UINT8 *pData, UINT32 ulSize)
{
    HX_RESULT retVal = HXR_HEADER_PARSE_ERROR;

    Clear();

    if (pData &&
	(ulSize >= GetPackedSize()) &&
	(pData[0] == 'N') &&
	(pData[1] == 'U') &&
	(pData[2] == 'G') &&
	(pData[3] == 'T'))
    {
	retVal = HXR_OK;

	pData += 4;
	m_uFileVersion = NGT_UNPACK_UINT16(pData);
	pData += 2;
	m_uFileBodySize = NGT_UNPACK_UINT16(pData);
    }

    return retVal;
}

HX_RESULT CNGTMetaFileHeader::Pack(UINT8 *pData, UINT32& ulSize)
{
    HX_RESULT retVal = HXR_FAIL;
    UINT32 ulPackedSize = GetPackedSize();

    if (pData &&
	(ulSize >= ulPackedSize))
    {
	retVal = HXR_OK;

	pData[0] = 'N';
	pData[1] = 'U';
	pData[2] = 'G';
	pData[3] = 'T';

	pData += 4;
	NGT_PACK_UINT16(pData, m_uFileVersion);
	pData += 2;
	NGT_PACK_UINT16(pData, m_uFileBodySize);

	ulSize = ulPackedSize;
    }

    return retVal;
}

void CNGTMetaFileHeader::Clear(void)
{
    m_uFileVersion = 0;
    m_uFileBodySize = 0;
}


/****************************************************************************
 *  CNGTMetaFileBody
 */
CNGTMetaFileBody::CNGTMetaFileBody()
    : m_ulExpiration(0)
    , m_ulConnectTime(0)
    , m_ulLocalDuration(0)
    , m_ulOverallDuration(0)
    , m_pLocalMimeType(NULL)
    , m_pRemoteSourceURL(NULL)
{
    ;
}

CNGTMetaFileBody::~CNGTMetaFileBody()
{
    HX_VECTOR_DELETE(m_pLocalMimeType);
    HX_VECTOR_DELETE(m_pRemoteSourceURL);
}


HX_RESULT CNGTMetaFileBody::Unpack(UINT8 *pData, UINT32 ulSize)
{
    HX_RESULT retVal = HXR_HEADER_PARSE_ERROR;

    Clear();

    if (pData &&
	(ulSize >= GetPackedSize()))
    {
	char* pEndOfString = NULL;
	UINT32 ulStringBufferLength = 0;
	
	retVal = HXR_OK;

	m_ulExpiration = NGT_UNPACK_UINT32(pData);
	pData += 4;
	m_ulConnectTime = NGT_UNPACK_UINT32(pData);
	pData += 4;
	m_ulLocalDuration = NGT_UNPACK_UINT32(pData);
	pData += 4;
	m_ulOverallDuration = NGT_UNPACK_UINT32(pData);
	pData += 4;

	ulSize -= 16;

	HX_ASSERT(ulSize >= 2);

	pEndOfString = StrNChr((const char *) pData, 0, ulSize);
	retVal = HXR_PARSE_ERROR;
	if (pEndOfString)
	{
	    ulStringBufferLength = (pEndOfString - ((char*) pData)) + 1;
	    m_pLocalMimeType = new char [ulStringBufferLength];
	    retVal = HXR_OUTOFMEMORY;
	    if (m_pLocalMimeType)
	    {
		SafeStrCpy(m_pLocalMimeType, (const char*) pData, ulStringBufferLength);
		pData = (UINT8*) (pEndOfString + 1);
		retVal = HXR_OK;
	    }
	}

	if (SUCCEEDED(retVal))
	{
	    HX_ASSERT(ulSize >= ulStringBufferLength);
	    ulSize -= ulStringBufferLength;
	    
	    retVal = HXR_PARSE_ERROR;

	    if (ulSize > 0)
	    {
		pEndOfString = StrNChr((const char *) pData, 0, ulSize);
		if (pEndOfString)
		{
		    ulStringBufferLength = (pEndOfString - ((char*) pData)) + 1;
		    m_pRemoteSourceURL = new char [ulStringBufferLength];
		    retVal = HXR_OUTOFMEMORY;
		    if (m_pRemoteSourceURL)
		    {
			SafeStrCpy(m_pRemoteSourceURL, (const char*) pData, ulStringBufferLength);
			retVal = HXR_OK;
		    }
		}
	    }
	}

	if (FAILED(retVal))
	{
	    Clear();
	}
    }

    return retVal;
}

HX_RESULT CNGTMetaFileBody::Pack(UINT8 *pData, UINT32& ulSize)
{
    HX_RESULT retVal = HXR_FAIL;
    UINT32 ulPackedSize = GetPackedSize();

    if (pData &&
	(ulSize >= ulPackedSize))
    {
	char* pEndOfString = NULL;
	retVal = HXR_OK;

	NGT_PACK_UINT32(pData, m_ulExpiration);
	pData += 4;
	NGT_PACK_UINT32(pData, m_ulConnectTime);
	pData += 4;
	NGT_PACK_UINT32(pData, m_ulLocalDuration);
	pData += 4;
	NGT_PACK_UINT32(pData, m_ulOverallDuration);
	pData += 4;

	ulSize -= 16;

	if (m_pLocalMimeType)
	{
	    SafeStrCpy((char*) pData, m_pLocalMimeType, ulSize);
	}
	else
	{
	    *pData = 0;
	}
	pEndOfString = StrNChr((const char *) pData, 0, ulSize);

	HX_ASSERT(pEndOfString);

	pEndOfString++;
	ulSize -= (pEndOfString - ((const char*) pData));
	pData = (UINT8*) pEndOfString;

	if (m_pRemoteSourceURL)
	{
	    SafeStrCpy((char*) pData, m_pRemoteSourceURL, ulSize);
	}
	else
	{
	    *pData = 0;
	}

	ulSize = ulPackedSize;
    }

    return retVal;
}

void CNGTMetaFileBody::Clear(void)
{
    m_ulExpiration = 0;
    m_ulConnectTime = 0;
    m_ulLocalDuration = 0;
    m_ulOverallDuration = 0;
    HX_VECTOR_DELETE(m_pLocalMimeType);
    HX_VECTOR_DELETE(m_pRemoteSourceURL);
}

UINT32 CNGTMetaFileBody::GetPackedSize(void)
{
    UINT32 ulSize = 18;

    if (m_pLocalMimeType)
    {
	ulSize += strlen(m_pLocalMimeType);
    }

    if (m_pRemoteSourceURL)
    {
	ulSize += strlen(m_pRemoteSourceURL);
    }

    return ulSize;
}
