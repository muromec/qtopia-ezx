# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: shell.py,v 1.35 2006/08/22 20:58:47 jfinnecy Exp $ 
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
"""Python implementation of some common shell and file system utilities
missing from the os and os.path module."""

import os
import sys
import string
import getopt
import stat
import re
import posixpath
import glob
import types
import time

import log
log.debug( 'Imported: $Id: shell.py,v 1.35 2006/08/22 20:58:47 jfinnecy Exp $' )

import chaingang

## unix-specific
try:
    import signal
    import select
    if sys.version[:3] < '2.2':
        import fcntl, FCNTL
    else:
            ## Python 2.2+
        import fcntl
        FCNTL = fcntl # backwards compatibility
except:
    pass

## macintosh-specific globals
try:
    import macfs
    import MacOS
except:
    pass

## win32 extensions
try:
    import win32api
    import win32security
    import win32pipe
    import win32process
    import win32file
    import win32event
    import pywintypes
    import win32con
except:
    win32api = None

## exception
error = "shell.py error"

## maximum chunk to read at a time while copying files
MAX_READ = 8192


## shell utility implementation
class ShellUtilities:

    def setup_tempdir(self):
        pass #FIXME

    def reset_tempdir(self):
        pass #FIXME
    
    def cleanup_tempdir(self):
        pass #FIXME
    

    if hasattr(os, "chmod"):
        def _chmod(self, file, mode):
            return os.chmod(file, mode)
    else:
        def _chmod(self, file, mode):
            pass

    if hasattr(os, "utime"):
        def _utime(self, file, atime, mtime):
            return os.utime(file, (atime, mtime))
    else:
        def _utime(self, file, atime, mtime):
            pass

    def _open(self, path, mode):
        try:
            return open(path, mode)
        except IOError, e:
            raise error, 'open("%s","%s") failed with IOError="%s" in "%s"' % (
                path, mode, str(e), os.getcwd())

    def _popen(self, command, mode, dir=None):
        old_dir=os.getcwd();
        if dir:
            os.chdir(dir)
        try:
            ret=os.popen(command, mode)
        except os.error, e:
            raise error, 'os.popen("%s") failed os.error="%s" in "%s"' % (
                command, str(e), os.getcwd())
        os.chdir(old_dir)
        return ret

    def glob(self, path):
        globbed = []
        try:
            globbed = glob.glob(path)
        except:
            pass

        if not globbed and os.path.exists(path):
            globbed = [path]

        return globbed
    
    def rm(self, rm_path):
        for path in self.glob(rm_path):
            self._rm_single(path)

    def cp(self, source_path, destination_path):
        log.trace( 'entry' , [source_path, destination_path] )
        for path in self.glob(source_path):
            if os.path.isfile(path):
                self._cp_single(path, destination_path)
            elif os.path.isdir(path):
                self._cp_dir(path, destination_path)
        log.trace( 'exit' )

    def cpdir(self, source_path, destination_path):
        for path in self.glob(source_path):
            if not os.path.isdir(path):
                raise error, 'cpdir() "%s" not a directory' % (path)

            bdir = os.path.basename(source_path)
            dpath = os.path.join(destination_path, bdir)
            self._cp_dir(path, dpath)

    def find(self, path):
        file_list, directory_list = self._find(path)
        return file_list, directory_list

    def mkdir(self, dir):
        if not dir or os.path.isdir(dir):
            return
        self.mkdir(os.path.dirname(dir))
        try:
            os.mkdir(dir)
        except OSError:
            if not os.path.isdir(dir):
                raise

    def run(self, command, line_cb, timeout_seconds, dir = None, keep_lock=None):
        log.trace( 'entry' , [command , line_cb , timeout_seconds , dir , keep_lock ] )
        
        self.setup_tempdir()
        pipe = self._popen(command, "r", dir)
        self.reset_tempdir()

        cwd=os.getcwd()

        if not keep_lock:
            chaingang.chdir_lock.release()
        try:
            output=[]
            lastline = ""
            
            try:
                pipe.readline
                has_readline=1
            except AttributeError:
                has_readline=0

            if line_cb and has_readline:
                while 1:
                    tmp = pipe.readline()

                    if not tmp:
                        break

                    output.append(tmp)
                    line_cb(tmp)

            else:
                while 1:
                    try:
                        tmp = pipe.read(4096)
                    except:
                        break

                    if not tmp:
                        break

                    output.append(tmp)

                    if line_cb:
                        lastline = lastline + tmp
                        lines = string.split(lastline, "\n")
                        lastline = lines[-1]
                        for line in lines[:-1]:
                            line_cb(line+"\n")

                if len(lastline):
                    line_cb(lastline)

            status = pipe.close()

        finally:
            if not keep_lock:
                chaingang.chdir_lock.acquire()
                os.chdir(cwd)
            self.cleanup_tempdir()
            
        log.trace( 'exit' , [ status ] )
        return status, string.join(output,"")

    def _rm_single(self, path):
        ## first take a wild stab at it
        try:
            os.remove(path)
        except os.error:
            pass
        else:
            return

        ## remove a directory
        if os.path.isdir(path):
            try:
                name_list = os.listdir(path)
            except os.error:
                name_list = []

            for name in name_list:
                fullname = os.path.join(path, name)
                self._rm_single(fullname)

            ## remove the directory
            os.rmdir(path)
            return

        ## Try again
        try:
            try:
                os.chmod(path, 0666)
            except:
                pass
            os.remove(path)
        except os.error:
            pass
        else:
            return

        raise error, 'Failed to delete file/dir = %s' % path

    def _cp_dir(self, source_path, destination_path):
        log.trace( 'entry' , [source_path, destination_path] )
        ## does the directory exist
        if not os.path.isdir(source_path):
            log.error('Cannot find source dir %s' % source_path )
            raise error, 'cp_dir() cannot find source directory="%s"' (
                source_path)

        self.mkdir(destination_path)

        src = [ [] ]
        for relparts in src:
            dir = apply(os.path.join, [ source_path ] + relparts)
            dest_dir = apply(os.path.join, [ destination_path ] + relparts)

            for file in os.listdir(dir):
                full_file = os.path.join(dir, file)
                full_dest = os.path.join(dest_dir, file)

                if os.path.isdir(full_file) and not os.path.islink(full_file):
                    src.append( relparts + [ file ] )  # This makes it recursive
                    self.mkdir( full_dest )
                else:
                    self._cp_single( full_file, full_dest )
        log.trace( 'exit' )

    def _cp_single(self, source_path, destination_path):
        log.trace( 'entry' , [source_path, destination_path] )
        ## does file exist?
        if not os.path.isfile(source_path):
            log.error('Cannot find source %s' % source_path )
            raise error, 'cannot find source="%s"' % (source_path)

        ## if the destination of the copy is a path, then we
        ## need to modify the destionation_path to have the
        ## source filename
        if os.path.isdir(destination_path):
            destination_path = os.path.join(destination_path,
                                            os.path.basename(source_path))

        ## preform the copy
        self._platform_cp(source_path, destination_path)
        log.trace( 'exit' )


    def _platform_cp(self, source_path, destination_path):
        ## copy file data
        source = self._open(source_path, "rb")
        destination = self._open(destination_path, "wb")

        self._raw_copy(source, destination)
        source.close()
        destination.close()

        ## stat the file; get file mode, atime, and mtime
        st = os.stat(source_path)

        if os.name == 'posix':
            os.chmod(destination_path, st[stat.ST_MODE])

        os.utime(destination_path, (st[stat.ST_ATIME], st[stat.ST_MTIME]))


    def _raw_copy(self, source, destination):
        data = source.read(MAX_READ)
        while data:
            destination.write(data)
            data = source.read(MAX_READ)


    def _find(self, directory):
        directory_list = []
        file_list = []

        ## list directory and separate the files from directories
        for path in os.listdir(directory):
            path = os.path.join(directory, path)

            if os.path.isdir(path) and not os.path.islink(path):
                directory_list.append(path)
            else:
                file_list.append(path)

        ## recurse into directories
        for dir in directory_list:
            fl, dl = self._find(dir)
            file_list = file_list + fl
            directory_list = directory_list + dl

        return file_list, directory_list


