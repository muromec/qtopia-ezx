# 
#  ***** BEGIN LICENSE BLOCK *****  
#   
#  Source last modified: $Id: compile.py,v 1.16 2006/07/20 20:41:06 jfinnecy Exp $ 
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
"""Sub-class of the Collate class from collate.py.  Implements a handler for
source code modules.  It runs Umake to generate a Makefile, then runs
make, nmake, or the generated AppleScript Makefile depending on the
platform."""

import os
import sys
import string
import traceback

import err
import ascript
import sysinfo
import shell
import outmsg
import time
import chaingang
import bldreg

from collate import Collate

try:
    import macfs
except:
    pass

class Compile(Collate):
    """Base compiler class, implements compile functionality common to
    all platforms."""

    ## Default stage unless "set_stage" has been called
    stage=0

    def is_builtin_compile_plugin(self):
        return 1
    
    def lcb(self, text):
        l = len(text)
        if l and text[l-1] == '\n':
            l = l - 1
        if l and text[l-1] == '\r':
            l = l - 1
        outmsg.verbose(text[:l])

    def run_cmd(self, cmd):
        outmsg.verbose(cmd)
        t = time.time()
        try:
            ret=shell.run(cmd, self.lcb)
        except shell.error, se:
            e = err.Error()
            e.Set(str(se))
            raise err.error, e

        outmsg.verbose("Time used: %.2f seconds" % (time.time() - t))
        return ret;

    def run(self):
        if not os.path.isfile("Umakefil") and \
           not os.path.isfile("umakefil"):
            self.error("Umakefil not found.")
        else:
            self.build()

    def set_stage(self,stage):
        self.stage=stage

    def build(self):
        ## Makefile Generation: run umake if there is no Makefile
        ## under the "no_umake_mode_flag",

        if (not os.path.isfile("Makefile") or \
            not self.settings.get("no_umake_mode_flag")) and \
            self.stage in [0,1]:
            
            self.umake()

        if chaingang.current_job.Get().refs > -1:
            outmsg.verbose("Missing BIF dependencies detected, will return to this module later.")
            return

        if not self.settings.get("umake_only_flag"):
            self.build_common()

    def amf(self):
        tmp=string.split(self.module.name,"/")
        tmp.append("makefile")
        return string.join(tmp,os.sep)
  

    def isstd(self):
        ## FIXME: This is an ugly hack because mac uses
        ## somewhat wonky makefiles
        if 'mac-pb' in sysinfo.family_list:
            return 0
        amfl=string.lower(self.amf())
        return bldreg.get("standard_makefile",amfl,0)


    def build_common(self):

        if self.stage in [0,1]:
            if self.settings.get('clean_mode_flag') and \
               not self.settings.get('no_make_depend_flag'):
                status, text = self.make_clean_depend()
            else:
                ## make clean
                ## under fast builds, we don't want to make clean
                ## don't do a make clean when doing a clean build
                if self.settings.get('clean_mode_flag'):
                    status, text = self.make_clean()
        
                ## make depend -- do not check status, it's okay
                ## if this command fails because makedepend is broken
                ## and too many developers can't figure it out
                if not self.settings.get('no_make_depend_flag'):
                    status, text = self.make_depend()
    

        ## Only do this when using threads
        if self.stage == 2 and self.isstd():
            self.make_objects()

        if self.stage in [0,3]:
            ## make all
            if self.isstd() and not self.settings.get('no_make_copy_flag'):
                status, text = self.make_copy()

                if status:
                    e = err.Error()
                    e.Set("Make failed.")
                    self.error(e.text)
                    raise err.error, e
            else:
                status, text = self.make_all()

                if status:
                    e = err.Error()
                    e.Set("Make failed.")
                    self.error(e.text)
                    raise err.error, e

                ## make copy
                if not self.settings.get('no_make_copy_flag'):
                    status, text = self.make_copy()

    def umake(self):
        self.output("generating makefiles")

        ## make a unique build_options list
        build_options = self.settings["build_options"][:]

        ## set static flag for build
        if self.module.build_static_flag:
            build_options.append("static")

        if self.module.build_static_only_flag:
            build_options.append("static_only")

        ## save current working directory because we don't
        ## know where we are going to be left after the call to Umake
        old_dir = os.getcwd()

        try:
            self.run_umake(build_options)
        finally:
           os.chdir(old_dir)

    def run_umake(self, build_options):
        ## attempt to clear the NS/Working space of Umake
        import umake
        umake.INIT()

        t = time.time()
        try:
            result = umake.Umake(build_options)
        except "NONSTANDARD":
            result=None
        except err.error, e:
            self.error(e.text)
            raise err.error, e
        except: 
            e = err.Error()
            e.Set("Error in umake call")
            e.SetTraceback(sys.exc_info())
            self.error(e.text)
            raise err.error, e

        if result and len(result):
            e = err.Error()
            e.Set("umake error=\"%s\"" % (result))
            raise err.error, e

        outmsg.verbose("Time used: %.2f seconds" % (time.time() - t))

    def make(self, arg = ""):
        self.error('make needs to be implemented in a subclass')

    def make_clean(self):
        ## don't do a make clean when doing a clobber build
        if self.settings.get('clobber_mode_flag'):
            return 0, ""

        self.output("making clean")
        (status, text) = self.make("clean")
        return status, text

    def make_depend(self):
        self.output("making depend")
        (status, text) = self.make("depend")
        return status, text

    def make_clean_depend(self):
        self.output("making clean & depend")
        (status, text) = self.make("clean depend")
        return status, text

    def make_all(self):
        self.output("making all")
        (status, text) = self.make("")
        return status, text

    def make_objects(self):
        self.output("making objects")
        (status, text) = self.make("all_objects")
        return status, text

    def make_copy(self):
        self.output("making copy")
        (status, text) = self.make("copy")
        return status, text


