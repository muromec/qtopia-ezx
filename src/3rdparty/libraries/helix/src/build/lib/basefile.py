# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: basefile.py,v 1.2 2006/06/19 23:11:27 jfinnecy Exp $ 
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
"""Module to handle processing related to dll output files.

This module is thread-safe.

Classes:
    Basefile           - interface to shared data source with recorded dll data
    BasefileException
"""

import os
import thread

import log
log.debug( 'Imported: $Id: basefile.py,v 1.2 2006/06/19 23:11:27 jfinnecy Exp $' )
import utils
import filelock


class BasefileException( Exception ):
    def __init__( self , value ):
        self.value = value
        
    def __str__( self ):
        return repr( self.value )        

        
class Basefile:
    """Provides an interface for storing and querying dll base addresses and
    sizes.
    
    Methods raise BasefileException if preconditions (specified in the method
    descriptions) are not satisfied.
    
    Methods:
        getBaseAddressOfFile()
        getNextBaseAddress()
        getSizeOfFile()
        isDLLRecorded()
        lock()
        readData()
        recordDLL()
        unlock()
    """    
    def __init__( self , filename , base ):
        """__init__(f,b)
        
        Initialize the basefile object using file f as the data store and with
        a default base of b.
        """
        self.__filename    = filename        
        self.__defaultBase = base
        self.__data        = []
        self.__dataCached  = 0        
        self.__lock        = filelock.FileLock( self.__filename , expiration=300 )
        self.__threadLock  = thread.allocate_lock()

    def getBaseAddressOfFile( self , filename ):
        """getBaseAddressOfFile(f) --> integer
        
        Returns the base address of the DLL f as recorded in the basefile.
        
        Preconditions:
            this thread has a lock on the basefile
            data has been read in
            f is in the data source
        """
        log.trace( 'entry' , [ filename ] )
        
        self.__threadLock.acquire()
        try:
            self.__checkLock()
            self.__checkDataCached()      
            self.__checkFile( filename )
            
            address = self.__getData( filename , 'base' )
        finally:            
            self.__threadLock.release()
            
        log.trace( 'exit' , [ "0x%x" % address ] )
        return address
        
    def getNextBaseAddress( self , dll ):
        """getNextBaseAddress(d) --> integer
        
        Returns the next base address to use for dll d by processing the data 
        in the basefile, or returns the default base if no basefile exists.
        This will ignore the existing values for d if it is already present
        (it thus assumes this is a rebase and the original values get replaced)
        
        Preconditions:
            this thread has a lock on the basefile
            data has been read in
        """
        log.trace( 'entry' , [ dll ] )
        
        self.__threadLock.acquire()
        try:
            self.__checkLock()
            self.__checkDataCached()
            
            result = self.__defaultBase    
            if len( self.__data ):
                highestDLL = self.__getHighestDLL( dll )
                if highestDLL:                    
                    base = self.__getData( highestDLL , 'base' )
                    size = self.__getData( highestDLL , 'size' )
                    if ( base + size ) > result:
                        result = base + size
        finally:
            self.__threadLock.release()
            
        log.trace( 'exit' , [ "0x%x" % result ] )
        return result
                
    def getSizeOfFile( self , filename ):
        """getSizeOfFile(f) --> integer
        
        Returns the size of the DLL f as recorded in the basefile. Raises

        Preconditions:
            this thread has a lock on the basefile
            data has been read in
            f is in the data source
        """
        log.trace( 'entry' , [ filename ] )
        
        self.__threadLock.acquire()
        try:
            self.__checkLock()
            self.__checkDataCached()
            self.__checkFile( filename )
            
            size = self.__getData( filename , 'size' )
        finally:            
            self.__threadLock.release()
            
        log.trace( 'exit' , [ "0x%x" % size ] )
        return size
        
    def isDLLRecorded( self , dll ):
        """isDLLRecorded(d) --> boolean
        
        Returns True if DLL d has data recorded in basefile.
        
        Preconditions:
            this process has a lock on the basefile
            data has been read in
        """
        log.trace( 'entry' , [ dll ] )
              
        self.__threadLock.acquire()
        try:
            self.__checkLock()
            self.__checkDataCached()
            found = self.__isDLLRecorded( dll )             
        finally:                
            self.__threadLock.release()
            
        log.trace( 'exit' , [ found ] )
        return found
         
    def lock( self ):
        """lock()
        
        Gets an exclusive lock on the data source. Use this before calling any
        other methods to insure data integrity (required).
        """
        self.__lock.getLock()
                   
    def readData( self ):
        """readData()
        
        Reads the data from the data source.

        Preconditions:
            this process has a lock on the basefile
        """
        log.trace( 'entry' )
        
        self.__threadLock.acquire()
        try:
            self.__checkLock()
            
            if os.path.exists( self.__filename ):
                rawData = utils.readFile( self.__filename )
                log.debug( 'Got %s lines of raw data.' % len( rawData ) )
                self.__parseData( rawData )
                log.debug( 'Parsed into %s lines of usable data.' % len( self.__data ) )
            
            self.__dataCached = 1
        finally:
            self.__threadLock.release()
            
        log.trace( 'exit' )                    
                        
    def recordDLL( self , dll , base , size ):
        """recordDLL(d,b,s)
        
        Records dll d with base b and size s in the basefile. If data for d
        is already present, the new data will replace the old data (only one
        record can exist per dll).
        
        Preconditions:
            this process has a lock on the basefile
            data has been read in
        """
        # Actually, writes the entire file after removing the original entry
        # for the DLL, if any.
        log.trace( 'entry' , [ dll , base , size ] )
        
        self.__threadLock.acquire()
        try:
            self.__checkLock()
            self.__checkDataCached()
            
            if self.__isDLLRecorded( dll ):
                log.debug( 'DLL %s previously recorded - removing.' % dll )
                self.__removeDLL( dll )
                    
            log.debug( 'Adding record for %s' % dll )
            self.__addRecord( [ dll , "0x%x" % base , "0x%x" % size ] )
            
            log.debug( 'Recording data to basefile.' )
            self.__recordData()
        finally:        
            self.__threadLock.release()
            
        log.trace( 'exit' )        

    def unlock( self ):
        """unlock()
        
        Releases the exclusive lock on the data source.
        """
        self.__lock.unlock()
        
    def __addRecord( self , fields ):
        """__addRecord(l)
        
        Adds a data record from data passed in list l. Format:
            [ name:string , base:string , size:string ]
            base and size are expected to be in the format:
            "0x%x" % integer
        """
        record = {}
        record['name'] = fields[0]
        record['base'] = eval(fields[1])
        record['size'] = eval(fields[2])
        
        self.__data.append(record)        
                
    def __checkDataCached( self ):
        """__checkDataCached()
        
        Checks precondition that data has been read in. Raises 
        BasefileException if it hasn't.
        """
        if not self.__dataCached:
            error = 'Must use readData() prior to querying basefile.'
            log.error( error )
            raise BasefileException( error )
        
    def __checkFile( self , filename ):
        """__checkFile(f)
        
        Checks precondition that filename f is in the basefile. Raises
        BasefileException if it isn't.
        """
        if not self.__isDLLRecorded( filename ):
            error = 'DLL %s is not in the basefile' % filename
            log.error( error )
            raise BasefileException( error )
            
    def __checkLock( self ):
        """__checkLock()
        
        Checks precondition that we have file lock. Raises BasefileException 
        if we don't.
        """
        if not self.__lock.hasLock():
            error = 'Must use lock() before using any other methods of Basefile.'
            log.error( error )
            raise BasefileException( error )
            
    def __getData( self , filename , item ):
        """__getData(f,i) --> variable type
        
        Returns the data item i for the dll f from the basefile data source.
        """
        i = self.__getIndex( filename )
        return self.__data[i][ item ]            
            
    def __getHighestDLL( self , dll ):
        """__getHighestDLL(d) --> string
        
        Returns the DLL with the highest base, ignoring d in the determination.
        """
        log.trace( 'entry' , [ dll ] )
        highest = 0
        name = ''
        for item in self.__data:
            if dll == item['name']:
                continue

            base = self.__getData( item['name'] , 'base' )
            if base > highest:
                highest = base
                name    = item['name']
        log.trace( 'exit' , [ name ] )
        return name        
        
    def __getIndex( self , filename ):
        """__getIndex(f) --> integer
        
        Returns the index to the hash in the list whose 'name' attribute == f,
        or -1 if not present.
        """
        for i in range( len( self.__data ) ):
            if filename == self.__data[i]['name']:
                return i                
        return -1
        
    def __isDLLRecorded( self , dll ):
        """__isDLLRecorded(d) --> boolean
        
        Returns true if DLL d is found in data cache. This method consolidates
        the code from isDLLRecorded to prevent threadLock contention.
        """
        found = 0
        for item in self.__data:
            if dll == item['name']:
                found = 1
        return found                
        
    def __parseData( self , data ):
        """__parseData(d)
        
        Parses the raw list of data lines d into an internal dictionary.
        """
        self.__data = []
        for line in data:
            fields = self.__parseLine( line )
            if len(fields):
                self.__addRecord( fields )
                
    def __parseLine( self , line ):
        """__parseLine(l) --> list
        
        Parses line l into a list of items.
        """
        # This could blow up if dll's have spaces in their paths. So far, never
        # been an issue, and this code was copied during a refactor so as not
        # to change behavior. If this problem ever crops up, this is a good
        # place to look...
        l = line.split(";")[0]        
        l = l.replace("\t"," ")
        l = l.split()
        return l
        
    def __recordData( self ):
        """recordData()
        
        Records the data out to the basefile.
        """
        # # Sort the data by the base address.
        # self.__data.sort(lambda x,y: cmp(x['base'] , y['base']) )
        lines = []
        for item in self.__data:
            lines.append( "%s\t0x%x\t0x%x ; " % ( item['name'],
                                                  item['base'],
                                                  item['size'] ))                                                  
        log.debug( "Writing %s lines to basefile" % len( lines ) )
        utils.writeStringsToFileNow( lines , self.__filename )        
        
    def __removeDLL( self , dll ):
        """__removeDLL(d)
        
        Removes the data for dll d from the data set.
        """
        i = self.__getIndex(dll)
        if i < 0:
            log.warn( 'Asked to __removeDLL() on non-present data: %s' % dll )
        else:
            del self.__data[i]