class UNIXShellUtilities(ShellUtilities):

    thread_safe=1

    class _pipe:
        "Wrapper for a unix fd which can wait() on a child process at close time."

        def __init__(self, fd, child_pid):
            self.fd = fd
            self.child_pid = child_pid

        def kill(self):
            try:
                os.kill(self.child_pid, 9)
            except os.error, e:
                pass

        def eof(self):
            pid, status = os.waitpid(self.child_pid, os.WNOHANG)
            if pid:
                os.close(self.fd)
                self.fd = -1
                return status
            return None

        def close(self):
            if self.fd >= 0:
                os.close(self.fd)
                self.fd = -1
                return os.waitpid(self.child_pid, 0)[1]
            return None

        def read(self, howmuch):
            return os.read(self.fd, howmuch)

        def write(self, data):
            return os.read(self.fd, data)

        def fileno(self):
            return self.fd

    def popen(self, cmd, args, mode, capture_err=1, dir = None):
        # flush the stdio buffers since we are about to change the
        # FD under them
        sys.stdout.flush()
        sys.stderr.flush()

        r, w = os.pipe()
        pid = os.fork()
        if pid:
            # in the parent
            # close the descriptor that we don't need and return the other one.
            if mode == 'r':
                os.close(w)
                return self._pipe(r, pid)
            else:
                os.close(r)
                return self._pipe(w, pid)

        # in the child

        # we'll need /dev/null for the discarded I/O
        try:
            null = os.open('/dev/null', os.O_RDWR)
        except os.error, e:
            raise error, 'cannot open "/dev/null" os.error="%s"' % (str(e))

        if mode == 'r':
            # hook stdout/stderr to the "write" channel
            os.dup2(w, 1)
            # "close" stdin; the child shouldn't use it
            # this isn't quite right... we may want the child to
            # read from stdin
            os.dup2(null, 0)
            # what to do with errors?
            if capture_err:
                os.dup2(w, 2)
            else:
                os.dup2(null, 2)
        else:
            # hook stdin to the "read" channel
            os.dup2(r, 0)
            # "close" stdout/stderr; the child shouldn't use them
            # this isn't quite right... we may want the child to write to these
            os.dup2(null, 1)
            os.dup2(null, 2)

        # don't need these FDs any more
        os.close(null)
        os.close(r)
        os.close(w)

        if dir:
            os.chdir(dir)

        # the stdin/stdout/stderr are all set up. exec the target
        try:
            os.execvp(cmd, (cmd,) + tuple(args))
        except:
            pass

        # crap. shouldn't be here.
        os._exit(127)

    def run(self, command, line_cb, timeout_seconds, dir = None, keep_lock=None):
        log.trace( 'entry' , [command , line_cb , timeout_seconds , dir , keep_lock ] )
        
        import errno
        status  = 0

        list = [ "/bin/sh","-c",command ]
        pipe = self.popen(list[0], list[1:], "r", 1, dir)

        cwd=os.getcwd()
        if not keep_lock:
            chaingang.chdir_lock.release()
        try:
            output = []
            line_buff = ""
            timeout_time = time.time() + timeout_seconds
            while 1:
                timeout = timeout_time - time.time()
                if timeout < 0:
                    pipe.kill()
                    raise error, 'shell.run() timeout on pid="%s"' % (
                        pipe.child_pid)

                try:
                    rlist, wlist, elist = select.select([pipe], [], [], timeout)
                except select.error, e:
                    if e[0] == errno.EINTR:
                        continue
                    output.append("\n BUILD WARNING, output truncated by select error %d \n" % e[0])
                    break

                if pipe in rlist:
                    try:
                        temp = pipe.read(8192)
                    except OSError, e:
                        if e.errno == errno.EINTR:
                            continue
                        output.append("\n BUILD WARNING, output truncated by read error %d \n" % e.errno)
                        break

                    if temp:
                        output.append(temp)
                    else:
                        break

                    if line_cb:
                        line_buff = line_buff + temp
                        while 1:
                            i = string.find(line_buff, "\n")
                            if i == -1:
                                break
                            i = i + 1
                            line_cb(line_buff[:i])
                            line_buff = line_buff[i:]

            status = pipe.close()

        finally:
            if not keep_lock:
                chaingang.chdir_lock.acquire()
                os.chdir(cwd)
        
        log.trace( 'exit' , [ status ] )
        return status, string.join(output,"")