class UNIXCompile(Compile):
    """UNIX  version of the Compile class.  Runs GNU make."""

    def make(self, arg = ''):
        cmd = "make %s" % (arg)

        return self.run_cmd(cmd)

class CompileClient:
    def is_self(self):
        return 0

    def __init__(self):
        self.files = {}


class CompileClient_self(CompileClient):
    def is_self(self):
        return 1


class ClientManager:
    def __init__(self):
        self.load=0
        self.clients = []
        self.lock=chaingang.Lock()

        self.waitlist = [ [], [], [] ]
        self.localhost_available = chaingang.default_concurrancy
        if os.environ.get("HYPERTHREADING"):
            self.hyperthreading=1
        else:
            self.hyperthreading=0

        self.appropriator_running = 0

        import thread
        thread.start_new_thread(self.releaser, ())

    def appropriator(self):
        #print "APPROPRIATOR STARTING"
        import bfclient

        bfserver_host=os.environ["BFSERVER"]
        bfserver_port=6100
        tmp=string.split(bfserver_host,":",1)
        if len(tmp) > 1:
            bfserver_host=tmp[0]
            bfserver_port=int(tmp[1])

        user=os.environ.get("USERNAME",os.environ.get("USER","unknown"))
        host=os.environ.get("COMPUTERNAME",os.environ.get("HOST","unknown"))
        id="%s@%s" % (user, host)

        class Connection:
            def __init__(self, host, port):
                self.host=host
                self.port=port

            def connect_once(self):
                import bprotocol
                #print "CONNECT_ONCE: %s" % self.host
                import socket
                if self.host == "SELF":
                    return 1

                self.sock=bprotocol.Connect(self.host, self.port)
                if not self.sock:
                    return None

                message=bprotocol.Message("PREPARE")
                self.sock.SendMessage2(message)
                try:
                    msg=self.sock.WaitForMessage2()
                except bprotocol.error:
                    return None
                except socket.error:
                    return None

                return 1

            def connect(self):
                for x in range(0,20):
                    if self.connect_once():
                        return 1
                    time.sleep(0.5)
                return None


        while self.waitlist[1]:
            try:
                clientdata=bfclient.FindClient(bfserver_host,
                                               bfserver_port,
                                               id,
                                               sysinfo.id,
                                               Connection,
                                               connect_pause_seconds=2,
                                               accept_localhost=0,
                                               xpri = 0)
            except:
                import traceback
                traceback.print_exc()

            cc=CompileClient()
            cc.clientdata=clientdata
            cc.appropriated=time.time()
            
            self.lock.acquire()
            self.clients.append(cc)
            if self.waitlist[1]:
                self.waitlist[1].pop().release()
            self.lock.release()
        
        self.lock.acquire()
        self.appropriator_running=0
        self.lock.release()

    def get(self, accept_remote = 1, xlocal=0):
        #print "GET (%d)" % accept_remote
        wakeup=0
        while 1:
            self.lock.acquire()

            if self.localhost_available * 2 > chaingang.default_concurrancy * self.hyperthreading or xlocal:
                self.localhost_available = self.localhost_available - 1
                self.lock.release()
                return CompileClient_self()

            if accept_remote and len(self.clients):
                ret=self.clients.pop()
                self.load = self.load + 1
                self.lock.release()
                return ret

            if self.localhost_available >0:
                self.localhost_available = self.localhost_available - 1
                self.lock.release()
                return CompileClient_self()

            l=chaingang.Lock()
            l.acquire()
            self.waitlist[accept_remote]=[l]+self.waitlist[accept_remote]

            if accept_remote and not self.appropriator_running:
                import thread
                thread.start_new_thread(self.appropriator, ())
                self.appropriator_running=1

            self.lock.release()
            l.acquire()
            wakeup=1

    def releaser(self):
        import bprotocol
        #print "RELEASOR RUNNING"
        
        self.lock.acquire()
        avg=1.0
        while 1:
            self.lock.release()
            time.sleep(1)
            self.lock.acquire()
            load=self.load + len(self.waitlist[1])
            available_clients = self.load + len(self.clients)
            #print "AVG=%f LOAD=%d FREE=%d WAITING=%d+%d LOCALFREE=%d" % (avg, self.load, len(self.clients), len(self.waitlist[0]), len(self.waitlist[1]), self.localhost_available)
            avg=avg * 0.98 + load/50.0

            if self.clients:
                if available_clients > avg + 0.5 or \
                   time.time() - self.clients[0].appropriated > 60 * 10:
                    #print "RELEASING A MACHINE!"
                    c=self.clients[0]
                    self.clients=self.clients[1:]

                    try:
                        message=bprotocol.Message("DONE")
                        c.clientdata.client.sock.SendMessage2(message)
                    except:
                        import traceback
                        traceback.print_exc()

                    try:
                        c.clientdata.bfc.ReleaseBFMachine()
                    except:
                        import traceback
                        traceback.print_exc()
                    c=0
            
        self.lock.release()
            

    def release(self, x):
        #print "PUTTING MACHINE IN QUEUE"
        self.lock.acquire()

        if x.is_self():
            self.localhost_available = self.localhost_available +1
            if self.waitlist[0]:
                self.waitlist[0].pop().release()
            elif self.waitlist[1]:
                self.waitlist[1].pop().release()
        else:
            self.load = self.load - 1
            self.clients.append(x)
            if self.waitlist[1]:
                self.waitlist[1].pop().release()

        self.lock.release()

