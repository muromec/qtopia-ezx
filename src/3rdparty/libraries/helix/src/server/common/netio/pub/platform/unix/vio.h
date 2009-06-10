/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: vio.h,v 1.2 2003/01/23 23:42:50 damonlan Exp $ 
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
};

VIO::VIO(SocketIO* pSIO)
    : m_pSIO(pSIO)
{
}

int VIO::writevec(HX_IOVEC* pVectors, UINT32 ulCount)
{
    int ret = ::writev(m_pSIO->fd(), pVectors, ulCount);

    if (ret < 0)
    {
	m_nError = errno;
	m_bWouldBlock = (m_nError == EWOULDBLOCK || m_nError == EAGAIN);
	if (m_bWouldBlock)
	{
	    ret = 0;
	}
#ifdef SCRVERBOSE
	else
	{
	    perror("writev");
	    fprintf(stderr, "(%d)\n", m_pSIO->fd());
	}
#endif
	return ret;
    }

    m_nError = 0;
    m_bWouldBlock = FALSE;
    return ret;
}
    
