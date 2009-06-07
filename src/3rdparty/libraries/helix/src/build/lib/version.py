# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: version.py,v 1.8 2007/04/30 22:51:13 jfinnecy Exp $ 
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
"""ADT to represent a version of the format x.y.z-string.

Classes:
    Version
    VersionException

The module also provides static methods for querying the Ribosome version:
    getMajorVersion()
    getMinorVersion()
    getPatchLevel()
    getVersionString()    
"""
import os
import sys
import re

import log
log.debug( 'Imported: $Id: version.py,v 1.8 2007/04/30 22:51:13 jfinnecy Exp $' )

class VersionException( Exception ):
    def __init__( self , value ):
        self.value = value
        
    def __str__( self ):
        return repr( self.value )

        
class Version:
    """class Version:
        
    Provides print and compare methods for versions stored as x.y.z with an
    optional -string addendum.    
    
    Methods:
        getAddendum()
        getMajor()
        getMinor()
        getPatch()
        getVersionNoAddendum()
        
        The __repr__ and __cmp__ built-ins have been overridden to provide 
        native comparison between Version objects, and simple string 
        substitution.
    """
    
    def __init__( self , version ):
        """__init__(v)
        
        Initialize the version v with format x.y.z with optional '-string'.
        Must provide at least one number. Anything after the first dash is
        a text addendum.
        
        Providing more than 3 numbers, all 0's, or a text version are illegal
        formats and will raise VersionException. 
        
        Trailing text after the last version number will be incorporated into
        the text addendum.         
        
        Legal examples:
            2
            2.3
            2.0.1-test
            0.0.1
            0.1
            3.2-devel
            3.2.2-devel-test_version-6.2
            2.4.4c1 (--> "2.4.4-c1" )            
            
        Illegal examples:
            3.2.1.5
            2.4c1.4
            0.0.0
            0 (same as 0.0.0)
            One            
        """
        # Parse out the tokens we want.
        tokens = self.__parseString( version )
        self.__major    = tokens['major']
        self.__minor    = tokens['minor']
        self.__patch    = tokens['patch']
        self.__addendum = tokens.get('addendum')
        
    def __repr__( self ):
        """String representation of the object data for easy printing."""
        version = self.getVersionNoAddendum()    
        if self.__addendum:
            version += '-%s' % self.__addendum       
        return version
        
    def __cmp__( self , y ):
        """Override the compare operators. Compares self (x) to object y:        
        Per Python: return negative if x<y, zero if x==y, positive if x>y.
        """          
        # Readability aids.
        x            = self
        xIsGreater   = 1
        bothAreEqual = 0
        yIsGreater   = -1
        
        if x.getMajor() > y.getMajor():
            return xIsGreater            
        elif x.getMajor() < y.getMajor():
            return yIsGreater            
        else: # Majors equal.
            if x.getMinor() > y.getMinor():
                return xIsGreater
            elif x.getMinor() < y.getMinor():
                return yIsGreater
            else: # Minors equal.
                if x.getPatch() > y.getPatch():
                    return xIsGreater
                elif x.getPatch() < y.getPatch():
                    return yIsGreater
                else: # All are equal.
                    return bothAreEqual
                    
        # Big error if we get here.
        log.error( "Couldn't properly compare version objects: %s vs %s" % \
                    ( x , y ) )

    def getAddendum( self ):
        """getAddendum() --> string
        
        Returns the addendum.
        """
        return self.__addendum 

    def getMajor( self ):
        """getMajor() --> int
        
        Returns the major portion.
        """
        return self.__major
        
    def getMinor( self ):
        """getMinor() --> int
        
        Returns the minor portion.
        """
        return self.__minor
        
    def getPatch( self ):
        """getPatch() --> int
        
        Returns the patch level.
        """
        return self.__patch
        
    def getVersionNoAddendum( self ):
        """getVersionNoAddendum() --> string
        
        Returns the version as a string without the addendum. The patch level
        is only printed if it is > 0.
        """
        version = '%s.%s' % ( self.__major , self.__minor )
        if self.__patch > 0:
            version += '.%s' % self.__patch
        return version        
    
    #
    # Private methods.
    #
    def __parseString( self , string ):
        """__parseString(s) --> hash
        
        Parses the string s into the proper components of the version. 
        Enforces all required formats here.
        """
        results = {}        

        # Take only the first word.
        word = string.split(' ')[0]
        
        # Split the word on '-'.
        items  = word.split('-' , 1)                
        
        # Now split the first item on '.' to get the numbers.
        tokens  = items[0].split('.')
        
        # Check for too many or too few numbers.
        if len(tokens) > 3:
            raise VersionException( 'Too many version numbers supplied: %s' % \
                                    len(tokens) )   
                                    
        if len(tokens) == 0:
            raise VersionException( 'Must supply at least a major version.' )

        # Parse the various tokens. Set defaults to 0.
        res = [0,0,0]
        for count in range(0 , len(tokens)):
            res[count] , add = self.__parseToken( tokens[count] )
            if add:
                if count != (len(tokens) - 1):
                    # We have an addendum, but there are more version numbers
                    # on the way.
                    raise VersionException( 'Improperly formatted version string: %s' % \
                                            string )
                else:
                    results['addendum'] = add
        
        # If there was a '-' in the original word, everything after the '-' 
        # is the addendum.
        if len(items) > 1:
            # If we already found an addendum from parsing the token, we will
            # add the new addendum as a second -item.
            if results.get('addendum'):
                results['addendum'] += '-%s' % items[1]
            else:               
                results['addendum'] = items[1]

        # Now, assign the results to the hash
        results['major'] = res[0]
        results['minor'] = res[1]
        results['patch'] = res[2]
            
        # Check for a 0.0.0 version (illegal).
        if results['major'] == 0 and \
           results['minor'] == 0 and \
           results['patch'] == 0:
               raise VersionException( 'Must have a version > 0.0.0' )
               
        return results
        
        
    def __parseToken( self , token ):
        """__parseToken(t) --> ( int , string )
        
        Parses the token t into an integer and optional string. Splits on the
        first non-digit character. Raises VersionException if the first
        character in the token is a non-digit, or token has zero length.
        """
        intPart = strPart = None
       
        if '' == token:
            # Token has zero length.
            raise VersionException( 'No token for parsing: %s' % token )
                    
        # Look for a non-digit in the token.        
        j = re.search( '\D' , token )
        # If there is one, split on that index.
        if j:
            index = j.start()
            if 0 == index:
                # First character is non-digit.
                raise VersionException( 'Version components must start with an integer.' )
            else:                
                intPart = int(token[:index])
                strPart = token[index:]
        # Otherwise, the entire token is digits.
        else:
            intPart = int(token)

        return intPart, strPart
                   
### 
# Static methods - assumes Ribosome version queries.
###

# Ribosome version handling.
__ribosome = Version( '2.4.6' )

def getMajorVersion():
    return __ribosome.getMajor()
    
def getMinorVersion():
    return __ribosome.getMinor()
    
def getPatchLevel():
    return __ribosome.getPatch()
    
def getVersionString():
    return __ribosome
    

# Python version handling.
__requiredPython = Version( '2.4.3' )

def enforcePythonVersion():
    systemPython = Version( sys.version )
    if systemPython < __requiredPython:
        error = 'Ribosome requires Python version %s' % __requiredPython
        msg = error + '\nVersion invoked is %s' % sys.version
        msg += '\nPath is %s' % os.environ.get('PATH')

# Disable version exception for Qtopia
#        log.error( msg )
#        raise VersionException( error )