class WinShellUtilities(ShellUtilities):
    pass


class Win9xShellUtilities(WinShellUtilities):
    def run(self, command, line_cb, timeout_seconds, dir=None, keep_lock=None):
        log.trace( 'entry' , [ command , line_cb , timeout_seconds , dir , keep_lock ] )
        
        old_dir=os.getcwd()
        try:
            if dir:
                os.chdir(dir)
            status = os.system(command)

        finally:
            os.chdir(old_dir)

        log.trace( 'exit' , [ status ] )
        return status, ""


class WinNTShellUtilities(WinShellUtilities):
    thread_safe=1 ## Hopefully

    def setup_tempdir(self):
        pass

    def reset_tempdir(self):
        pass
    
    def cleanup_tempdir(self):
        pass

    def _popen(self, command, mode, dir=None):
        command = "%s 2>&1" % (command)
        return WinShellUtilities._popen(self, command, mode, dir)


class WinNTExtendedShellUtilities(WinNTShellUtilities):
    thread_safe=1
    

    def make_inheritable(self, handle):
        p=win32api.GetCurrentProcess()
        ret=win32api.DuplicateHandle(p,handle,
                                     p,0,1,
                                     win32con.DUPLICATE_SAME_ACCESS)
        win32api.CloseHandle(handle)
        return ret
        

    def _launch_idle_process(self, command_line, dir=None):
        (hChildStdinRd, hChildStdinWr) = win32pipe.CreatePipe(None, 0)
        (hChildStdoutRd, hChildStdoutWr) = win32pipe.CreatePipe(None, 0)

        hChildStdinRd=self.make_inheritable(hChildStdinRd)
        hChildStdoutWr=self.make_inheritable(hChildStdoutWr)

        startupinfo = win32process.STARTUPINFO()
        startupinfo.dwFlags = \
            win32process.STARTF_USESTDHANDLES | \
            win32process.STARTF_USESHOWWINDOW;
        startupinfo.hStdInput = hChildStdinRd;
        startupinfo.hStdOutput = hChildStdoutWr;
        startupinfo.hStdError = hChildStdoutWr;

        appName = None
        commandLine = command_line
        processAttributes = None
        threadAttributes = None
        bInheritHandles = 1
        dwCreationFlags = win32process.IDLE_PRIORITY_CLASS
        newEnvironment = os.environ
        currentDirectory = None
        if dir:
            currentDirectory = os.path.normpath(os.path.join(os.getcwd(), dir))

        ## no dialog boxes that hang the build system
        SEM_FAILCRITICALERRORS = 0x0001
        SEM_NOGPFAULTERRORBOX = 0x0002
        SEM_NOALIGNMENTFAULTEXCEPT = 0x0004
        SEM_NOOPENFILEERRORBOX = 0x8000

        win32api.SetErrorMode(
            SEM_FAILCRITICALERRORS|\
            SEM_NOGPFAULTERRORBOX|\
            SEM_NOOPENFILEERRORBOX)

        try:
            (hProcess, hThread, dwProcessId, dwThreadId) = \
                win32process.CreateProcess(
                appName,
                commandLine,
                processAttributes,
                threadAttributes,
                bInheritHandles,
                dwCreationFlags,
                newEnvironment,
                currentDirectory,
                startupinfo)
        except pywintypes.error:
            return None

        ## close the thread handle, as well as the other I/O handles
        win32api.CloseHandle(hThread)
        win32api.CloseHandle(hChildStdinRd)
        win32api.CloseHandle(hChildStdoutWr)

        return hProcess, hChildStdinWr, hChildStdoutRd

    def run(self, command, line_cb, timeout_seconds, dir = None, keep_lock=None):
        log.trace( 'entry' , [ command , line_cb , timeout_seconds , dir , keep_lock ] )
        
        ## when _lanuch_idle_process fails, it returns None which
        ## will raise a TypeError from the tuple-assignment
        self.setup_tempdir()
        x=self._launch_idle_process(command, dir)
        self.reset_tempdir()
        
        try:
            (hProcess, hChildStdinWr, hChildStdoutRd) = x
        except TypeError:
            self.cleanup_tempdir()
            return 999, "Failed to launch process (%s)" % command

        cwd=os.getcwd()
        if not keep_lock:
            chaingang.chdir_lock.release()
        try:

            ## get the output of the command
            output = []
            line_buff = ""
            while 1:
                try:
                    r = win32event.WaitForSingleObject(
                        hChildStdoutRd, win32event.INFINITE)
                except pywintypes.error:
                    break

                if r == win32event.WAIT_FAILED:
                    break
                if r == win32event.WAIT_ABANDONED:
                    break

                try:
                    n, temp = win32file.ReadFile(hChildStdoutRd, 4096, None)
                except IndexError:
                    break
                except pywintypes.error:
                    break

                output.append(temp)
                if line_cb:
                    line_buff = line_buff + temp
                    while 1:
                        i = string.find(line_buff, "\n")
                        if i == -1:
                            break
                        i = i + 1
                        line_cb(line_buff[:i])
                        line_buff = line_buff[i:]


            ## get the exit status of the command
            try:
                win32event.WaitForSingleObject(hProcess, win32event.INFINITE)
            except pywintypes.error:
                pass
            
            while 1:
                status = win32process.GetExitCodeProcess(hProcess)
                if status != 259:
                    break
                time.sleep(0.1)

        finally:
            if not keep_lock:
                chaingang.chdir_lock.acquire()
                os.chdir(cwd)
            self.cleanup_tempdir()

        log.trace( 'exit' , [ status ] )
        return status, string.join(output,"")


