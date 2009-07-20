#!./python
#
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: log.py,v 1.13 2007/04/30 22:51:13 jfinnecy Exp $ 
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
"""Singleton class to provide some logging functionality.

    As a singleton, do not instantiate __Log objects directly.
    
    Static hooks are provided.
    Use the following methods directly:
    initLogging() - Sets up logging. Must be run before log calls will
                         write to any files.
                      This should be called once in the highest level of the
                      executing process. Each time this is called, logging will
                      begin writing to a new batch of logfiles. Typically, this
                      should be the first thing any executing script does, and
                      then *never* do this from within imports.
    
    debug()       - Use to log everything.
    error()       - Use to log an error message.
    info()        - Use to log a non-error message, typically what you'd want
                        to see on the screen.
    warn()        - Use to log a warning message - typically meant for use
                        as non-fatal asserts. These should be rare.

    trace()       - Use to trace method entries/exits.
    
    info, warn, and error also print to the screen, or the screen callback
    function, if it has been set:
    
    setCallback() - Point screen prints to a different handler method.
    
        
    Usage Tips:
        
    Use of this class creates up to 4 logfiles in the specified directory,
    one for debug-level, one for info, one for error (if any) and one for
    warn (if any). Each file in a given log "batch" is prefixed with the
    same 3-digit serial number, for easy grouping/handling. A "batch" occurs
    each time initLogging() is called, and so this should only be called once,
    at the earliest point in program execution.
    
    Logfiles will typically be of the format:
        <serial>.<identifier>.<facility>.log

    For optimum use of this class, pick identifier names that are easily
    identifiable with a discrete operation. For example, use buildID
    (001.80342.info.log) not process name (001.build.info.log).
    Currently, the ribosome system does not readily support this, and logs will
    tend to use either 'build' or 'umake' as an identifier.
    
    Typical example:
        
        Early in program execution, set up the logging system. Any call to
        initLogging() will start a new batch of logging in the specified dir.
            
        import log
        
        log.initLogging( someDir, anIdentifier )
        
 
        Then, in any other place during execution, to log, simply use the
        public hooks:
            
        log.info( msg )
        
        info() can be replaced with debug(), error() or warn() as appropriate.
        
        Usage inside a method will normally look like:
            
        def someMethod( value1, value2 ):
            log.trace( 'entry', [value1, value2] )
            
            <actual method code here>
            
            log.trace( 'exit' , retValue )
            return retValue                       

       
        Messages log as follows:
            
                                      Facility
                          Info  Debug  Error  Warn  Screen
                  INFO     XX    XX                   XX
        LogLevel  DEBUG          XX
                  WARN     XX    XX            XX     XX
                  ERROR    XX    XX     XX            XX
                  
        The logic is:
            Debug logs only to the debug facility.
            Info is for things you would normally log to the screen.
            Error is for execution errors, like missing files, etc.
            Warn is for program errors (code bugs).
            Error and Warn, both log to the screen, so get mirrored to Info,
              but otherwise have their own files for easy spotting.
            *Everything* logs to debug.
          
        Finally, should you want to transfer screen dump over a network pipe
        or otherwise process it, you can use the setCallback() method to
        replace printing to stdout with some other homegrown handler.
        
    ** Run this file on the command line to execute module unit tests. Check 
    log.main.__doc__ for more details.
    
    Future additions:
        Add methods to dynamically adjust log/facility matrix.
        Add pre-init buffering to resolve any chicken/egg issues (such as
           waiting to figure out a directory or identifier name from relevant
           data that might not be known at process bootstrap time).
"""

#
# Issues & Thoughts:
#
# One of the things to keep an eye on is diskspace usage. Since logfiles
# are serialized (and the debug logs can be *quite* large) we may need to
# keep an eye on this.
#

# Standard imports
import os
import traceback
import thread
import re
import string

# Ribosome Library imports
import timestamp


#
# Static Attributes
#

# Singleton reference - don't touch this directly! 
# You should only be using the public hooks below to do your logging anyways.
instance = None

#
# Gets set to true when running unit tests to enable debug output.
__LOG_DEBUG__ = 0

#
# Lock for thread-safe operations
lock = thread.allocate_lock()

