/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: hxsocketdata.h,v 1.6 2006/02/16 23:07:05 ping Exp $ 
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

/* 

Helper class for HXThreadedSocket 

*/

#if !defined(HXSOCKETDATA_H__)
#define HXSOCKETDATA_H__

_INTERFACE IHXSockAddr;
_INTERFACE IHXBuffer;
_INTERFACE IHXMutex;
class CRingBuffer;

class HXSocketData
{
public:
    HXSocketData(IUnknown* pContext);
    ~HXSocketData();

    HX_RESULT Init(UINT32 cbBufferLimit, UINT32 cbAvgPacket);
    HX_RESULT Init(UINT32 slotCount);

    bool IsInitialized() const;
    HX_RESULT AddTail(IHXBuffer* pBuf, IHXSockAddr* pAddr = 0);
    void RemoveHead(IHXBuffer*& pBuf, IHXSockAddr*& pAddr);
    void RemoveHead(IHXBuffer*& pBuf);
    void PeekHead(IHXBuffer*& pBuf, IHXSockAddr*& pAddr);
    void PeekHead(IHXBuffer*& pBuf);
    void CleanUp();
    
    bool IsFull() const;
    UINT32 GetBytes() const;    // bytes accumulated
    UINT32 GetFree() const;     // slots free to hold data
    UINT32 GetCount() const;    // slots occupied with data

    bool TryResetEmptySignal();
    bool TryResetFullSignal();

private:

    // represents items stored in the container
    class Data
    {
    public:
        Data(IHXBuffer* pBuf, IHXSockAddr* pAddr = 0);
        ~Data();

        void Get(IHXBuffer*& pBuf, IHXSockAddr*& pAddr);
    private:

        IHXBuffer* m_pBuf;
        IHXSockAddr* m_pAddr;
    };

    UINT32          m_cbTotal;
    UINT32          m_cbMax;
    CRingBuffer*    m_pRing;
    IUnknown*	    m_pContext;
    IHXMutex*       m_pMutex;

    // for tracking transition to empty or full buffer
    bool            m_emptySignal;
    bool            m_fullSignal;
};

inline
UINT32 HXSocketData::GetBytes() const
{
    return m_cbTotal;
}


#endif //HXSOCKETDATA_H__





