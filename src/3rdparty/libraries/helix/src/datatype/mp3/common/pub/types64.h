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

#ifndef _XDVDCOMMON_H_
#define _XDVDCOMMON_H_

#include "hxcom.h"
#include "hxthread.h"

typedef enum
{
    eErrorNone,             // Success
    eErrorNoData,           // Not enough data to decode
    eErrorInvalidArg,       // Invalid argument
    eErrorInvalidSource,    // Can not decode this stream
    eErrorBadSequence,      // Command issued in an illegal sequence
    eErrorUnknown,          // Generic error
    eErrorNoClock = -1      // Clock not available (yet)

} eXDvdError;

typedef enum
{
    eVideo,
    eAudio,
    eSubpicture,
    eNavigation
} ePacketType;

typedef enum
{
    eMPEG1,
    eMPEG2,
    eAC3,
    eLPCM,
    eDTS
} ePacketSubType;

typedef enum
{
    eIsPlaying = 0,
    eIsStopped = 1,
    eIsPaused = 2
} eXDvdState;

typedef enum
{
    eXDvdScanNone,
    eXDvdScanI,
    eXDvdScanIP,
    eXDvdScanSlowMotion
} eXDvdScanMode;

typedef enum
{
    eDimNormal,
    eDimWide,
    eDimFullScreen
} eDimensionType;

typedef enum
{
    eFormatLetterbox,
    eFormatPanScan,
    eFormatSameAsInput
} eDisplayFormatType;

// Packets
typedef struct
{
    UINT8       *pData;         // Pointer to the compressed data
    UINT32      lBytes;         // Size of the packet data

    UINT8       *pHeader;       // Pointer to the packet header
    UINT32      lHeaderBytes;   // Size of the packet including the header

    UINT8       *pPrivateData;  // Optional private data for packet
    UINT32      lPrivBytes;     // Size of private data
    
    ePacketType ePacket;        // Packet type
    ePacketSubType eSubtype;    // Packet data type

    INT64       llDts;          // Decode time stamp
    INT64       llPts;          // Display time stamp
    INT64       llDelta;        // Delta to mod the timestamps    
    
    char        cStreamId;      // Stream id from the packet header
    char        cHasDts;        // Packet has a valid decode time stamp
    char        cHasPts;        // Packet has a valid display time stamp
} Packet;

// Highlight stuff
typedef struct _XDVD_DVD_YUV
{
	UINT8   Reserved;
	UINT8   Y;
	UINT8   U;
	UINT8   V;
} XDVD_DVD_YUV, *PXDVD_DVD_YUV;

typedef struct _XDVD_PROPERTY_SPPAL
{
	XDVD_DVD_YUV sppal[16];
} XDVD_PROPERTY_SPPAL, *PXDVD_PROPERTY_SPPAL;


typedef struct _XDVD_COLCON
{
	UINT8 emph1col:4;
	UINT8 emph2col:4;
	UINT8 backcol:4;
	UINT8 patcol:4;
	UINT8 emph1con:4;
	UINT8 emph2con:4;
	UINT8 backcon:4;
	UINT8 patcon:4;

} XDVD_COLCON, *PXDVD_COLCON;

typedef struct _XDVD_PROPERTY_SPHLI
{
	UINT16      HLISS;      //
	UINT16      Reserved;
	UINT32      StartPTM;   // start presentation time in x/90000
	UINT32      EndPTM;     // end PTM in x/90000
	UINT16      StartX;
	UINT16      StartY;
	UINT16      StopX;
	UINT16      StopY;
	XDVD_COLCON ColCon;   // color contrast description (4 bytes as given in HLI)
} XDVD_PROPERTY_SPHLI, *PXDVD_PROPERTY_SPHLI;

// Helper classes
class CCritSec
{
public:
    CCritSec()  {HXMutex::MakeMutex(m_pMutex);}    
    ~CCritSec() {HX_DELETE(m_pMutex);}

    void        Lock(){m_pMutex->Lock();}
    void        Unlock(){m_pMutex->Unlock();}
protected:
    HXMutex    *m_pMutex;
};

// Locks a critical section, and unlocks it automatically
// when the lock goes out of scope
class CAutoLock
{
    // Make copy constructor and assignment operator inaccessible
    CAutoLock(const CAutoLock &refAutoLock);
    CAutoLock &operator=(const CAutoLock &refAutoLock);

protected:
    CCritSec *m_pLock;

public:
    CAutoLock(CCritSec * plock)
    {
        m_pLock = plock;
        m_pLock->Lock();
    }

    ~CAutoLock() {m_pLock->Unlock();}
};

// Basic clock.  Must implement GetTime_dw();
class CClock
{
public:
    CClock() {
                m_dwStartTime =
                m_dwStartPts = 0;

                m_lClockDelta = 0;
             };

    virtual ~CClock() {};

    virtual UINT32  GetTime_dw()=0;
    virtual void    ResetClock_v()=0;

    UINT32  GetStartPts_dw() {return m_dwStartPts;};
    UINT32  GetStartTime_dw() {return m_dwStartTime;};
    void    SetStartTime_v(UINT32 dwTime, UINT32 dwPts) {m_dwStartTime = dwTime;
                                                         m_dwStartPts = dwPts;
                                                         m_lClockDelta = 0;};
protected:
    UINT32  m_dwStartTime,
            m_dwStartPts;

    INT32   m_lClockDelta;
};

#endif