client_manager = None
    
class WinCompile(Compile):

    def __init__(self, module, settings):
        Compile.__init__(self, module, settings)
        self.copy_done=0

    def mtime(self, x):
        if self.mtime_cache.has_key(x):
            return self.mtime_cache[x]

        import umake_lib
        t=umake_lib.mtime(x)
        self.mtime_cache[x]=t
        return t


    def parse_makefile(self, basedir, mf_name, file_hash, src_root):
        #print "PARSE: %s" % mf_name

        remote_files=string.split(open(os.path.join(basedir, mf_name+"-mkdep.o"),"r").read(),"\n")

        def mkrel(path, basedir=basedir, src_root=src_root):
            x=os.path.normpath(os.path.join(basedir, path))
            #print x
            #print src_root
            if string.lower(x[:len(src_root)]) == string.lower(src_root):
                return x[len(src_root):]
            return None

        relpath=mkrel(basedir)

        ## FIXME
        to_do=1
	t=time.time()
        fix_makefile=0

        for ofile in remote_files:
            file=os.path.join(basedir, ofile)
            #print "FILE: %s" % file
            if os.path.isdir(file) and os.listdir(file):
                continue
            nfile=mkrel(ofile)
            if nfile:
                #print "HASH %s" % file
                file_hash[nfile]=1
                if not fix_makefile and os.path.isabs(ofile):
                   fix_makefile=1
                   print "FIXME: %s => %s" % (ofile, nfile)

        if fix_makefile:
           to=string.join([os.pardir] * len(string.split(relpath,os.sep)),os.sep) + os.sep
           outmsg.verbose("Trying to fix absolute paths in %s by replacing %s with %s" % (mf_name, src_root, to))
           mf=open(os.path.join(basedir, mf_name),"r").read()
           mf=string.replace(mf, src_root, to)
           open(os.path.join(basedir, mf_name),"w").write(mf)

	#print "Loop3, %f seconds" % (time.time() - t)

        return (to_do, src_root, relpath)

    def low_make_all(self):
        if self.isstd():
            ret=self.run_cmd("nmake /nologo copy")
            if not ret[0]:
                self.copy_done=1
            return ret
        else:
            return self.run_cmd("nmake /nologo")

    def make_objects2(self):
        self.output("making objects")
        (status, text) = self.make("all_objects")
        return status, text

    def compile_remotely(self, dir):
        global client_manager
        if not client_manager:
            client_manager=ClientManager()
    
        dir=os.getcwd()
        run_nmake=0
        cc=0
        chaingang.chdir_lock.release()
        unlocked=1
        try:
            ## Get a host to compile on
            import bfclient
            import bprotocol
            self.mtime_cache={}
    
            amf=self.amf()
            amfl=string.lower(amf)
            std=self.isstd()
    
            ## DEBUG
            #print "STD[%s]=%d" % (amf, std)
    
            cc=client_manager.get(std)
            if not cc.is_self():
                #print "GOT REMOTE"
                t1=time.time()
                client=cc.clientdata.client.sock
        
                file_hash = {}
        
                src_root=bldreg.get("build","path", None)

                src_root=os.path.normpath(os.path.join(dir, src_root))
                if src_root[-1] != os.sep:
                    src_root = src_root + os.sep
        
                to_do=0
                makefiles = [amf]
                for amf in makefiles:
                    file_hash[amf]=1
                    amfl=string.lower(amf)
                    alias=bldreg.get("alias",amfl,None)
                    if alias:
                        makefiles.extend(alias)
                    else:
                        path, file = os.path.split(amf)
                        (t, SR, relpath) = \
                                self.parse_makefile(os.path.join(src_root, path),
                                                    file, file_hash, src_root)
                        to_do=to_do+t
                t2=time.time()
                #print "Makefiles parsed in %f seconds" % (t2 - t1)
                if not to_do:
                    #print "NOTHING TO DO"
                    return (0,"")  ## Nothing to do
                else:
                    #print "Archiving files..."
                    archive_path=os.path.join(dir,"remotefiles.rna")
                    
                    new_list=[]
                    for f in file_hash.keys():
                        mt=self.mtime(os.path.join(dir, f))
                        if cc.files.get(f) != mt:
                            cc.files[f]=mt
                            new_list.append(f)

                    import archive
                    archive.RNA( archive_path, src_root, 0).Archive(new_list)

                    t3=time.time()
                    #print "Archive done in %f seconds, building " % (t3 - t2)
            
                    message=bprotocol.Message("BUILD")
                    message.SetAttrib("system_id",os.environ.get("SYSTEM_ID"))
                    message.SetAttrib("chdir", relpath)
                    message.SetAttrib("target", "make")
                    message.SetAttrib("cmd", "nmake /nologo /i SUBMAKEFLAGS=/i")
                    client.SendMessage2(message, open(archive_path,"rb"))
                    msg=client.WaitForMessage2()

                    status=int(msg.attrib["retcode"])

                    t4=time.time()

                    #print "Compile done in %f seconds, status=%d" % (t4-t3, status)            
                    message=bprotocol.Message("GET FILE")
                    message.SetAttrib("path","remote-result.rna")
                    client.SendMessage2(message)
                    msg=client.WaitForMessage2()
                    results=os.path.join(dir,"remote-result.rna")
                    client.RecieveFile(msg, open(results, "wb"))
            
                    client_manager.release(cc)
                    cc=0

                    ## Get a localhost lock for the rest of this compile
                    #if status != 0:
                    #    cc=client_manager.get(0,1)
                    cc=client_manager.get(0,1)

                    chaingang.chdir_lock.acquire()
                    unlocked=0
                    os.chdir(dir)

                    t5=time.time()
                    #print "Fetched file in %f seconds" % (t5-t4)

                    archive.RNA( results, src_root, 0, 0).Extract()

                    t6=time.time()
                    #print "Decompress done in %f seconds" % (t6-t5)

                    #if status == 0:
                    #    return (0,"")

                    return self.make_objects2()


            if cc and cc.is_self():
                #print "GOT SELF"
                if unlocked:
                    chaingang.chdir_lock.acquire()
                    unlocked=0
                    os.chdir(dir)
                #return self.low_make_all()
                return Compile.make_objects(self)

        finally:
            if cc:
               client_manager.release(cc)
            if unlocked:
               chaingang.chdir_lock.acquire()
               os.chdir(dir)

    def make_objects(self):
        if os.environ.get("BFSERVER"):
            try:
                ret=self.compile_remotely(dir)
                if ret:
                    return ret
            except:
                e = err.Error()
                e.Set("Error in remote compile (non-fatal)")
                e.SetTraceback(sys.exc_info())
                print
                print e.Text()
                    
        return Compile.make_objects(self)

    def make(self, arg = ""):
        if arg == "copy" and self.copy_done:
            return (0,"")

        cmd = "nmake /nologo %s" % arg
        return self.run_cmd(cmd)

