/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: rlstate.cpp,v 1.7 2005/03/14 19:36:39 bobclark Exp $
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

#include "hxtypes.h"
#include "hxassert.h"
#include "hxcom.h"
#include "ihxpckts.h"
#include "hxstrutl.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

#include "rlstate.h"

static const char WAITFORSWITCHOFF[] = "WaitForSwitchOff";
static const size_t WAITFORSWITCHOFFLEN = sizeof(WAITFORSWITCHOFF) - 1;

static const char ONDEPEND[] = "OnDepend";
static const size_t ONDEPENDLEN = sizeof(ONDEPEND) - 1;

static const char OFFDEPEND[] = "OffDepend";
static const size_t OFFDEPENDLEN = sizeof(OFFDEPEND) - 1;

#define NO_DEPEND_SET 0xFFFF

inline void SKIP_WHITE_SPACE(char* __x)
{
    while (*__x == ' ' || *__x == '\t' || *__x == '\n' || *__x == '\r')
    {
	__x++;
    }
}


/****************************************************************************
 *  Method:
 *    CASMRuleState::CASMRuleState
 *
 */
CASMRuleState::CASMRuleState(UINT16 nNumRules, char* pRuleBook, 
			     UINT16 usBookSize)
{
    m_nNumRules = nNumRules;
    m_bNeedSwitchOff = new HXBOOL[nNumRules];
    m_bSubscribePending = new HXBOOL[nNumRules];     
    m_bSubscribed = new HXBOOL[nNumRules];
    m_bUnsubscribePending = new HXBOOL[nNumRules];
    m_lastASMFlagsForThisRule = new UINT8[nNumRules];
    m_OnDepends = new UINT16*[nNumRules];
    m_OffDepends = new UINT16*[nNumRules];

    for (UINT16 i = 0; i < nNumRules; i++)
    {
	m_bNeedSwitchOff[i] = FALSE;
	m_bSubscribePending[i] = FALSE;
	m_bSubscribed[i] = FALSE;
	m_bUnsubscribePending[i] = FALSE;
	m_lastASMFlagsForThisRule[i] = 0;
	m_OnDepends[i] = NULL;
	m_OffDepends[i] = NULL;
    }

    ParseRuleBookForDirectives(m_nNumRules, pRuleBook, usBookSize,
	m_bNeedSwitchOff, m_OffDepends, m_OnDepends);
}

/****************************************************************************
 *  Method:
 *    CASMRuleState::~CASMRuleState
 *
 */
CASMRuleState::~CASMRuleState()
{
    HX_VECTOR_DELETE(m_bSubscribePending);
    HX_VECTOR_DELETE(m_bSubscribed);
    HX_VECTOR_DELETE(m_bUnsubscribePending);
    HX_VECTOR_DELETE(m_lastASMFlagsForThisRule);
    HX_VECTOR_DELETE(m_bNeedSwitchOff);

    for (UINT16 i = 0; i < m_nNumRules; i++)
    {
	HX_VECTOR_DELETE(m_OffDepends[i]);
	HX_VECTOR_DELETE(m_OnDepends[i]);
    }

    HX_VECTOR_DELETE(m_OffDepends);
    HX_VECTOR_DELETE(m_OnDepends);

    m_nNumRules = 0;
}

/****************************************************************************
 *  Method:
 *    CASMRuleState::ParseRuleBookDirectives
 *
 */
