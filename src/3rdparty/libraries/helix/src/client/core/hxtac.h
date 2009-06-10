/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxtac.h,v 1.11 2007/07/06 21:58:11 jfinnecy Exp $
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
 * terms of the GNU General Public License Version 2 (the
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

#ifndef _MASTERTAC_H_
#define _MASTERTAC_H_



typedef enum _TACStatus
{
    TAC_Pending = 0,
    TAC_Source = 1,
    TAC_Track = 2,
    TAC_Group = 3,
	TAC_Presentation = 4
} TACStatus;

#define NUMB_TAC_NAMES 6

// forward class declares
class HXPlayer;
class HXClientRegistry;
class HXBasicGroupManager;



// HXMasterTAC class definition
class HXMasterTAC 
#if defined(HELIX_FEATURE_REGISTRY)
		: public IHXPropWatchResponse
#endif /* HELIX_FEATURE_REGISTRY */
{

public:
    HXMasterTAC(IUnknown* pContext, HXBasicGroupManager* pGroupManager);
    virtual ~HXMasterTAC();

    void		SetRegistry(HXClientRegistry* pRegistry, UINT32 ID);

    void		SetTAC(IHXValues* tacProps, TACStatus status);
    void		ResetTAC(HXBOOL bResetStatus = TRUE, HXBOOL bClearMasterProps = FALSE);
    HXBOOL		CheckTrackAndSourceOnTrackStarted(INT32 nGroup, INT32 nTrack, UINT32 sourceID);

    HXBOOL		CheckPresentationForTACInfo();
    HXBOOL		CheckTrackForTACInfo(INT32 nGroup, INT32 nTrack);
    HXBOOL		CheckSourceForTACInfo(INT32 nGroup, INT32 nTrack, UINT32 sourceID);

    HXBOOL		CheckGroupForTACInfo(INT32 nGroup);

    HXBOOL		IsTACComplete(IHXValues* pProps);
    void		Close();

protected:
    LONG32		m_lRefCount;

public:

    /*
     * IUnknown methods
     */
    STDMETHOD(QueryInterface)	(THIS_
				REFIID riid,
				void** ppvObj);

    STDMETHOD_(ULONG32,AddRef)	(THIS);

    STDMETHOD_(ULONG32,Release)	(THIS);

#if defined(HELIX_FEATURE_REGISTRY)
    //IHXPropWatchResponse methods
    STDMETHOD(AddedProp)	(THIS_
				const UINT32		id,
				const HXPropType   	propType,
				const UINT32		ulParentHash);

    STDMETHOD(ModifiedProp)	(THIS_
				const UINT32		id,
				const HXPropType   	propType,
				const UINT32		ulParentHash);

    STDMETHOD(DeletedProp)	(THIS_
				const UINT32		id,
				const UINT32		ulParentHash);
#endif /* HELIX_FEATURE_REGISTRY */

private:
#if defined(HELIX_FEATURE_REGISTRY)
    HXClientRegistry*		m_pRegistry;
#endif /* HELIX_FEATURE_REGISTRY */
    HXBasicGroupManager*	m_pGroupManager;

    IUnknown*			m_pContext;
    IHXValues*			m_pTACProps;
    TACStatus			m_tacStatus;
    IHXPropWatch*		m_pTACPropWatch;
    //UINT32			m_tacPropIDs[4];
    CHXMapLongToObj*	m_ptacPropIDs;
    UINT32              m_masterTACPropIDs[NUMB_TAC_NAMES];

    void    RetrieveTACProperties(IHXValues* pFromProps);
};

// TAC data class
class TACData
{
public:
    TACData()
    {
	m_titleID = 0;
	m_authorID = 0;
	m_copyrightID = 0;
	m_abstractID = 0;
	m_keywordsID = 0;
	m_descriptionID = 0;
    }

    UINT32 FindMasterIndex(UINT32 sourcePropID);
    void ClearAll(IHXPropWatch* pPropWatch);
    HXBOOL IsIDPresent(UINT32 sourcePropID);
    void Clear(UINT32 sourcePropID);
    void SetPropAndWatch(UINT32 n, UINT32 propID, IHXPropWatch* pPropWatch);

    static const UINT32 NoFind;

    UINT32 m_titleID;
    UINT32 m_authorID;
    UINT32 m_copyrightID;
    UINT32 m_abstractID;
    UINT32 m_keywordsID;
    UINT32 m_descriptionID;
};

#endif	// _MASTERTAC_H_