class MacintoshCompile(Compile):
    """Macintosh version of the Compile class.  It creates a small AppleScript
    in memory to execute the all() function that exists in the Umake
    generated AppleScript Makefile."""

    def make_depend(self):
        return 0, ""

    def make_clean(self):
        return 0, ""

    def make_clean_depend(self):
        return 0, ""

    ## Kluge, do drmsigning here!!
    ## Extra kluge: Copied code from the drmsign command
    def make_copy(self):
        path = macfs.FSSpec('Makefile').as_pathname()
        dirs=[ os.path.dirname(path) ]
        for dir in dirs:
            for file in os.listdir(dir):
                file = os.path.join(dir, file)

                if os.path.isdir(file):
                    dirs.append(file)

                if file[-9:] == "--drmsign":
                    component_path = file[:-9]
                    import signclient

                    try:
                        drmsign_host = os.environ["DRMSIGN_HOST"]
                    except KeyError:
                        print "[ERROR] set DRMSIGN_HOST."
                        continue

                    try:
                        component = open(component_path, "rb").read()
                    except IOError:
                        print "[ERROR]: cannot read file=\"%s\"" % (component_path)
                        continue

                    sc = signclient.SignClient(drmsign_host, "", "")
                    result_status, result_component = sc.GetSignedComponent(component)

                    ## check status
                    if result_status != 0:
                        print "[ERROR]: exit status=\"%s\"" % (str(result_status))
                        continue


                    opath=os.path.join(self.settings.get("copy_path"),
                                 os.path.basename(component_path))
                    
                    ## write signed component
                    ## Make sure we do not zap resource fork!
                    try:
                        open(component_path, "wb").write(result_component)
                        open(opath, "wb").write(result_component)
                    except IOError:
                        print "[ERROR]: cannot write file=\"%s\"" % (component_path)
                        continue
                    

        return 0, ""

    def make(self, arg = ''):
        old_dir = os.getcwd()
        status = 0
        result = ''

        path = macfs.FSSpec('Makefile').as_pathname()

        ## ALL
        if arg == '':
            script = ascript.CreateAppleScript(
                'set errtext to ""',
                'set scriptobj to (load script file "%s")' % (path),
                'tell scriptobj',
                '  with timeout of 99999 seconds',
                '    run',
                '    set errtext to the result',
                '  end timeout',
                'end tell',
                'return errtext')

            result = script.CompileAndExecute()

        else:
            self.error('unsupported MAKE argument')

        ## so that it doesn't equal None
        if result == None:
            result = ''

        ## process output from AppleScript
        result = string.translate(result, string.maketrans('\r', '\n'))
        result = string.strip(result)
        result = string.replace(result,"\n\n","\n")

        ## strip off the stupid quotes
        if len(result) >= 1:
            if result[0] == '"' and result[-1] == '"':
                result = string.strip(result[1:-1])

        outmsg.verbose(result)

        ## Scan the output for error messages
        for line in string.split(result,"\n"):
            if not line:  ## empty line
                continue

            words=string.split(line)
            if words[1] == "Warning": ## Warning
                continue

            ## Anything else is an error
            status = 1
            self.error('failed make')

        os.chdir(old_dir)
        return status, result


