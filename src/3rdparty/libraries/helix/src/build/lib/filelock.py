# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: filelock.py,v 1.2 2006/06/19 23:11:27 jfinnecy Exp $ 
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
"""This module implements a platform-portable lock class for locking resources
across multiple processes. Intended primarily as a FileLock for a shared data
source, it should be usable for *any* inter-process locking needs, so long as
all processes a) know where to expect the lock file; and b) have write 
access to that location, which both seem like reasonable assumptions.

Classes:
    FileLock
"""
import os
import time
import thread

import log
log.debug( 'Imported: $Id: filelock.py,v 1.2 2006/06/19 23:11:27 jfinnecy Exp $' )
import utils


class FileLock:
    """Portable file locking mechanism. This class is intended for making file
    locks in a multi-process environment, where thread locking will not
    suffice. The file lock is specific to a proc-thread combination and will
    block all other proc-thread combinations that try to get the same lock.
    
    Stale Lock Handling:
        Object will try to remove its lock upon object destruction if it still 
    has it, which will resolve some potential stale lock issues, but processes 
    which terminate unexpectedly will still leave locks behind.    
        When creating a lock, the user may specify an expiration time e in
    seconds. This signals the object that the lock should expire e seconds
    after it is created.
        For a description of lockfile names and expirations, see the 
    documentation for __init__.

    Portability:
        This class is portable across all platforms.
    
    Assumptions: 
        This class *must* have write access in the directory where the file to
    be locked resides and all processes must agree (externally) to the name and
    location of that file.
    
    Thread safety:
        This class is thread safe.
    
    Methods:
        getLock()   - gets an exclusive lock on a file (blocking)
        hasLock()   - do we have the file lock?
        release()   - releases the lock        
    """    
    def __init__( self , name , expiration=0 ):        
        """__init__(n, expiration=e)
        
        Initialize the object with the name of the file you want to lock. For
        best results, use absolute pathing. The object will make a semaphore of
        the form <name>_tmp_lockfile in the same location as the target file.
        
        Caller may specify an optional expiration value e, which is how many
        seconds from the time the file lock (the lock, not the object) is 
        created before it "expires". An expired file lock will be ignored by 
        others attempting to lock the same file. If not specified, locks never
        expire on their own.
        
        Behavior may be weird as a FileLock gets close to its expiration. All
        efforts have been made to prevent such weirdness, but its better to
        use expiration appropriately: pick values that absolutely mean your
        process has died unexpectedly. If you think, in the worst case, that
        your process will need the lock for 30 seconds, set an expiration of
        5 minutes (or more). Don't set an expiration of 35 seconds.
        """
        self.__lockfile   = '_'.join( [ name , 'tmp' , 'lockfile' ] )
        self.__expSecs    = expiration
        self.__threadLock = thread.allocate_lock()
        
        log.debug( 'Using %s as a file lock' % self.__lockfile )

        # Internal arbitrary constants
        self.__checkCycles = 30   # Check for stale locks every this many cycles.
        self.__sleepTime   = 0.5  # How much time (sec) to sleep between loops.  
        
    def __del__( self ):
        """Catches graceful program aborts (like keyboard interrupts), not so
        gracefule aborts (like propagated exceptions to the interpreter), but
        not total process meltdown (like closing shells).
        """
        log.debug( "One last check for stale locks in object destruction." )
        if self.__hasLock():
            self.unlock()
        else:
            log.debug( "I don't have a lock. My work is done." )
        
    def getLock( self ):
        """getLock()
        
        Gets an exclusive lock on the file. This call is blocking. In some 
        cases, might exit execution without a lock (like on an IO exception). 
        Caller should always test hasLock() to be sure the lock was actually
        received.
        """
        log.trace( 'entry' )
        
        count = 0    # How many times have we looped
        self.__threadLock.acquire()
        try:
            while not self.__hasLock():
                if self.__lockfileExists():
                    if self.__isTimeToCheckForStaleLocks( count ):
                        self.__checkForStaleLock()
                        
                    self.__threadLock.release()
                    time.sleep( self.__sleepTime )
                    count += 1                    
                    self.__threadLock.acquire()
                else:
                    self.__tryToGetLock()
        finally:
            self.__threadLock.release()
            
        log.trace( 'exit' )                
   
    def hasLock( self ):
        """hasLock() --> boolean
        
        Do we have the file lock?
        """
        try:
            self.__threadLock.acquire()
            hasLock = self.__hasLock()
        finally:
            self.__threadLock.release()        
        
        return hasLock
        
    def unlock( self ):
        """unlock()
        
        Releases the lock.
        """        
        log.trace( 'entry' )
        
        self.__threadLock.acquire()
        try:
            # If we have the lock, but we are near being stale, then we are
            # going to leave the lockfile and let the next guy stale us out.
            # This way we avoid the problem where we had the lock, but
            # then our process slept and some other process staled us out,
            # replaced our lock, and then we awaken and remove their lock.
            if self.__hasLock():
                if self.__isNearStale():
                    log.debug( "I'm near stale, so pretending I already staled out." )
                else:
                    log.debug( "Removing the lockfile %s." % self.__lockfile )
                    os.remove( self.__lockfile )
            else:
                log.debug( "Not my lock anymore - hopefully I staled out." )
        finally:
            self.__threadLock.release()
        
        log.trace( 'exit' )        
        
    def __checkForStaleLock( self ):
        """__checkForStaleLock()
        
        Tries to detect if a lock is stale. A stale lock is defined as a
        lockfile that is older than its internally-specified expiration.
        """
        log.debug( "Checking for stale lock in '%s'." % self.__lockfile )
        expiration = self.__getExpirationFromFile()
        if expiration: 
            if self.__isStale( expiration ):                
                log.debug( "Lockfile expired at '%s' (current = %s)." % 
                                ( expiration , int(time.time()) ) )
                log.debug( "Removing stale lock %s." % self.__lockfile )
                os.remove( self.__lockfile )
        else:
            log.debug( "This lockfile never expires." )

    def __formatLineForLockfile( self ):
        """__getLineForLockfile()
        
        Returns the formatted line to use as the identifier in the lockfile
        for this object.
        """
        # Line has the format of PID!ThreadID@Expiration.
        # This uses two different delimiters ( ! @ ) to make splitting to get
        # the desired values easier later.
        # Expiration is the localtime in seconds after which the lockfile can
        # be considered to be expired.
        if self.__expSecs:
            expiration = int(time.time()) + self.__expSecs
        else:
            expiration = 0
        
        line = "%s!%s@%s@" % ( os.getpid() , thread.get_ident() , expiration )
        return line

    def __getExpirationFromFile( self ):
        """__getExpirationFromFile() --> int
        
        Returns the expiration time (in seconds) found in the lockfile.
        """
        line = self.__getFirstLineFromLockfile()
        if line:
            line = int(line.split( '@' )[1])            
        return line
        
    def __getFirstLineFromLockfile( self ):
        """__getFirstLineFromLockfile()
        
        Returns the first line from the lockfile.
        """
        # Race conditions don't matter here.
        line = None
        if os.path.exists( self.__lockfile ):
            line = utils.readLineFromFile( self.__lockfile )
            line = line.split( '\n' )[0]            
        return line            

    def __getIDFromFile( self ):
        """__getIDFromFile() --> string
        
        Returns the process-thread ID from the lockfile.
        """
        line = self.__getFirstLineFromLockfile()
        if line:
            line = line.split( '@' )[0]            
        return line 
       
    def __hasLock( self ):
        """__hasLock() --> boolean
        
        Returns true if the caller has the FileLock.
        """
        # Having the file lock is defined as having pid and thread_id that
        # match those recorded in the first line of the lockfile.
        hasLock = False
        fID = self.__getIDFromFile()
        if fID == "%s!%s" % ( os.getpid() , thread.get_ident() ):
            hasLock = True
        return hasLock
        
    def __isNearStale( self ):
        """__isNearStale() --> boolean
        
        Returns true if our lockfile will stale out in the next 30 seconds.
        """
        nearAdjustment = 30 # Define how many seconds is "near".        
        nearExpiration = self.__getExpirationFromFile()
        if nearExpiration:
            nearExpiration -= nearAdjustment
            return self.__isStale( nearExpiration )
        else:
            # expiration == 0 means never expire
            return False
            
    def __isStale( self , expiration ):
        """__isStale(t) --> boolean
        
        Returns True if time t has already passed.
        Returns False if time t is 0.
        """
        if ( expiration < int(time.time()) ):
            return True
        else:
            return False
            
    def __isTimeToCheckForStaleLocks( self , count ):
        """__isTimeToCheck(c) --> boolean
        
        We have done count c loops; is it time to check for stale locks?
        """
        # This is separated out to increase readability in the getLock() loop.
        if 0 == count % self.__checkCycles:
            return True
        else:
            return False
            
    def __lockfileExists( self ):
        """__lockfileExists() --> boolean
        
        Does the lockfile exist?
        """
        # This is separated out to increase readability in the getLock() loop.
        if os.path.exists( self.__lockfile ):
            return True
        else:
            return False     
            
    def __tryToGetLock( self ):
        """__tryToGetLock()
        
        Participate in a race with as many other processes as care, to append
        data to the lockfile. Winner will be first data into the file.
        """
        line = self.__formatLineForLockfile()                
        log.debug( 'Writing %s to lockfile' % line )
        utils.appendStringToFile( line , self.__lockfile )
        
        
