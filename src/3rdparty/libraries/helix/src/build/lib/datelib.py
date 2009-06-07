#!/usr/bin/env python
# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: datelib.py,v 1.4 2007/04/30 22:51:13 jfinnecy Exp $ 
#   
#  Copyright Notices: 
#   
#  Portions Copyright (c) 1995-2006 RealNetworks, Inc. All Rights Reserved. 
#   
#  Patent Notices: This file may contain technology protected by one or  
#  more of the patents listed at www.helixcommunity.org 
#   
#  1.   The contents of this file, and the files included with this file, 
#  are protected by copyright controlled by RealNetworks and its  
#  licensors, and made available by RealNetworks subject to the current  
#  version of the RealNetworks Public Source License (the "RPSL")  
#  available at  * http://www.helixcommunity.org/content/rpsl unless  
#  you have licensed the file under the current version of the  
#  RealNetworks Community Source License (the "RCSL") available at 
#  http://www.helixcommunity.org/content/rcsl, in which case the RCSL 
#  will apply.  You may also obtain the license terms directly from 
#  RealNetworks.  You may not use this file except in compliance with 
#  the RPSL or, if you have a valid RCSL with RealNetworks applicable 
#  to this file, the RCSL.  Please see the applicable RPSL or RCSL for 
#  the rights, obligations and limitations governing use of the 
#  contents of the file. 
#   
#  2.  Alternatively, the contents of this file may be used under the 
#  terms of the GNU General Public License Version 2 (the 
#  "GPL") in which case the provisions of the GPL are applicable 
#  instead of those above.  Please note that RealNetworks and its  
#  licensors disclaim any implied patent license under the GPL.   
#  If you wish to allow use of your version of this file only under  
#  the terms of the GPL, and not to allow others 
#  to use your version of this file under the terms of either the RPSL 
#  or RCSL, indicate your decision by deleting Paragraph 1 above 
#  and replace them with the notice and other provisions required by 
#  the GPL. If you do not delete Paragraph 1 above, a recipient may 
#  use your version of this file under the terms of any one of the 
#  RPSL, the RCSL or the GPL. 
#   
#  This file is part of the Helix DNA Technology.  RealNetworks is the 
#  developer of the Original Code and owns the copyrights in the 
#  portions it created.   Copying, including reproducing, storing,  
#  adapting or translating, any or all of this material other than  
#  pursuant to the license terms referred to above requires the prior  
#  written consent of RealNetworks and its licensors 
#   
#  This file, and the files included with this file, is distributed 
#  and made available by RealNetworks on an 'AS IS' basis, WITHOUT  
#  WARRANTY OF ANY KIND, EITHER EXPRESS OR IMPLIED, AND REALNETWORKS  
#  AND ITS LICENSORS HEREBY DISCLAIM  ALL SUCH WARRANTIES, INCLUDING  
#  WITHOUT LIMITATION, ANY WARRANTIES OF MERCHANTABILITY, FITNESS  
#  FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR NON-INFRINGEMENT. 
#   
#  Technology Compatibility Kit Test Suite(s) Location:  
#     http://www.helixcommunity.org/content/tck 
#   
# Contributor(s):
# 
# ***** END LICENSE BLOCK *****
# 
import re
import time

_re_rev_date1 = re.compile(
    "([0-9]{4})-([0-9][0-9])-([0-9][0-9]) "\
    "([0-9][0-9]):([0-9][0-9]):([0-9][0-9])")

_re_rev_date12 = re.compile(
    "([0-9]{4})/([0-9][0-9])/([0-9][0-9]) "\
    "([0-9][0-9]):([0-9][0-9]):([0-9][0-9])")

_re_rev_date2 = re.compile(
    "([0-9]{4})-([0-9][0-9])-([0-9][0-9])")

def date_to_ticks(date):
    """date_to_ticks(s) --> float
    
    Takes a string s representing a date/time in one of the following 3 
    formats and returns the localtime tick equivalent.    
        
    YYYY-MM-DD HH:MM:SS
    YYYY/MM/DD HH:MM:SS
    YYYY-MM-DD
    """
    date.strip()

    temp = None

    match = _re_rev_date1.match(date)
    if not match:
        match = _re_rev_date12.match(date)

    if match:
        temp = tuple(map(int, match.groups()))
    else:
        match = _re_rev_date2.match(date)
        if match:
            temp = tuple(map(int, match.groups())) + (0, 0, 0)
            
    if temp:
        dst=time.localtime(time.mktime(temp + (0,1,0)))[8]
        return time.mktime(temp + (0,1,dst))

    return None

def date_to_gmticks(date):
    """date_to_gmticks(s) --> float
    
    Takes a string s in one of 3 formats and returns the UTC tick equivalent.        
    See date_to_ticks() for valid formats.
    """
    t=date_to_ticks(date)
    if not t:
        return None
    
    # Determine if we are in DST.
    if time.localtime( t )[8]:
        tzOffset = time.altzone
    else:
        tzOffset = time.timezone
        
    return t + tzOffset

def asctime(ticks):
    """asctime(t) --> string
    
    Given ticks t, returns time as a string of format:
        YYYY-MM-DD HH:MM:SS
    """
    return time.strftime("%Y-%m-%d %H:%M:%S", time.localtime(ticks))

def cvstime(ticks):
    """cvstime(t) --> string
    
    Given ticks t, returns time as a string of format:
        YYYY/MM/DD HH:MM:SS
    """
    return time.strftime("%Y/%m/%d %H:%M:%S", time.localtime(ticks))

def cvs_server_time(ticks):
    """cvs_server_time(t) --> string
    
    Given ticks t, returns UTC time as a string of format:
        YYYY-MM-DD HH:MM:SS
    """
    return time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime(ticks))


def get_day(ticks):
    """get_day(t) --> int
    
    Given ticks t, returns day of the month.
    """
    (t, t, day, t, t, t, t, t, t) = time.localtime(ticks)
    return day


def asctime_from_seconds(sec):
    """asctime_from_seconds(s) --> string
    
    Given seconds s, return a string representation of hours, minutes and
    seconds: h:mm.ss
    
    FIXME: Do we really want mm:ss represented as mm.ss? 0:35.59 intuitively
    does not seem like 35 minutes, 59 seconds. Need to investigate callers.
    """
    seconds = sec % 60
    minutes = (sec / 60) % 60
    hours   = sec / 3600

    return "%d:%02d.%02d" % (hours, minutes, seconds)


def timestamp():
    """timestamp() --> string
    
    Returns the current timestamp formatted as a string:
        YYYY-MM-DD HH:MM:SS
    """
    t = time.localtime(time.time())
    return "%04d-%02d-%02d %02d:%02d:%02d" % t[:6]