#
# Log/facility matrix - this tells us which facilities to write to for a given
#   log level.
# Future: provide methods to change this matrix at run-time.
matrix = {}
matrix ['INFO']  = ['info','debug','screen']
matrix ['WARN']  = ['warn','debug','screen']
matrix ['ERROR'] = ['info','error','debug','screen']
matrix ['DEBUG'] = ['debug']
matrix ['ENTRY'] = ['debug']
matrix ['EXIT']  = ['debug']


def __getLog():
    """Properly handles singleton object instantiation. Do not use directly."""
    global instance
    global lock
    
    lock.acquire()
    if not instance:
        instance = __Log()  
    lock.release()
    
    return instance

    
#
# Public interface.
#
# This is your interface to the logging system.
#
def debug(msg):
    """debug(m)
    
    Logs the message m with the DEBUG log level.
    """
    obj = __getLog()
    obj.debug(msg)
        
    
def error(msg):
    """error(m)
    
    Logs the message m with the ERROR log level.
    """
    obj = __getLog()
    obj.error(msg)
    
    
def info(msg):
    """info(m)

    Logs the message m with the INFO log level.
    """
    obj = __getLog()
    obj.info(msg)
    
    
def trace( type, params=None ):
    """trace(e, [p, <p...>])

    Logs a method entry or exit, along with arguments or return values. e
    should specify one of 'entry' or 'exit' (case insensitive). Anything
    other than 'exit' will be interpreted as 'entry'. Parameters can be
    anything (typically just repeat the arguments or return values).
    
    The log line will show the function of interest, a facility designator
    of [ENTRY] or [EXIT ], and the values of the paramaters at run-time.
    
    Example:
        def someFunction(aString, aList):
            log.trace('ENTRY', [aString, aList])
            <snip>                
            log.trace('exit', [retVal1, retVal2])
            return (retVal1, retVal2)        
    """
    obj = __getLog()
    obj.trace( type, params )
    
    
def warn(msg):
    """warn(m)
    
    Logs the message m with the WARN log level.
    """
    obj = __getLog()
    obj.warn(msg)
    
    
def initLogging(dir,id):
    """initLogging(d, i)
    
    Initializes the log object as follows:
        
    - Sets the log output dir to d. Will try to make d if d does not exist.
    - Sets the identifier equal to i for use in building log file names.        
    - Sets the serial number to use for all logfiles in this batch.
    - Sets the message counter to 0.
    
    If there are no errors, sets isInitialized to 1, otherwise, sets it
      to 0.
    
    Logfile names end up being like:
        <d>\<serial>.<i>.<facility>.log
    
    Example:
        l.initLogging('c:\\temp\\logging','testLog')
        l.info(msg)
        -->
        c:\temp\logging\001.testLog.info.log
    """
    obj = __getLog()
    obj.initLogging(dir, id)

    
def setCallback(fn):
    """setCallback(m)
    
    Sets the method to call for "screen" facility logs. If not set, screen
    prints will print to the screen. This is useful for sending logs
    through network pipes for capture in existing logging systems.
    """
    obj = __getLog()
    obj.setCallback(fn)
    
    
