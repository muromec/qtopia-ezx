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

#ifndef AMR_RS_ITR_H
#define AMR_RS_ITR_H

#include "hxtypes.h"
#include "amr_flavor.h"

class AMRTOCInfo;

class AMRRobustSortingItr
{
public:
    AMRRobustSortingItr();
    ~AMRRobustSortingItr();

    HXBOOL Init(AMRFlavor flavor, ULONG32 ulChannels, 
	      ULONG32 ulStartBlock, ULONG32 ulBlockInc,
	      const AMRTOCInfo& tocInfo);

    ULONG32 Block() const;
    ULONG32 Offset() const;

    HXBOOL More() const;
    void Next();

private:
    class FrameInfo
    {
    public:
	FrameInfo();
	
	void Init(ULONG32 ulBlock, ULONG32 ulOffset, ULONG32 ulSize);
	
	ULONG32 Block() const;
	ULONG32 Offset() const;
	ULONG32 Size() const;

	void DecSize();
    private:
	ULONG32 m_ulBlock;
	ULONG32 m_ulOffset;
	ULONG32 m_ulSize;
    };

    void IncFrameIndex();

    HXBOOL m_bMore;

    ULONG32 m_ulFrameIndex;
    ULONG32 m_ulOffset;

    ULONG32 m_ulFrameCount;
    FrameInfo* m_pFrameInfo;
};

#endif /* AMR_RS_ITR_H */