BaseMacShellUtilities=ShellUtilities
if os.name == 'mac' and  os.environ.get("FAKE_MAC","no") == "YES":
    BaseMacShellUtilities=UNIXShellUtilities

class MacShellUtilities(BaseMacShellUtilities):
    def _platform_cp(self, source_path, destination_path):
        ## copy file data
        source = self._open(source_path, 'rb')
        destination = self._open(destination_path, 'wb')

        self._raw_copy(source, destination)
        source.close()
        destination.close()

        ## copy resource file data
        source = MacOS.openrf(source_path, '*rb')
        destination = MacOS.openrf(destination_path, '*wb')
        self._raw_copy(source, destination)
        source.close()
        destination.close()

        ## set creator/type on the Macintosh
        source_spec = macfs.FSSpec(source_path)
        (creator, type) = source_spec.GetCreatorType()
        destination_spec = macfs.FSSpec(destination_path)
        destination_spec.SetCreatorType(creator, type)

        ## copy file mode/time bits
        st = os.stat(source_path)
        mtime = st[stat.ST_MTIME]
        destination_spec.SetDates(mtime, mtime, mtime)

    def utime(path, atime, mtime):
        try:
            _mac_second_diff = 2082816000.0 
            fsp = macfs.FSSpec(self.FP(mac_path(filename)))
            mt=mtime + _mac_second_diff
            at=atime + _mac_second_diff
            fsp.SetDates(mt, mt, at)
        except MacOS.Error:
            raise error, "utime(%s,%d,%d) failed" %(path,atime,mtime)

    def chmod(path, mode): 
         pass