#
# Class implementation.
#
class __Log:
    """None of these methods should be called directly. Use the public
    interface methods defined above.
    """
    def __init__(self):
        if __LOG_DEBUG__:
            print 'Instantiating singleton log object'
                    
        self.logDir        = None      # The directory to log to.        
        self.serial        = 0         # Serial component of filenames.
        self.identifier    = None      # Identifier component of filenames.
        self.counter       = 0         # Log counter.
        self.isInitialized = 0         # Status of the log object.
        self.callback      = None      # Callback function to use in place
                                       #   of screen prints.     
    #
    # Public class methods
    #    
    def initLogging(self, dir, identifier):
        """Internal method - see log.initLogging.__doc__ for details."""
        global lock
        lock.acquire()
 
        # Find/make the directory for the logfiles.
        errors = 0
        if not os.path.isdir(dir):
            try:
                self.__makeDir(dir)
            except:
                errors = 1
                
        # If there were no problems laying down the directory, go ahead
        # and set up the object.
        if not errors:
            self.logDir        = dir
            self.identifier    = identifier
            self.counter       = 0
            self.serial        = self.__getSerial(dir)
            self.isInitialized = 1
        else:
            print 'Error trying to make log dir: %s' % dir
            print 'Logging not initialized!'

        lock.release()
        
        
    def setCallback(self, method):
        """Internal method - see log.setCallback.__doc__ for details."""
        self.callback = method
        
        
    def trace(self, entry , params=None):
        """Internal method - see log.trace.__doc__ for details."""
        # Turn the list of values into a printable string.
        if params: 
            paramString = string.join( ['%s' % (param) for param in params] , ', ')
        else:
            paramString = ''

        # Examine the trace stack to get the calling function.
        funcName = traceback.extract_stack()[-3:-2][0][2]
        modName  = traceback.extract_stack()[-3:-2][0][0]

        # Format the message based on entry/exit state.
        if entry.upper() == 'EXIT':
            entry = 'EXIT'
            msg = 'Leaving %s::%s()' % (modName, funcName)
            if paramString:
                 msg += ' with return values: %s' % paramString                
        else:
            entry = 'ENTRY'
            msg = 'Entering %s::%s(%s)' % (modName,
                                           funcName, 
                                           paramString)
        
        # Time to log the message.
        self.__log(entry, msg)
                      
        
    def debug(self, msg):
        """Internal method - see log.debug.__doc__ for details."""
        self.__log('DEBUG', msg)

        
    def error(self, msg):
        """Internal method - see log.error.__doc__ for details."""
        self.__log('ERROR', msg)       

        
    def info(self, msg):
        """Internal method - see log.info.__doc__ for details."""
        self.__log('INFO', msg)
        
        
    def warn(self, msg):
        """Internal method - see log.warn.__doc__ for details."""
        self.__log('WARN', msg)
    
                
    #
    # Private class methods
    #
    def __formatMessage(self, logType, msg):
        """__formatMessage(t,m) -> string
        (Private method)
        
        Builds a formatted message string that includes logType t, message m, 
        and a timestamp. The format is:
            
        [#-00000327][2006-01-24 12:32:31][6776][INFO ] : This is a sample message.
        [Counter   ][Date               ][Thrd][Level] : <message>
        """
        return '[#-%08d][%s][%d][%-5s] : %s' % (self.counter,
                                             timestamp.Timestamp().getLocalString(), 
                                             thread.get_ident(),
                                             logType,
                                             msg)
        

    def __getLogfile(self, facility):
        """__getLogfile(f) --> string
        (Private method)
        
        Determines the file name to use for a given facility, and returns this
        filename as a string of the format:
            004.<identifier>.<facility>.log
        """
        filename = '%03d.%s.%s.log' % ( self.serial, self.identifier, facility)
        filename = os.path.join(self.logDir, filename)
        
        return filename
        
                                                                                
    def __getSerial(self, dir):
        """__getSerial(d) --> int
        (Private method)
        
        Determines the next serial number to use for a new log batch in the
        given dir d.
        """
        highest = 0
        
        if os.path.isdir(dir):
            if __LOG_DEBUG__:
                print 'Found directory to serialize'
            logs = os.listdir(dir)
            if logs:
                for logfile in logs:
                    # See if file is of interest to us.
                    # Regex matches:
                    #    one or more digits            [ \d+ ]
                    #    followed by a period          [ \. ]
                    #    followed by any string        [ .* ]
                    #    followed by the string '.log' [ \.log ]
                    if re.match( '\d+\..*\.log' , logfile ):
                        (serial, dummy) = logfile.split('.',1)
                        serial = int(serial)
                        if __LOG_DEBUG__:
                            print 'Splitting logfile %s' % logfile
                            print 'Found serial %d' % serial
                        if serial > highest:
                            highest = serial
        else:
            print 'LOG.PY::__getSerial() - cannot find dir to serialize.'
        
        highest += 1
        
        return highest
        
        
    def __log(self, logLevel, msg):
        """ __log(l,m)
        (Private method)
        
        Logs message m to facilities specified for loglevel l.
        
        If __log cannot write to a file, it will print to stdout.
        """
        # This entire method should be an atomic operation since shared data is
        # used throughout, and also to make sure that things get logged in the
        # order that their log statements are executed.
        global lock                                                                                                                         
        lock.acquire()
                                      
        # Format the message.
        fMsg = self.__formatMessage(logLevel, msg)
                
        # Check for object initialization, then proceed.
        if self.isInitialized:
            # Log to each facility specified for the given log level.
            for facility in matrix[logLevel.upper()]:
                if __LOG_DEBUG__:
                    print 'Logging to facility: %s' % facility
                self.__writeMessage(fMsg, facility)
        else:
            # Important for not losing log messages when log object is not
            # initialized. Temporarily disabled to cut down on noise at user
            # request while fixing issues with automation.
            # print 'Log object not initialized - cannot log: %s' % fMsg
            pass

        self.counter += 1
        
        lock.release()

        
    def __makeDir(self, dir):
        """__makeDir(d)
        (Private method)
        
        Recursively builds a directory path.
        
        Propagates all exceptions to the original caller.
        """
        # Recurses into the path until it finds a dir that exists, then makes
        # the subdirs all the way back out.
        (parent, node) = os.path.split(dir)
        if __LOG_DEBUG__:
            print 'Splitting %s into %s and %s' % (dir, parent, node)
        
        # Keep recursing until we find a parent that exists.
        if not os.path.isdir(parent):
            if __LOG_DEBUG__:
                print 'Parent dir %s does not exist' % parent
            self.__makeDir(parent)
        
        # Now, make each node as we pop off the call stack.
        if __LOG_DEBUG__:
            print 'Parent %s exists' % parent
            print 'Making node %s' % dir
        os.mkdir(dir)
        
        
    def __writeMessage(self, msg, facility):
        """__writeMessage(m, f)
        (Private method)
        
        Writes the specified message to the facility f.
        """
        if facility == 'screen':
            if self.callback:
                if __LOG_DEBUG__:
                    print 'Using callback function %s' % self.callback
                self.callback(msg)
            else:
                # Strip the trailing newline.
                print(msg)
        else:
            logfile = self.__getLogfile(facility)
            if __LOG_DEBUG__:
                print 'Writing to logfile %s' % logfile
            try:
                log = open(logfile, 'a')
                try:
                    log.write('%s\n' % msg)
                finally:
                    log.close()
            except:
                print 'Cannot write to logfile %s : %s' % (logfile, msg)
                self.errors = 1
            
            