void 
CASMRuleState::ParseRuleBookForDirectives(UINT16 num_rules,
					  char* pRuleBook,
					  UINT16 usBookSize,
					  HXBOOL* pNeedSwitchOff,
					  UINT16** pOffDepends, 
					  UINT16** pOnDepends)
{
    char* pOffset = pRuleBook;
    UINT16 idxCurrentRule = 0;

    while(*pOffset != '\0' && ((UINT16)(pOffset - pRuleBook) < usBookSize))
    {
	while(*pOffset != ';')
	{
	    if (*pOffset == 'w' || *pOffset == 'W')
	    {
		// check to see if we have a wait directive
		if (strncasecmp(WAITFORSWITCHOFF, pOffset, 
			WAITFORSWITCHOFFLEN) == 0)
		{
		    pOffset += WAITFORSWITCHOFFLEN;
		    
		    SKIP_WHITE_SPACE(pOffset);

		    // should have an = sign
		    HX_ASSERT(*pOffset == '=');
		    pOffset ++;

		    SKIP_WHITE_SPACE(pOffset);

		    pNeedSwitchOff[idxCurrentRule] = 
			(*pOffset == 't' || *pOffset == 'T');
		}
	    }
	    if (*pOffset == 'o' || *pOffset == 'O')
	    {
		HXBOOL bDependsList = FALSE;
		UINT16* pDependsList = NULL;
		
		if (strncasecmp(ONDEPEND, pOffset, 
			ONDEPENDLEN) == 0)
		{
		    bDependsList = TRUE;
		    pDependsList = new UINT16[num_rules];
		    pOnDepends[idxCurrentRule] = pDependsList;
		    memset(pDependsList, 0xFF, sizeof(UINT16) * num_rules);
		    pOffset += ONDEPENDLEN;
		}
		else if (strncasecmp(OFFDEPEND, pOffset, 
			OFFDEPENDLEN) == 0)
		{
		    bDependsList = TRUE;
		    pDependsList = new UINT16[num_rules];
		    pOffDepends[idxCurrentRule] = pDependsList;
		    memset(pDependsList, 0xFF, sizeof(UINT16) * num_rules);
		    pOffset += OFFDEPENDLEN;
		}

		if (bDependsList)
		{

		    SKIP_WHITE_SPACE(pOffset);

		    HX_ASSERT(*pOffset == '=');
		    pOffset++;

		    SKIP_WHITE_SPACE(pOffset);

		    pOffset = ParseDependsList(pOffset, pDependsList);
		}
	    }
	    pOffset++;
	}
	idxCurrentRule++;
	pOffset++;
    }
}

/****************************************************************************
 *  Method:
 *    CASMRuleState::ParseRuleBookDirectives
 *
 */
char* 
CASMRuleState::ParseDependsList(char* pDependsText, UINT16* pDependsList)
{
    char* pOffset = pDependsText;
    UINT16 idxCurrentDepends = 0;
    UINT16 nCurrentDepends = 0;
    
    SKIP_WHITE_SPACE(pOffset);

    // depends directive list is in "'s
    if (*pOffset != '\"')
    {
	HX_ASSERT(*pOffset == '\"');
    }
    else
    {
	// skip the first "
	pOffset++;

	// parse string till we find the last "
	while (*pOffset != '\"')
	{
            SKIP_WHITE_SPACE(pOffset);

	    // should be a list of numbers and we are at the beginning of one
	    while(*pOffset >= '0' && *pOffset <= '9')
	    {
		nCurrentDepends =  
		    (nCurrentDepends * 10) + *pOffset - '0';
		pOffset++;
	    }

	    pDependsList[idxCurrentDepends] = nCurrentDepends;

	    SKIP_WHITE_SPACE(pOffset);

	    // depend numbers are seperated by commas or this could be
	    // the last one
	    if (*pOffset == ',')
	    {
		pOffset++;
	    }
	    else
	    {
		// if it wasn't a comma, it better be the end of the depends
		// list
		HX_ASSERT(*pOffset == '\"');
	    }
	}
    }

    return pOffset;
}

/****************************************************************************
 *  Method:
 *    CASMRuleState::StartSubscribePending
 *
 */
void 
CASMRuleState::StartSubscribePending(UINT16 usRuleNum)
{
    HX_ASSERT(usRuleNum < m_nNumRules);

//printf("%lu\t\tSubscribe Pending:\t%u\n", this, usRuleNum);
    if (m_bUnsubscribePending[usRuleNum])
    {
//printf("%lu\t\tImediate Subscribe (was pending Unsubscribe):\t%u\n", this, usRuleNum);
	m_bUnsubscribePending[usRuleNum] = FALSE;
	m_bSubscribed[usRuleNum] = TRUE;
    }
    else
    {
        m_bSubscribePending[usRuleNum] = TRUE;
    }
}

