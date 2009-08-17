#!/usr/bin/env python
# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: timestamp.py,v 1.2 2007/04/30 22:51:13 jfinnecy Exp $ 
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
"""Module to encapsulate time and date handling in Ribosome."""
import time

import datelib


class TimestampException( Exception ):
    def __init__( self , value ):
        self.value = value
        
    def __str__( self ):
        return repr( self.value )

class Timestamp:
    """class Timestamp
        
    Provides an abstract timestamp class, which a user can query for local or 
    utc time, in ticks or string format. Provides a small number of 
    convenience methods as well.
        
        getLocalTicks()   - returns the local time in seconds since epoch (ticks)
        getUTCTicks()     - returns the UTC time in ticks
        getLocalString()  - returns the local time as a string
        getUTCString()    - returns the UTC time as a string

        getLocalCVSTime() - returns local time as a string formatted for cvs
                            command lines
        getLocalDay()     - returns the integer date of the day of the month
        getUTCDay()       - utc version of above
        
    Timestamp objects also support basic add and subtract of seconds, as well 
    as boolean math comparisons between two timestamps.
    """
    
    def __init__( self , init = None , isUTC = False ):
        """__init__(i,u)
        
        Initializes object to optional value i, where i is either integer ticks
        or a date string of one of the following formats:
           YYYY-MM-DD HH:MM:SS
           YYYY/MM/DD HH:MM:SS
           YYYY-MM-DD
        
        If an initializer i is specified, it is assumed to be in localtime 
        unless optional u == True.
        
        If no initializer is specified, the object is initialized to current
        time.
        """
        ticks = None

        # Figure out which kind of init we got, integer/float (ticks),
        # string, or None at all (use current time).        
        if not init:
            ticks = int(time.time())
        elif ( type(init) == int ) or ( type(init) == float ):
            ticks = int(init)
        elif ( type(init) == str ):
            ticks = int( datelib.date_to_ticks( init ))
        
        if not ticks:
            raise TimestampException( "Invalid initialization attempt: init = %s" % init )
        
        if isUTC:
            self.__utcTicks = ticks
            # Figure out localtime equivalent. Are we in DST?
            if self.__isDST( ticks ):
                self.__localTicks = ticks - time.altzone
            else:
                self.__localTicks = ticks - time.timezone                
        else:
            self.__localTicks = ticks
            # Figure out UTC equivalent. Are we in DST?
            if self.__isDST( ticks ):
                self.__utcTicks = ticks + time.altzone
            else:
                self.__utcTicks = ticks + time.timezone
      
    def __cmp__( self , y ):
        if self.__utcTicks < y.getUTCTicks():
            return -1
        elif self.__utcTicks > y.getUTCTicks():
            return 1
        elif self.__utcTicks == y.getUTCTicks():
            return 0            
        
    def __add__( self , y ):
        if (type(y) == int) or (type(y) == float):
            return Timestamp( self.getUTCTicks() + int(y) , True )
        else:
            raise TimestampException( "Unsupported type in addition attempt: %s" % y )
            
    def __sub__( self , y ):
        if (type(y) == int) or (type(y) == float):
            return Timestamp( self.getUTCTicks() - int(y) , True )
        else:
            raise TimestampException( "Unsupported type in subtraction attempt: %s" % y )
        
    def __getDay( self , ticks ):
        """__getDay(t) --> integer
        
        Returns the integer correpsonding to the day of the month represented 
        by ticks t.
        """
        return datelib.get_day( ticks )
        
    def __getString( self , ticks , sep ):
        """__getString(t,s) --> string
        
        Return date corresponding to ticks (t) as a string of format:
            
            YYYY[sep]MM[sep]DD HH:MM:SS

        where [sep] == the separator (s).            
        """        
        if '-' == sep:
            return datelib.asctime( ticks )
        elif '/' == sep:
            return datelib.cvstime( ticks )
        else:
            raise TimestampException( 'Invalid separator value: %s' % sep )
            
    def __isDST( self , ticks ):
        """__isDST(t) --> boolean
        
        Boolean query: does ticks t fall within daylight savings time?
        """
        if time.localtime( ticks )[8]:
            return True
        else:
            return False
            
    def getLocalCVSTime( self ):
        """getLocalCVSTime() --> string
        
        Returns the local time in a format suitable for use in CVS command lines.
        """
        return datelib.cvstime( self.__localTicks )
        
    def getLocalDay( self ):
        """getLocalDay() --> integer
        
        Returns the day of the month of the local time.
        """
        return self.__getDay( self.__localTicks )
        
    def getUTCDay( self ):
        """getUTCDay() --> integer
        
        Returns the day of the month of the UTC time.
        """
        return self.__getDay( self.__utcTicks )
        
    def getLocalString( self , sep = '-' ):
        """getLocalString(s) --> string
        
        Returns the local date/time of the object as a string of format:
            
            YYYY[sep]MM[sep]DD HH:MM:SS

        where [sep] == the separator (s).            
        """        
        return self.__getString( self.__localTicks , sep )
        
    def getUTCString( self , sep = '-' ):
        """getUTCString(s) --> string
        
        Returns the UTC date/time as a string of format:
            
            YYYY[sep]MM[sep]DD HH:MM:SS

        where [sep] == the separator (s).            
        """        
        return self.__getString( self.__utcTicks , sep )
        
    def getLocalTicks( self ):
        """getLocalTicks() --> integer
        
        Returns the localtime ticks of the object.
        """
        return self.__localTicks       
        
    def getUTCTicks( self ):
        """getUTCTicks() --> integer
        
        Returns the UTC ticks of the object.
        """
        return self.__utcTicks
        

