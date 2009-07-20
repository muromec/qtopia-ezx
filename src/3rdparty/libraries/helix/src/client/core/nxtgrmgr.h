/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: nxtgrmgr.h,v 1.3 2005/03/14 20:31:02 bobclark Exp $
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

#ifndef _NEXTGROUPMANAGER_
#define _NEXTGROUPMANAGER_

class	HXPlayer;
class	CHXSimpleList;
class   CHXString;
class 	SourceInfo;
struct	IHXGroup;
struct  IHXInterruptState;

class NextGroupManager
{
public:
    NextGroupManager(HXPlayer* pPlayer);
    ~NextGroupManager();

	void		Init();
    HX_RESULT	SetCurrentGroup(UINT16 uGroupNumber, IHXGroup* pGroup);
    HX_RESULT	GetCurrentGroup(UINT16& uCurrentGroup, IHXGroup*& pGroup);

    HX_RESULT	AddSource(SourceInfo* pSourceInfo);
    UINT16	GetNumSources(void);
    HX_RESULT	GetSource(UINT16 uIndex, SourceInfo*& pSourceInfo);
    HX_RESULT	RemoveSource(UINT16 uIndex, SourceInfo*& pSourceInfo);
    HX_RESULT	RemoveSource(SourceInfo* pSourceInfo);
    void	RemoveAllSources(void);
    HXBOOL	Lookup(HXSource* pSource, SourceInfo*& pSourceInfo);
    HX_RESULT	CanBeStarted(HXSource* pSource, SourceInfo* pThisSourceInfo);
    
    HX_RESULT	ProcessIdle(void);
    
    void	SetLastError(HX_RESULT theErr, HXSource* pSource = NULL, 
			     char* pUserString = NULL);

    HX_RESULT	GetLastError(HXSource*& pSource, const char*& pUserString)	
		{pSource = m_pErrorSource; pUserString = m_UserString; return m_LastError;};

    HXBOOL	ReportError (HXSource* pSource, 
			     HX_RESULT theErr, const char* pUserString = NULL);
    void	Cleanup(void);
    void	StopPreFetch(void);
    void	ContinuePreFetch(void);

    HX_RESULT	AddRepeatTrack(UINT16 uTrackIndex, IHXValues* pTrack);

protected:
    HXPlayer*		m_pPlayer;
    IHXInterruptState* m_pInterruptState;
    CHXSimpleList*	m_pSourceList;
    HX_RESULT		m_LastError;
    CHXString		m_UserString;
    UINT16		m_uGroupNumber : 16;    
    HX_BITFIELD		m_bStopPrefetch : 1;
    IHXGroup*		m_pGroup;
    HXSource*		m_pErrorSource;

    HX_RESULT	GetSourceInfo(UINT16 uTrackIndex, SourceInfo*& pSourceInfo);
};
#endif /*_NEXTGROUPMANAGER_*/