/****************************************************************************
 *  Method:
 *    CASMRuleState::CanSubscribeNow
 *
 */
HXBOOL
CASMRuleState::CanSubscribeNow(UINT16 usRuleNum)
{
    // check for OnDepen dependent rules for this rule, if they are all
    // subscribed then this rule can subscribe
    UINT16 idxDepend;
    HXBOOL bAllDependsSubscribed = TRUE;

    if (m_OnDepends[usRuleNum] != NULL)
    {
	// once we find one depend pending we can stop looking
	for (idxDepend = 0; idxDepend < m_nNumRules && 
		m_OnDepends[usRuleNum][idxDepend] != NO_DEPEND_SET && 
		bAllDependsSubscribed; idxDepend++)
	{
	    // if the dependent rule is not subscribed then we have a
	    // dependPending
	    bAllDependsSubscribed = 
		m_bSubscribed[m_OnDepends[usRuleNum][idxDepend]];
	}
    }

    // we can subscribe as long as all dependent rules are subscribed
    return bAllDependsSubscribed;
}

/****************************************************************************
 *  Method:
 *    CASMRuleState::CompleteSubscribe
 *
 */
void
CASMRuleState::CompleteSubscribe(UINT16 usRuleNum)
{
    HX_ASSERT(usRuleNum < m_nNumRules);
//printf("%lu\t\tSubscribe Complete:\t%u\r\n", this, usRuleNum);
    m_bSubscribePending[usRuleNum] = FALSE;
    m_bSubscribed[usRuleNum] = TRUE;
}

/****************************************************************************
 *  Method:
 *    CASMRuleState::StartUnsubscribePending
 *
 */
void
CASMRuleState::StartUnsubscribePending(UINT16 usRuleNum)
{
    HX_ASSERT(usRuleNum < m_nNumRules);
//printf("%lu\t\tUnsubscribe Pending:\t%u\r\n", this, usRuleNum);
    if (m_bSubscribed[usRuleNum])
    {
	m_bUnsubscribePending[usRuleNum] = TRUE;
    }
    else
    {
	/*
	 * when a client subscribes to a rule and then decides to subscribe to a
	 * mutually exclusive rule (e.g. 8 and then 6), the subscribe to the second
	 * rule calls ASMRuleState::CancelStreamSwitch which clears all pending
	 * subscribes (i.e. the one to 8) and then starts a subscribe pending to 6.
	 * Later the client comes along and decides to explicitly unsubscribe to 8
	 * which causes an assertion failure in the asm rule state.	 
	 */
//	HX_ASSERT(m_bSubscribePending[usRuleNum]);
	/*
	 *  This should be fine since a new subscription state must have been established.
	 */
	 
	// we can cancel this pending subscribe if we are unsubscribing
	m_bSubscribePending[usRuleNum] = FALSE;
    }
}

/****************************************************************************
 *  Method:
 *    CASMRuleState::CanUnsubscribeNow
 *
 */
HXBOOL
CASMRuleState::CanUnsubscribeNow(UINT16 usRuleNum)
{
    // check for OffDepen dependent rules for this rule, if any are 
    // subscribed then this rule can't unsubscribe
    UINT16 idxDepend;
    HXBOOL bAllDependsUnsubscribed = TRUE;

    if (m_OffDepends[usRuleNum] != NULL)
    {
	// once we find one depend pending we can stop looking
	for (idxDepend = 0; idxDepend < m_nNumRules && 
	      m_OffDepends[usRuleNum][idxDepend] != NO_DEPEND_SET && 
	      bAllDependsUnsubscribed; idxDepend++)
	{
	    // if the dependent rule is still subscribed then we have a
	    // dependent rule that's not unsubscribed
	    bAllDependsUnsubscribed = 
		!m_bSubscribed[m_OffDepends[usRuleNum][idxDepend]];
	}
    }

    // we can unsubscribe if there aren't any dependent rules subscribed
    return bAllDependsUnsubscribed;
}

