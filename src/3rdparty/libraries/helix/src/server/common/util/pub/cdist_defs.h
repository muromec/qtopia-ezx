/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: cdist_defs.h,v 1.2 2003/01/23 23:42:51 damonlan Exp $ 
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

#ifndef _CDIST_DEFS_H_
#define _CDIST_DEFS_H_

/*
 * Tracemask debug levels
 */
#define CDIST_D_ERROR		0x00000001	/* error condition */
#define CDIST_D_INFO		0x00000002	/* general informative messages */
#define CDIST_D_ENTRY		0x00000004	/* function entry/exit */

#define CDIST_D_RULE		0x00000010	/* Rule evaluation logic */
#define CDIST_D_STATS		0x00000020	/* bw stats to stdout */

/*
 * Defaults
 */
#define CDIST_DEFAULT_RTSP_IMPORT 1


/*
 *  Registry shme.  
 */
#define CDIST_REGISTRY_ROOT		"config.ContentSubscription"
#define CDIST_REGISTRY_SUB_ENABLED	CDIST_REGISTRY_ROOT ".Enabled"
#define CDIST_REGISTRY_DEBUG_LEVEL	CDIST_REGISTRY_ROOT ".DebugLevel"
#define CDIST_REGISTRY_PUBLISHER_LIST	CDIST_REGISTRY_ROOT ".Publishers"
#define CDIST_REGISTRY_RULE_LIST	CDIST_REGISTRY_ROOT ".Rules"
#define CDIST_REGISTRY_RTSP_IMPORT	CDIST_REGISTRY_ROOT ".RTSPImport"


#define CDIST_REGISTRY_COMMIT_RULE_CHANGE \
				"server.ContentSubscription.RuleChangeCommit"
#define CDIST_REGISTRY_FILE_PULLDOWN \
				"server.ContentSubscription.FilePullCommand"
#define CDIST_REGISTRY_FILE_PULLDOWN_HOLDER \
				CDIST_REGISTRY_FILE_PULLDOWN ".Placeholder"


#define CDIST_REGISTRY_STATISTICS_AGGREGATE_PERIOD \
				CDIST_REGISTRY_ROOT ".StatisticAggregationPeriodSec"

#define CDIST_REGISTRY_STATS_ROOT	"server.ContentDistribution.Statistics"
#define CDIST_REGISTRY_STATISTICS	CDIST_REGISTRY_STATS_ROOT ".All"

#define CDIST_REGISTRY_STATS_MEI_CONN_TOTAL \
			CDIST_REGISTRY_STATS_ROOT ".MEIConnectionsTotal"
#define CDIST_REGISTRY_STATS_MEI_CONN_ACTIVE \
			CDIST_REGISTRY_STATS_ROOT ".MEIConnectionsActive"
#define CDIST_REGISTRY_STATS_MEI_CONN_RTSP \
			CDIST_REGISTRY_STATS_ROOT ".MEIConnectionsRTSP"

#define CDIST_REGISTRY_STATS_MII_CONN_TOTAL \
			CDIST_REGISTRY_STATS_ROOT ".MIIConnectionsTotal"
#define CDIST_REGISTRY_STATS_MII_CONN_ACTIVE \
			CDIST_REGISTRY_STATS_ROOT ".MIIConnectionsActive"
#define CDIST_REGISTRY_STATS_MII_CONN_RTSP \
			CDIST_REGISTRY_STATS_ROOT ".MIIConnectionsRTSP"

#define CDIST_REGISTRY_STATS_EXPORT_BYTES \
			CDIST_REGISTRY_STATS_ROOT ".ExportBytes"
#define CDIST_REGISTRY_STATS_EXPORT_BPS \
			CDIST_REGISTRY_STATS_ROOT ".ExportBPS"
#define CDIST_REGISTRY_STATS_EXPORT_TOTAL_BYTES \
			CDIST_REGISTRY_STATS_ROOT ".ExportTotalBytes"

#define CDIST_REGISTRY_STATS_LOCAL_READ_BYTES \
			CDIST_REGISTRY_STATS_ROOT ".LocalReadBytes"
#define CDIST_REGISTRY_STATS_LOCAL_READ_BPS \
			CDIST_REGISTRY_STATS_ROOT ".LocalReadBPS"
#define CDIST_REGISTRY_STATS_LOCAL_READ_TOTAL_BYTES \
			CDIST_REGISTRY_STATS_ROOT ".LocalReadTotalBytes"
#define CDIST_REGISTRY_STATS_LOCAL_READ_CURRENT_PERCENT \
			CDIST_REGISTRY_STATS_ROOT ".LocalReadCurrentPercent"
#define CDIST_REGISTRY_STATS_LOCAL_READ_AVERAGE_PERCENT \
			CDIST_REGISTRY_STATS_ROOT ".LocalReadAveragePercent"