class UNIXNoStopOnErrorCompile(Compile):
    """Just like the UNIXCompile class, except GNU make is launched with
    the "-k" argument to ignore errors.  This was put in so source code drops
    containing only objects, where the targets may not build, could be
    created and sent to companies.  The idea was (hopefully) someone would
    check to see that all the source files which are deleted before the
    code leaves the company would at least produce object files, and the
    company receiving the source/object drop could then use those objects
    to do their build."""

    def make(self, arg = ''):
        cmd = 'make -k %s' % (arg)
        return self.run_cmd(cmd)


class WinNoStopOnErrorCompile(WinCompile):
    """Same as UNIXNoStopOnErrorCompile, for Windows."""

    def make(self, arg = ''):
        cmd = 'nmake /nologo /k SUBMAKEFLAGS=/k %s' % (arg)
        return self.run_cmd(cmd)


def get_umake_backend():
    """Return the proper umake backend, this must of course match
    up with the proper collate class below"""

    if os.name == "mac":
        if os.environ.get('BUILD_ON_PLATFORM','') == 'MacOSX':
            generator = "umake_cwxml_ascript"  ## XML codewarrior
        else:
            generator = "umake_ascript"  ## Applescript codewarrior
    elif 'symbian-mmp' in sysinfo.family_list:
        generator = "umake_symbianmmp"
    elif 'win' in sysinfo.family_list or 'symbian-wins' in sysinfo.family_list:
        if 'win-vc7' in sysinfo.family_list:
            generator = "umake_win_vc7_makefile"
        elif 'win-vc8' in sysinfo.family_list:
            generator = "umake_win_vc8_makefile"
        else:
            generator = "umake_win_makefile"
    elif 'mac-pb' in sysinfo.family_list:
        generator = "umake_pb"  ## project builder
    else:
        generator = "umake_makefile"
            
    #print "generator = %s" % repr(generator)

    return generator
            
            
def get_collate_class():
    """Returns the proper Compiler class for the platform.  There's quite a
    bit of platform-specific crap in here."""
    
    if sysinfo.id == 'vxworks':
        return UNIXCompile
    elif sysinfo.id == 'procnto-2.00-i386':
        return UNIXNoStopOnErrorCompile
    elif sysinfo.id == 'linux-2.2-libc6-kerbango-powerpc':
        return UNIXNoStopOnErrorCompile
    elif sysinfo.id == 'ics-2.0-i386':
        return WinNoStopOnErrorCompile
    elif 'win' in sysinfo.family_list:
        return WinCompile
    elif os.name == 'posix':
        return UNIXCompile
    elif 'symbian-winscw' in sysinfo.family_list:
        return UNIXCompile
    elif os.name == 'nt' or os.name == 'dos':
        return WinCompile
    elif os.name == 'mac':
        if 'mac-pb' in sysinfo.family_list:
            return UNIXCompile
        return MacintoshCompile
