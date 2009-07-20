/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: statinfo.h,v 1.11 2006/02/07 19:21:09 ping Exp $
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

#ifndef	_STATINFO_H_
#define _STATINFO_H_

#define REG_TYPE_STRING	    100
#define REG_TYPE_NUMBER	    101
#define REG_TYPE_COMPOSITE  102
#define REG_TYPE_UNKNOWN    200

class CStatisticEntry
{
public:

    IUnknown*		m_pContext;
    IHXRegistry*	m_pRegistry; 
    UINT32		m_ulRegistryID;
    UINT32		m_ulType;
    HXBOOL		m_bAddKey;
   

			CStatisticEntry(IUnknown*	    /*IN*/  pContext,
					char*		    /*IN*/  pszRegKey,
					UINT32		    /*IN*/  ulType);
			
			~CStatisticEntry(void);

    HX_RESULT		SetStr(char*	/*IN*/  pszValue);
    HX_RESULT		SetInt(INT32	/*IN*/	lValue);
    
    INT32		GetInt(void);
    char*		GetStr(void);

    CStatisticEntry&    operator=(CStatisticEntry& rhs);    
};

class STATS
{
public:
    STATS(IUnknown*	    /*IN*/ pContext,
	  UINT32	    /*IN*/ ulRegistryID);
    virtual ~STATS();

    virtual void	Reset(void);
    
    //Assignment operator. Copies only the values, will not change
    //the parent, registry or anything else.
    STATS& operator=(const STATS& rhs );    

    HX_RESULT		m_lastError;

    IUnknown*		m_pContext;
    IHXRegistry*	m_pRegistry;
    UINT32		m_ulRegistryID;
    HXBOOL		m_bInitialized;
   
    CStatisticEntry*	m_pNormal;
    CStatisticEntry*	m_pRecovered;
    CStatisticEntry*	m_pReceived;
    CStatisticEntry*	m_pOutOfOrder;
    CStatisticEntry*	m_pFirstTimestamp;
    CStatisticEntry*	m_pLastTimestamp;
    CStatisticEntry*	m_pFilledBufferSize;
    CStatisticEntry*	m_pLost;
    CStatisticEntry*	m_pLate;
    CStatisticEntry*	m_pDuplicate;
    CStatisticEntry*	m_pTotal;
    CStatisticEntry*	m_pLost30;
    CStatisticEntry*	m_pTotal30;
    CStatisticEntry*	m_pResendRequested;
    CStatisticEntry*	m_pResendReceived;
    CStatisticEntry*	m_pClipBandwidth;
    CStatisticEntry*	m_pAvgBandwidth;
    CStatisticEntry*	m_pCurBandwidth;
    CStatisticEntry*	m_pHighLatency;
    CStatisticEntry*	m_pLowLatency;
    CStatisticEntry*	m_pAvgLatency;
};

class PLAYER_STATS : public STATS
{
public:
    PLAYER_STATS(IUnknown*	    /*IN*/  pUnknown,
		 UINT32		    /*IN*/  ulRegistryID);
    virtual ~PLAYER_STATS();

    virtual void	Reset(void);

    //Assignment operator. Copies only the values, will not change
    //the parent, registry or anything else.
    PLAYER_STATS& operator=(const PLAYER_STATS& rhs );

    CStatisticEntry*	m_pBufferingMode;
};

class SOURCE_STATS : public STATS
{
public:
    SOURCE_STATS(IUnknown*	    /*IN*/  pUnknown,
		 UINT32		    /*IN*/  ulRegistryID);
    virtual ~SOURCE_STATS();

    virtual void	Reset(void);

    //Assignment operator. Copies only the values, will not change
    //the parent, registry or anything else.
    SOURCE_STATS& operator=(const SOURCE_STATS& rhs );

    CStatisticEntry*	m_pTransportMode;
    CStatisticEntry*	m_pBufferingMode;
    CStatisticEntry*	m_pSourceName;
    CStatisticEntry*	m_pServerInfo;
    CStatisticEntry*	m_pProtocolVersion;
    CStatisticEntry*	m_pProtocol;
    
    CStatisticEntry*	m_pTitle;
    CStatisticEntry*	m_pAuthor;
    CStatisticEntry*	m_pCopyright;
    CStatisticEntry*	m_pAbstract;
    CStatisticEntry*	m_pDescription;
    CStatisticEntry*	m_pKeywords;
};

class STREAM_STATS : public STATS
{
public:
    STREAM_STATS(IUnknown*	    /*IN*/  pUnknown,
		 UINT32		    /*IN*/  ulRegistryID);
    virtual ~STREAM_STATS();

    virtual void	Reset(void);

    //Assignment operator. Copies only the values, will not change
    //the parent, registry or anything else.
    STREAM_STATS& operator=(const STREAM_STATS& rhs );

    CStatisticEntry*	m_pRenderer;
    CStatisticEntry*	m_pMimeType;
};

HXBOOL SetIntIfNecessary(CStatisticEntry* pEntry, INT32 lValue);

#endif	// _STATINFO_H_
