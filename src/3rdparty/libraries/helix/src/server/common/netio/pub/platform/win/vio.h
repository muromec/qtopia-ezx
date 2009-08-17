/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: vio.h,v 1.3 2004/05/13 20:42:17 tmarshall Exp $ 
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

// Simple class to handle OS level vector writes

class VIO 
{
public:
    VIO(SocketIO* pSIO);
    int writevec(HX_IOVEC* pVectors, UINT32 ulCount);
    int would_block() { return m_bWouldBlock; }
    int error() { return m_nError; }
private:
    BOOL m_bWouldBlock;
    SocketIO* m_pSIO;
    int m_nError;
    WSABUF m_pWSABUFs[HX_IOV_MAX];
};

VIO::VIO(SocketIO* pSIO)
    : m_pSIO(pSIO)
{
}

int VIO::writevec(HX_IOVEC* pVectors, UINT32 ulCount)
{
    // Need to convert to stupid non-standard vector format
    UINT32 ulByteCount = 0;
    UINT32 ulBytesSent = 0;
    UINT32 ulSentTotal = 0;

    for (int i = 0; i < (int)ulCount; i++)
    {
	m_pWSABUFs[i].buf = (char*)pVectors[i].iov_base;
	m_pWSABUFs[i].len = pVectors[i].iov_len;
	ulByteCount += pVectors[i].iov_len;
    }

    int ret = 0;

    while (ret == 0 && ulByteCount > ulSentTotal)
    {
    	ret = WSASend(m_pSIO->socket(), m_pWSABUFs, ulCount, &ulBytesSent,
		NULL, NULL, NULL);
	ulSentTotal += ulBytesSent;
    }

    if (ret != 0)
    {
	m_nError = WSAGetLastError();
	m_bWouldBlock = (m_nError == WSAEWOULDBLOCK);
	if (m_bWouldBlock)
	{
	    return ulSentTotal;
	}
	return -1;
    }

    m_nError = 0;
    m_bWouldBlock = FALSE;
    return ulSentTotal;
}
    
