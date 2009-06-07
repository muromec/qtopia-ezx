/* ***** BEGIN LICENSE BLOCK ***** 
 * Version: RCSL 1.0/RPSL 1.0 
 *  
 * Portions Copyright (c) 1995-2002 RealNetworks, Inc. All Rights Reserved. 
 *      
 * The contents of this file, and the files included with this file, are 
 * subject to the current version of the RealNetworks Public Source License 
 * Version 1.0 (the "RPSL") available at 
 * http://www.helixcommunity.org/content/rpsl unless you have licensed 
 * the file under the RealNetworks Community Source License Version 1.0 
 * (the "RCSL") available at http://www.helixcommunity.org/content/rcsl, 
 * in which case the RCSL will apply. You may also obtain the license terms 
 * directly from RealNetworks.  You may not use this file except in 
 * compliance with the RPSL or, if you have a valid RCSL with RealNetworks 
 * applicable to this file, the RCSL.  Please see the applicable RPSL or 
 * RCSL for the rights, obligations and limitations governing use of the 
 * contents of the file.  
 *  
 * This file is part of the Helix DNA Technology. RealNetworks is the 
 * developer of the Original Code and owns the copyrights in the portions 
 * it created. 
 *  
 * This file, and the files included with this file, is distributed and made 
 * available on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER 
 * EXPRESS OR IMPLIED, AND REALNETWORKS HEREBY DISCLAIMS ALL SUCH WARRANTIES, 
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS 
 * FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
 * 
 * Technology Compatibility Kit Test Suite(s) Location: 
 *    http://www.helixcommunity.org/content/tck 
 * 
 * Contributor(s): 
 *  
 * ***** END LICENSE BLOCK ***** */ 

#ifndef _MPAPKTPARSE_H_
#define _MPAPKTPARSE_H_

class CHXTimestampConverter;

/*
 * CMpaPacketParser: Basic, unformatted MPEG Audio streams
 */
class CMpaPacketParser : public CPacketParser
{
public:
    CMpaPacketParser();
    virtual ~CMpaPacketParser();

    /*
     *  CPacketParser methods
     */
    HX_RESULT   AddPacket       (IHXPacket* pPacket, INT32 streamOffsetTime);
    HX_RESULT   RenderAll       (void);
    void        RestartStream   (void);

protected:
    virtual inline HXBOOL ParseHeaders(UCHAR*& pBuffer, UINT32& ulSize)
    {
        return TRUE;
    }

    virtual void    HandlePacketLoss() {};

    IHXPacket*     m_pPacket;
    HXBOOL            m_bPacketLoss;
    INT32           m_lStreamOffsetTime;    
};

/*
 * C2250PacketParser: RFC 2250 streams (audio or system)
 */
class C2250PacketParser : public CMpaPacketParser
{
public:
    C2250PacketParser() : CMpaPacketParser() {m_pTsConvert=NULL;}
    virtual ~C2250PacketParser();

    void    PostSeek(UINT32 time);

protected:
    HXBOOL ParseHeaders(UCHAR*& pBuffer, UINT32& ulSize);
    void HandlePacketLoss();

    CHXTimestampConverter* m_pTsConvert;
};

#ifdef DEMUXER
/*
 * CSysPacketParser: System streams
 */
class CSysPacketParser : public CMpaPacketParser
{
public:
    CSysPacketParser(HXBOOL bMPEG2, IHXRegistry* pRegistry);
    virtual ~CSysPacketParser() {}
    inline void PostSeek(UINT32 timeAfterSeek)
    {        
        CMpaPacketParser::PostSeek(timeAfterSeek);
        m_ulPlayTime = timeAfterSeek;
        m_llLastPts = 0;
    }

protected:
    virtual inline HXBOOL ParseHeaders(UCHAR*& pBuffer, UINT32& ulSize)
    {
        return Demux(pBuffer, ulSize, m_ulDecBufBytes);
    }

    HXBOOL    Demux(UCHAR*& pBuffer, UINT32& ulSize, UINT32 ulFragSize);

    CDemuxer*    m_pDemuxer;
    IHXRegistry* m_pRegistry;
    INT64           m_llLastPts;
    INT64           m_llFirstPts;
    UINT32          m_ulPlayTime; 

    HXBOOL            m_bCheckVcdBug;     // Check once for vcd pts bug
    HXBOOL            m_bVcdBug;          // Encoded pts refer to first
                                        // byte in packet.
};

class C2250SysPacketParser : public C2250PacketParser, 
                             public CSysPacketParser
{
public:
    C2250SysPacketParser(HXBOOL bMPEG2, IHXRegistry* pRegistry) : 
        CSysPacketParser(bMPEG2, pRegistry), C2250PacketParser() {}
      
    ~C2250SysPacketParser() {}

protected:
    virtual inline HXBOOL ParseHeaders(UCHAR*& pBuffer, UINT32& ulSize)
    {
        return C2250PacketParser::ParseHeaders(pBuffer, ulSize) ?
            CSysPacketParser::ParseHeaders(pBuffer, ulSize) : 
            FALSE;
    }
};

#endif // DEMUXER
#endif //_MPAPKTPARSE_H_
