/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: globals.h,v 1.9 2007/08/20 19:35:14 dcollins Exp $
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

extern BOOL     g_bSlightAcceleration;
extern BOOL     g_bNoCrashAvoidance;
extern BOOL     g_bCrashAvoidancePrint;
extern BOOL     g_bPrintProcessPIDs;
extern BOOL     g_bNoClientValidation;
extern BOOL     g_bReportRestart;
extern BOOL     g_bFastMalloc;
extern BOOL     g_bFastMallocAll;
extern UINT32   g_ulReportServerStats;
extern BOOL     g_bSkipCPUTest;
extern BOOL     g_bAllowCoreDump;
extern BOOL     g_bShowDebugErrorMessages;
extern BOOL     g_bDisableHeartbeat;
extern BOOL     g_bDisablePacketAggregation;
extern BOOL     g_bIgnoreEtcHosts;

extern UINT32   g_ulSizemmap;

extern UINT32*  g_pBytesServed;
extern UINT32*  g_pPPS;
extern UINT32*  g_pLiveIncomingPPS;
extern UINT32*  g_pOverloads;
extern UINT32*  g_pBehind;
extern UINT32*  g_pNoBufs;
extern UINT32*  g_pOtherUDPErrs;
extern UINT32*  g_pMainLoops;
extern UINT32*  g_pForcedSelects;
extern UINT32*  g_pAggregateRequestedBitRate;
extern UINT32*  g_pConcurrentOps;
extern UINT32*  g_pConcurrentMemOps;
extern UINT32*  g_pSchedulerElems;
extern UINT32*  g_pISchedulerElems;
extern UINT32*  g_pTotalNetReaders;
extern UINT32*  g_pTotalNetWriters;
extern UINT32*  g_pMutexNetReaders;
extern UINT32*  g_pMutexNetWriters;
extern UINT32*  g_pIdlePPMs;
extern UINT32*  g_pWouldBlockCount;
extern UINT32*  g_pSocketAcceptCount;
extern UINT32*  g_pAggregatablePlayers;
extern UINT32*  g_pFileObjs;
extern UINT32*  g_pCPUCount;
extern UINT32*  g_pNumStreamers;
extern UINT32*  g_pResends;
extern UINT32*  g_pAggreg;
extern UINT32*  g_pIsForcedSelect;
extern Timeval* g_pNow;
extern BOOL*    g_bITimerAvailable;
extern UINT32*  g_pNumCrashAvoids;

extern UINT32*  g_pAggregateTo;
extern UINT32*  g_pAggregateHighest;
extern UINT32*  g_pAggregateMaxBPS;

extern BOOL     g_bReconfigServer;
extern BOOL     g_bResetLeaks;
extern BOOL     g_bCheckLeaks;

extern BOOL IAmController();

extern UINT32*  g_pStreamerCount;

// Broadcast Statistics
extern double* g_pBroadcastDistBytes;
extern UINT32* g_pBroadcastDistPackets;
extern double* g_pBroadcastRecvBytes;
extern UINT32* g_pBroadcastRecvPackets;
extern UINT32* g_pBroadcastPacketsDropped;
extern UINT32* g_pBroadcastPPMOverflows;