#define CDIST_REGISTRY_STATS_FASTFILE_READ_BYTES \
			CDIST_REGISTRY_STATS_ROOT ".FastFileReadBytes"
#define CDIST_REGISTRY_STATS_FASTFILE_READ_BPS \
			CDIST_REGISTRY_STATS_ROOT ".FastFileReadBPS"
#define CDIST_REGISTRY_STATS_FASTFILE_READ_TOTAL_BYTES \
			CDIST_REGISTRY_STATS_ROOT ".FastFileReadTotalBytes"
#define CDIST_REGISTRY_STATS_FASTFILE_READ_CURRENT_PERCENT \
			CDIST_REGISTRY_STATS_ROOT ".FastFileReadCurrentPercent"
#define CDIST_REGISTRY_STATS_FASTFILE_READ_AVERAGE_PERCENT \
			CDIST_REGISTRY_STATS_ROOT ".FastFileReadAveragePercent"

#define CDIST_REGISTRY_STATS_CACHE_READ_BYTES \
			CDIST_REGISTRY_STATS_ROOT ".CacheReadBytes"
#define CDIST_REGISTRY_STATS_CACHE_READ_BPS \
			CDIST_REGISTRY_STATS_ROOT ".CacheReadBPS"
#define CDIST_REGISTRY_STATS_CACHE_READ_TOTAL_BYTES \
			CDIST_REGISTRY_STATS_ROOT ".CacheReadTotalBytes"
#define CDIST_REGISTRY_STATS_CACHE_READ_CURRENT_PERCENT \
			CDIST_REGISTRY_STATS_ROOT ".CacheReadCurrentPercent"
#define CDIST_REGISTRY_STATS_CACHE_READ_AVERAGE_PERCENT \
			CDIST_REGISTRY_STATS_ROOT ".CacheReadAveragePercent"

#define CDIST_REGISTRY_STATS_IMPORT_BYTES \
			CDIST_REGISTRY_STATS_ROOT ".ImportBytes"
#define CDIST_REGISTRY_STATS_IMPORT_BPS \
			CDIST_REGISTRY_STATS_ROOT ".ImportBPS"
#define CDIST_REGISTRY_STATS_IMPORT_TOTAL_BYTES \
			CDIST_REGISTRY_STATS_ROOT ".ImportTotalBytes"
#define CDIST_REGISTRY_STATS_IMPORT_CURRENT_PERCENT \
			CDIST_REGISTRY_STATS_ROOT ".ImportCurrentPercent"
#define CDIST_REGISTRY_STATS_IMPORT_AVERAGE_PERCENT \
			CDIST_REGISTRY_STATS_ROOT ".ImportAveragePercent"

/*
 * structs
 */
/*
 * CDistMIIStatistics: byte transfer rates, server connections
 * See cdistpln for aggregation.  See server/fsmanager and miiplin for
 * statistics gathering.
 */
class CDistMIIStatistics
{
public:
    UINT16		m_usSize;
    UINT8		m_ubVersion;
    UINT8		m_ubEnabled;

    UINT32		m_ulMEIConnectionsTotal;
    UINT32		m_ulMEIConnectionsActive;
    UINT32		m_ulMEIConnectionsRTSP;

    UINT32		m_ulMIIConnectionsTotal;
    UINT32		m_ulMIIConnectionsActive;
    UINT32		m_ulMIIConnectionsRTSP;

    UINT32		m_ulExportBytes;
    UINT32		m_ulExportBPS;
    UINT64		m_ullExportTotalBytes;

    UINT32		m_ulLocalReadBytes;
    UINT32		m_ulLocalReadBPS;
    UINT64		m_ullLocalReadTotalBytes;
    UINT32		m_ulLocalReadCurrentPercent;
    UINT32		m_ulLocalReadAveragePercent;

    UINT32		m_ulFastFileReadBytes;
    UINT32		m_ulFastFileReadBPS;
    UINT64		m_ullFastFileReadTotalBytes;
    UINT32		m_ulFastFileReadCurrentPercent;
    UINT32		m_ulFastFileReadAveragePercent;

    UINT32		m_ulCacheReadBytes;
    UINT32		m_ulCacheReadBPS;
    UINT64		m_ullCacheReadTotalBytes;
    UINT32		m_ulCacheReadCurrentPercent;
    UINT32		m_ulCacheReadAveragePercent;

    UINT32		m_ulImportBytes;
    UINT32		m_ulImportBPS;
    UINT64		m_ullImportTotalBytes;
    UINT32		m_ulImportCurrentPercent;
    UINT32		m_ulImportAveragePercent;
};

#endif  // _CDIST_DEFS_H_