## instance of shell utilities based on platform
_shell = None

if os.name == "mac":
    log.debug('Using MacShell')
    _shell = MacShellUtilities()

elif os.name == "posix":
    log.debug('Using UNIXShell')
    _shell = UNIXShellUtilities()

elif os.name == "nt" and os.environ.get('OS') == 'Windows_NT':
    if win32api:
        log.debug('Using NTExtended')
        _shell = WinNTExtendedShellUtilities()
    else:
        log.debug('Using NTShell')
        _shell = WinNTShellUtilities()

elif os.name == "dos" or \
         string.find(os.environ.get("winbootdir", ""), "WINDOWS") >= 0:
    log.debug('Using 9xShell')
    _shell = Win9xShellUtilities()

else:
    log.error('Unsupported platform attempt by shell.py')
    raise error, "unsupported platform for shell.py"


## entry points

def move(path, to):
    try:
        os.rename(path, to)
    except OSError:
        cp(path, to)
        rm(path)

def rm(path):
    """Given a path, recursively remove it."""
    _shell.rm(path)

def cp(source_path, destination_path):
    """Copy a file from source_path to destination_path."""
    _shell.cp(source_path, destination_path)

def cpdir(source_path, destination_path):
    """Recursively copy a source directory to a destination directory."""
    _shell.cpdir(source_path, destination_path)

def find(path_list):
    """I can't remember what this is for."""
    return _shell.find(path_list)

def mkdir(path):
    """Make the directory for the given path, recursively if needed."""
    return _shell.mkdir(path)


def run(command,
        line_cb = None,
        timeout_seconds = 36000,
        dir = None,
        keep_lock=4711):
    """Run a command, capturing stdin/stdout.  Terminate the process if it
    runs longer than timeout_seconds.  Return (status, output) from the
    command.  Callback the line_cb function upon reading each line from
    the command if a callback is given."""

    # print "SHELL: Running %s in %s" % (command, dir)
                           
    if keep_lock == 4711:
        keep_lock=0
        cmd=string.lower(string.split(command," ",1)[0])
        if cmd[-4:]==".exe":
            cmd=cmd[:-4]
            
        if cmd == "isbuild":
            keep_lock=1

    return _shell.run(command, line_cb, timeout_seconds, dir, keep_lock)

def isthreadsafe():
    try:
        return _shell.thread_safe
    except AttributeError:
        return None

def Open(file, mode):
    return _shell._open(file, mode)

def Utime(file, atime, mtime):
    return _shell._utime(file, atime, mtime)

def Chmod(file, mode):
    return _shell._chmod(file, mode)