/****************************************************************************
 *  Method:
 *    CASMRuleState::CompleteUnsubscribe
 *
 */
void
CASMRuleState::CompleteUnsubscribe(UINT16 usRuleNum)
{
    HX_ASSERT(usRuleNum < m_nNumRules);
//printf("%lu\t\tUnsubscribe Complete:\t%u\r\n", this, usRuleNum);
    m_bUnsubscribePending[usRuleNum] = FALSE;
    m_bSubscribed[usRuleNum] = FALSE;
}

/****************************************************************************
 *  Method:
 *    CASMRuleState::CanSwitchStreamsNow
 *
 */
HXBOOL
CASMRuleState::CanSwitchStreamsNow()
{
    UINT16 i = 0;

    // if there are no unsubscribes pending && there are no subscribed
    // rules
    while (i < m_nNumRules && !m_bUnsubscribePending[i] && 
	!m_bSubscribed[i])
    {
	i++;
    }

    return (i == m_nNumRules);
}

/****************************************************************************
 *  Method:
 *    CASMRuleState::CompleteStreamSwitch
 *
 */
void
CASMRuleState::CompleteStreamSwitch()
{
//printf("%lu\t\t--->Compete Stream Switch\r\n", this);

    // mark all of the pending rules subscribed to
    for (UINT16 usRuleNumber = 0; usRuleNumber < m_nNumRules;
	usRuleNumber++)
    {
	if (IsSubscribePending(usRuleNumber))
	{
	    CompleteSubscribe(usRuleNumber);
	}
    }
//printf("%lu\t\t<---Stream Switch Completed\r\n", this);
}

/****************************************************************************
 *  Method:
 *    CASMRuleState::CancelStreamSwitch
 *
 */
void
CASMRuleState::CancelStreamSwitch()
{
//printf("%lu\t\tCancel Stream Switch\r\n", this);

    // mark all of the pending rules subscribed to
    for (UINT16 usRuleNumber = 0; usRuleNumber < m_nNumRules;
	usRuleNumber++)
    {
	m_bSubscribePending[usRuleNumber] = FALSE;
    }
}

/****************************************************************************
 *  Method:
 *    CASMRuleState::CompleteAllUnsubscribes
 *
 */
void
CASMRuleState::CompleteAllUnsubscribes()
{
//printf("%lu\t\t--->Compete Unsubscribe All Rules\r\n", this);
    for (UINT16 usRuleNumber = 0; usRuleNumber < m_nNumRules;
	usRuleNumber++)
    {
	if (IsUnsubscribePending(usRuleNumber))
	{
	    CompleteUnsubscribe(usRuleNumber);
	}
    }
//printf("%lu\t\t<---All Rules Unsubscribe Complete\r\n", this);
}


/****************************************************************************
 *  Method:
 *    CASMRuleState::AnyPendingUnsubscribes
 *
 */
HXBOOL 
CASMRuleState::AnyPendingUnsubscribes()
{
    UINT16 usRuleNumber;
    for (usRuleNumber = 0; usRuleNumber < m_nNumRules;
	usRuleNumber++)
    {
	if (IsUnsubscribePending(usRuleNumber))
	{
	    break;
	}
    }

    return usRuleNumber < m_nNumRules;
}


/****************************************************************************
 *  Method:
 *    CASMRuleState::GetNextPendingUnsubscribe
 *
 */
UINT16 
CASMRuleState::GetNextPendingUnsubscribe()
{
    UINT16 usRuleNumber;
    for (usRuleNumber = 0; usRuleNumber < m_nNumRules;
	usRuleNumber++)
    {
	if (IsUnsubscribePending(usRuleNumber))
	{
	    break;
	}
    }

    HX_ASSERT(usRuleNumber < m_nNumRules);
    return usRuleNumber;
}
