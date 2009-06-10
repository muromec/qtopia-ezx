/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: interval.cpp,v 1.8 2007/07/06 20:51:41 jfinnecy Exp $
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

#include "hlxclib/stdlib.h"
#include "interval.h"

/*
* Minimum average time between RTCP packets from this site (in
* seconds).  This time prevents the reports from `clumping' when
* sessions are small and the law of large numbers isn't helping
* to smooth out the traffic.  It also keeps the report interval
* from becoming ridiculously small during transient outages like
* a network partition.
*/
double const RTCP_MIN_TIME = 5.0;
/*
* Fraction of the RTCP bandwidth to be shared among active
* senders.  (This fraction was chosen so that in a typical
* session with one or two active senders, the computed report
* time would be roughly equal to the minimum report time so that
* we don't unnecessarily slow down receiver reports.) The
* receiver fraction must be 1 - the sender fraction.
*/
double const RTCP_SENDER_BW_FRACTION = 0.25;
double const RTCP_RCVR_BW_FRACTION = (1-RTCP_SENDER_BW_FRACTION);
/* To compensate for "unconditional reconsideration" converging to a
* value below the intended average.
*/
double const COMPENSATION = 2.71828 - 1.5;


double rtcp_interval(INT32 receivers,
                     INT32 senders,
                     double rs_byterate,
		     double rr_byterate,
                     HXBOOL we_sent,
                     double avg_rtcp_size,
                     HXBOOL initial,
		     double rtcp_min_time)
{
    double rtcp_byterate = rr_byterate + rs_byterate;
    double t;                   /* interval */
    INT32 n = receivers + senders;          /* no. of members for computation */

    /*
    * Very first call at application start-up uses half the min
    * delay for quicker notification while still allowing some time
    * before reporting for randomization and to learn about other
    * sources so the report interval will converge to the correct
    * interval more quickly.
    */
    if (initial) 
    {
	rtcp_min_time /= 2;
    }

    /*
    * If there were active senders, give them at least a minimum
    * share of the RTCP bandwidth.  Otherwise all participants share
    * the RTCP bandwidth equally.
    */
    if (senders > 0 ) 
    {
	if (we_sent) 
	{
	    rtcp_byterate = rs_byterate;
	    n = senders;
	} 
	else 
	{
	    rtcp_byterate = rr_byterate;
	    n -= senders;
	}
    }

    /*
    * The effective number of sites times the average packet size is
    * the total number of octets sent when each site sends a report.
    * Dividing this by the effective bandwidth gives the time
    * interval over which those packets must be sent in order to
    * meet the bandwidth target, with a minimum enforced.  In that
    * time interval we send one report so this time is also our
    * average time between reports.
    */
    t = avg_rtcp_size * n / rtcp_byterate;
    if (t < rtcp_min_time) 
    {
	t = rtcp_min_time;
    }	

    /*
    * To avoid traffic bursts from unintended synchronization with
    * other sites, we then pick our actual next report interval as a
    * random number uniformly distributed between 0.5*t and 1.5*t.
    */
    t = t * (((double)rand() / (double)RAND_MAX) + 0.5);
    t = t / COMPENSATION;

    return t;
}