if __name__ == '__main__':
    #
    # Test stub for FileLock class.
    #
    # Since checking this class requires multiple processes (all getLock() calls
    # are blocking) we don't currently have a test framework to run (meaningful) 
    # unit tests against the FileLock class.
    #
    # This stub can be used to manually run tests against the FileLock module in
    # place of automated unit tests.
    # 
    # Test cases:
    #    - Use expiring locks against (use short expiration times):
    #       1 Lockfile with no expiration exists
    #       2 Lockfile with expiration exists.
    #       3 No lockfile exists.
    #    - Use no-expiration locks against:
    #       4 Lockfile with no expiration exists
    #       5 Lockfile with expiration exists
    #       6 No lockfile exists
    # 
    # I have used the standard that a "Pass" means this module behaves as expected
    # for the above test cases with 4 simultaneous processes, and no stale lockfile
    # is left behind. The definition of "as expected" can be found in the FileLock 
    # documentation.
    #
    # In addition to the above tests, failure tests should be run (these can be
    #  single-process and would probably fit the unittest framework) where:
    #       7 Read errors on a FileLock you placed (no exceptions)
    #       8 Read error on a file someone else placed (bad data?) - (exception)
    #       9 Write errors on an existing FileLock (exception raised)
    #      10 keyboard interrupt is received (exception raised - file removed)
    #      11 Using FileLock's with expiration close to time of use.
    
    import sys
    
    if len( sys.argv ) < 3:
        print 'Usage: filelock <file> <expiration>'
        print
        print '   file       = name of file to apply a lock to'
        print '   expiration = seconds for lock to be valid'
        print
        sys.exit(1)
            
    import filelock    
    
    fl = filelock.FileLock( sys.argv[1] , int(sys.argv[2]) )
    
    fl.getLock()
    
    print "I got a lock!"
    
    for i in range( 12 ):
        print "sleeping 5 secs..."
        time.sleep( 5 )
    
    if fl.hasLock():
        print "I still have the lock."
    else:
        print "I don't have the lock anymore."
        
    fl.unlock()