#
# Unit Tests
#

def main():
    """Unit tests log to \tmp\logtest or /tmp/logtest directory. Manually
    verify the contents of the log files after running.
    """
    logdir = os.path.join('\\','tmp','logtest')
    
    def dummyFunc(someVal):
        trace('entry', [someVal])
        
        exitVal = 1
        trace('exit', [exitVal])
            
    def whackedFunc(arg1, arg2):
        trace('entry', [arg1, arg2])
        
        exitVal = {'outKey':'outVal'}
        trace('exit', [exitVal])
    
    global __LOG_DEBUG__
    __LOG_DEBUG__ = 1
    
    print
    print 'Running unit tests for log.py'
    print
    
    print 'Try logging before its time'
    info('Blee - too soon!')
    
    id = 'test'
    print "Init'ing log object:"
    print '  dir: %s' % logdir
    print '  id : %s' % id
    initLogging(logdir, id)
    print
    
    s = 10
    b = 'A string'
    print 'Calling an entry trace from main() with s = %s, b = %s' % (s,b)
    trace('entry' , [s , b] )
    print
    
    print 'Calling an exit trace from main() with same values'
    trace( 'exit' ,  [s , b] )
    print
        
    someString = 'This is a message string'
    print 'Calling a message log with the string: %s' % someString
    info( someString )
    print
    
    print 'Calling an error log'
    error( 'This is an error string' )
    print
    
    print 'Calling a debug log'
    debug( 'This is a debug message' )
    print
    
    print 'Warning Warning - Danger Will Robinson!'
    warn('Will Robinson would be proud.')
    print
    
    print 'Checking trace, calling dummyFunc(5), expecting 1 return'
    dummyFunc(5)
    print
    
    print 'See if this looks ugly with whacked params.'
    a = [(3,['a'])]
    b = {'key':'value'}
    print 'a = %s\nb = %s' % ( a, b )
    whackedFunc(a,b)
    print
    
    print 'Setting callback equal to a function that prepends an *'
    def myCallback(m):
        print '*' + m
    setCallback( myCallback )
    print 'And calling an info log'
    info('Testing setCallback()')
    print 'End of unit tests for log.py'
    

if __name__ == '__main__':
    main()
