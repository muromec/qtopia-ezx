/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxdrmcore.h,v 1.2 2007/04/05 21:56:15 sfu Exp $
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

#ifndef _HX_DRMCORE_
#define _HX_DRMCORE_

#include "hxresult.h"
#include "hxtypes.h"
#include "hxcomm.h"
#include "hxfiles.h"
#include "strminfo.h"
#include "hxplugn.h"
#include "hxcore.h"

#include "hxsrcin.h"
#include "hxdrm.h"

// forward decl.
class HXSource;
class Plugin2Handler;

struct IHXPacket;
struct IHXValues;
struct IHXPlayer;


class HXDRM :   public IHXSourceInput
{
public:
    HXDRM(HXSource* pSource);

    // IUnknown methods
    STDMETHOD(QueryInterface)	(THIS_ REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)	(THIS);
    STDMETHOD_(ULONG32,Release)	(THIS);

    // IHXSourceInput methods
    STDMETHOD(OnFileHeader)     (THIS_ HX_RESULT status, IHXValues* pValues);
    STDMETHOD(OnStreamHeader)	(THIS_ HX_RESULT status, IHXValues* pValues);
    STDMETHOD(OnStreamDone)	(THIS_ HX_RESULT status, UINT16 unStreamNumber);
    STDMETHOD(OnPacket)		(THIS_ HX_RESULT status, IHXPacket* pPacket);
    STDMETHOD(OnTermination)	(THIS_ HX_RESULT status);

    // check to see if the source is helix drm protected
    static HXBOOL       IsProtected(IHXValues* pFileHeader);

    // load the DRM plugin matching the DRMId in the file header
    HX_RESULT           InitDRMPlugin(IHXValues* pHeader);

    // process the file header
    HX_RESULT           ProcessFileHeader(IHXValues* pHeader);

    // process the stream headers in HXSource
    HX_RESULT           ProcessStreamHeader(IHXValues* pHeader);

    // decrypt then send the packet to renderer
    HX_RESULT           ProcessEvent(CHXEvent* pEvent);

    // flush packets
    HX_RESULT           FlushPackets(HXBOOL bPushOutToCaller);

    // pre-recording hooks
    HX_RESULT           FileHeaderHook(IHXValues* pHeader);
    HX_RESULT           StreamHeaderHook(IHXValues* pHeader);

    virtual 		~HXDRM(void);

protected:

    LONG32		m_lRefCount;
    HXSource*		m_pSource;
    IHXPlayer*          m_pPlayer;
    IHXSourceHandler*   m_pDigitalRightsManager;
    CHXSimpleList	m_HXEventList;

};

#endif // _HX_DRMCORE_


