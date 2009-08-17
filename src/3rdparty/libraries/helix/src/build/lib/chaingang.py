import string
import os
import copy
#import inspect
#tracelog=open("trace.log","w")

import log
log.debug( 'Imported: $Id: chaingang.py,v 1.20 2006/06/19 23:11:27 jfinnecy Exp $' )

## Helper

class Heap:
    def __init__(self):
        self.data=[]
        #self.hpos={}

    def isgt(self, ind1, ind2):
        try:
            return self.data[ind1] > self.data[ind2]
        except TypeError:
            print self.data[ind1].dump()
            print self.data[ind2].dump()
            print "CMP: %s" % repr(self.data[ind1].__cmp__(self.data[ind2]))
            global count_subjob_cache
            print count_subjob_cache
            raise

    def __setitem__(self, pos, x):
        self.data[pos]=x
        #self.hpos[x]=pos

    def swap(self, x, y):
        (self[x], self[y]) = (self.data[y], self.data[x])

    def check_heap(self):
        try:
            for ind in range(0,len(self.data)):
                car=ind*2+1
                cdr=ind*2+2
                if self.data[car].count_subjobs()>self.data[ind].count_subjobs():
                    print "FEL FEL FEL (%d, %d)" % (car, ind)
                    break
                if self.data[cdr].count_subjobs()>self.data[ind].count_subjobs():
                    print "FEL FEL FEL (%d, %d)" % (cdr, ind)
                    break

            for ind in range(0,len(self.data)):
                print "Subjobs[%d] = %d" % (ind, self.data[ind].count_subjobs())

            if len(self.data):
                raise "FEL FEL FEL"

        except IndexError:
            pass

    def pushdown(self, ind):
        while 1:
            #print "PUSHDOWN: %d" % ind
            car=ind*2+1
            cdr=ind*2+2
            
            if cdr < len(self.data):
                if self.isgt(car, cdr):
                    n=car
                else:
                    n=cdr
            elif car < len(self.data):
                n=car
            else:
                return

            if not self.isgt(n, ind):
                return

            self.swap(ind, n)
            ind=n

    def pushup(self, ind):
        while ind:
            #print "PUSHUP: %d" % ind
            parent=(ind - 1)/2
            if not self.isgt(ind, parent):
                self.pushdown(ind)
                return

            self.swap(ind, parent)
            ind=parent

        self.pushdown(ind)

    def pop(self):
        #print "POP"
        ret=self.data[0]
        #del self.hpos[ret]
        new_top=self.data.pop()
        if self.data:
            self[0]=new_top
            self.pushdown(0)
        #self.check_heap()
        return ret

    def len(self):
        return len(self.data)

    def push(self, value):
        #print "PUSH %d" % (value.count_subjobs())
        l=len(self.data)
        self.data.append(value)
        #self.hpos[value]=l
        self.pushup(l)
        #self.check_heap()

    #def remove(self, value):
    #    if self.hpos.has_key(value):
    #        pos=self.hpos[value]
    #        del self.hpos[value]
    #        if pos == len(self.data) -1:
    #            self.data.pop()
    #        else:
    #            self[pos]=self.data.pop()
    #            self.pushup(pos)

class Tlocal_nothreads:
    def __init__(self):
        self.x=None

    def Set(self, x):
        self.x=x

    def Get(self):
        return self.x

def get_thread_ident_nothread():
    return 1

count_subjob_cache={}

class ChainGangJob:
    def __init__(self, name, fun, args, ordinal=0):
        self.name=name
        self.fun=fun
        self.args=args
        self.refs=-1
        self.subjobs=[]
        self.ordinal = ordinal
        self.done=0
        self.weight=1

    def set_weight(self, weight):
        global count_subjob_cache
        count_subjob_cache={}
        self.weight=weight

    def add_sub_job(self, job):
        global count_subjob_cache
        count_subjob_cache={}
        sjobs=[job]
        for j in sjobs:
            if j is self:
                raise "Recursion in add_sub_job %s >>> %s" %( self.name, job.name)
            sjobs.extend(j.subjobs)
        job.refs = job.refs + 1
        self.subjobs.append(job)

    def run(self, chaingang):
        global current_job
        self.refs=-1
        current_job.Set(self)
        apply(self.fun, self.args)
        current_job.Set(None)
        if self.refs == -1:
            chaingang.lock()
            self.done=1
            for job in self.subjobs:
                if job.refs:
                    job.refs = job.refs -1
                else:
                    chaingang.add_job(job)
            chaingang.unlock()

    def count_subjobs(self):
        global count_subjob_cache
        key=self.name
        ret=count_subjob_cache.get(self.name,-1)
        if ret == -1:
            ret=float(self.weight)
            for j in self.subjobs:
                ret=ret+j.count_subjobs()
            ret=ret / ((self.refs+1) or 1)
            count_subjob_cache[self.name]=ret
        return ret

    def dump(self):
        return  "ChaingangJob(name=%s, subjobs=%s, ordinal=%s)" % (
            repr(self.name),
            repr(self.count_subjobs()),
            repr(self.ordinal))

    def __cmp__(self, job):
        if job.name == self.name:
            return 0
        if self.count_subjobs() > job.count_subjobs():
            return 1
        if self.count_subjobs() < job.count_subjobs():
            return -1

        if self.ordinal > job.ordinal:
            return 1
        if self.ordinal < job.ordinal:
            return -1
        return 0

    def __deepcopy__(self, memo):
        ret=ChainGangJob(self.name,
                         self.fun,
                         self.args,
                         self.ordinal)
        ret.subjobs=copy.deepcopy(self.subjobs, memo)
        ret.refs=self.refs
        return ret

class ChainGang_nothreads:

    def __init__(self, todo=[], num=1):
        self.heap=Heap()
        self.error=0
        self.output_hash={}
        self.job_hash={}
        for job in todo:
            self.heap.push(job)

        t=todo[:]
        for job in t:
            if self.job_hash.has_key(job.name):
                continue
            self.job_hash[job.name]=job
            t.extend(job.subjobs)

        self.done=0

    def run(self):
        self.worker()

    def add_job(self, job):
        self.heap.push(job)

    def pop(self):
        if self.heap.len():
            self.done=self.done+1
            return self.heap.pop()
        return None

    def doit(self, todo):
        todo.run(self)

    def worker(self):
        global current_chaingang
        current_chaingang.Set(self)
        while not self.error:
            todo=self.pop()
            if not todo:
                break
            self.doit(todo)
        current_chaingang.Set(None)

    def lock(self):
        pass
    
    def unlock(self):
        pass

    def reschedule_by_id(self, id):
        ret=0
        global current_job
        cjob=current_job.Get()
        if not cjob:
            return 0
        job=self.job_hash.get(id)
        if job and not job.done:
            job.add_sub_job(cjob)
            ret = ret + 1
        return ret

    def reschedule_by_id2(self, id):
        ret=0
        global current_job
        cjobU=current_job.Get()
        if not cjobU:
            return 0
        cjobL=self.job_hash.get(string.replace(cjobU.name," umake",""))
        if not cjobL:
            return self.reschedule_by_id(id)

        jobL=self.job_hash.get(id)
        jobU=self.job_hash.get(id+" umake")
        if jobL and not jobL.done:
            jobL.add_sub_job(cjobL)

        if jobU and not jobU.done:
            jobU.add_sub_job(cjobU)
            ret = ret + 1
        
        return ret

    def dump_dependencies_on(self, id):
        ret=0
        global current_job
        cjob=current_job.Get()
        if not cjob:
            return 0
        job=self.job_hash.get(id)
        if job and not job.done:
            for subjob in cjob.subjobs:
                job.add_sub_job(subjob)
            ret=ret+1
        return ret

class Lock:
    def acquire(self, blocking=1):
        pass

    def release(self):
        pass


## Generic
default_concurrancy=1

if os.environ.has_key("RIBOSOME_THREADS"):
   default_concurrancy=int(os.environ["RIBOSOME_THREADS"])

## Windows
elif os.environ.has_key("NUMBER_OF_PROCESSORS"):
   default_concurrancy=int(os.environ["NUMBER_OF_PROCESSORS"])

## Linux
elif os.path.exists("/proc/cpuinfo"):
   default_concurrancy=len(string.split(open("/proc/cpuinfo").read(),"\n\n"))-1

## Solaris
elif os.path.exists("/usr/bin/mptstat"):
   default_concurrancy=len(os.popen("/usr/bin/mpstat","r").readlines())-1

ChainGang=ChainGang_nothreads
Tlocal=Tlocal_nothreads
get_thread_ident=get_thread_ident_nothread

have_threads = 0

try:
    import thread
    import sys
    import err
    import time
    import outmsg

    Lock = thread.allocate_lock

    have_threads = 1
    ## Threads lock up on HPUX for some reason -- disable
    if sys.platform == "hp-uxB":
        have_threads=0

    ## Threads lock up on FreeBSD 4 for some reason -- disable 
    if sys.platform == "freebsd4":
        have_threads=0

except ImportError:
    pass

except AttributeError:
    pass


if have_threads:

    class Tlocal_threads:
        def __init__(self):
            self.x={}

        def Set(self, x):
            self.x[thread.get_ident()]=x

        def Get(self):
            return self.x.get(thread.get_ident())

    Tlocal=Tlocal_threads
    get_thread_ident=thread.get_ident


    class FunWrapper:
        def __init__(self, hash, fun):
            self.hash=hash
            self.fun=fun
            
        def __call__(self, *args):
            l=self.hash.get(thread.get_ident(), 17)
            if l != 17 :
                l.append( ( self.fun, args ) )
            else:
                return apply(self.fun, args)
                
        def remove( self , dummy ):
            # The build system is apparently pushing an object of FunWrapper
            # class onto a file list somewhere. That file list is then being
            # iterated over with the .remove() method being called.
            # This is causing the build to throw an exception and hang the
            # threading system, and hang the build client machine.
            #
            # Initial guess is that this is an output wrapper that is defunct
            # or was never being used. Recent changes to the logging system
            # have made the build clients start to write to stdout, and so
            # this wrapper has come into play. Why / where it's getting pushed 
            # onto a file list is unknown.
            #
            # The exact call appears to be in output.py
            #     self.fil_list.remove(rm_fil)
            #
            # The above data is from very preliminary observation.
            #
            # While I track this sucker down, I am putting this stub here to
            # prevent client hangs.
            pass
        
    class OutputWrapper:
        def __init__(self, hash, file):
            self.hash=hash
            self.file=file

        def __getattr__(self, name):
            fun=getattr(self.file, name)
            return FunWrapper(self.hash, fun)

## This implements a way of parallelizing function calls

    class ChainGang_threaded(ChainGang_nothreads):
        def __init__(self, todo=[], num=default_concurrancy ):
            log.info( "Enabling threading, %s threads requested." % num )
            self.debug=os.environ.get("CHAINGANG_DEBUG")
            ChainGang_nothreads.__init__(self, todo)
            self.queue_lock=thread.allocate_lock()
            self.genlock=thread.allocate_lock()
            self.write_lock=thread.allocate_lock()
            self.num_threads=num
            self.running_threads=0
            self.output_hash={}
            self.old_stdout=0
            self.traces={}

        def start_jobs(self):
            if self.running_threads:
                N=1
            else:
                N=0

            log.debug( 'Job request heap length: %s' % self.heap.len() )
            while ( self.running_threads < self.num_threads and
                    N < self.heap.len()):

                log.debug( 'Requesting additional thread.' )    
                thread.start_new_thread(self.worker, ())
                self.running_threads = self.running_threads + 1
                log.debug( 'Total running threads: %s' % self.running_threads )
                N=N+1           

        def trace(self, frame, event, arg):
            #fn, l, fun, co, ind = inspect.getframeinfo(frame)
            #tracelog.write(" %d: %s @ %s:%d\n" % (thread.get_ident(), fun, fn, l))
            #tracelog.flush()
            self.traces[thread.get_ident()]=( frame, event, arg )

        def run(self):
            log.trace( 'entry' )
            log.info( 'Starting thread scheduler to process jobs.' )
            self.old_stdout = sys.stdout
            self.init_profile()

            self.cwd=os.getcwd()
            chdir_lock.release()
            ql=0
            try:
               if not self.debug:
                   sys.stdout = OutputWrapper(self.output_hash,
                                              sys.stdout)
               self.queue_lock.acquire()
               ql=1
               self.start_jobs()
              
               ## UGLY
               last_done=self.done
               last_change=time.time()
               log.debug( 'Entering thread-watcher loop.' )
               while self.running_threads:                  
                  self.queue_lock.release()
                  ql=0
                  time.sleep(0.3)
                  self.queue_lock.acquire()
                  ql=1
                  if last_done != self.done:
                     last_change=time.time()
                     last_done=self.done
                  elif time.time() - last_change > 900:
                     log.debug( 'Stale thread detected.' )
                     last_change=time.time() + 1800
                     try:
                         log.info( self.status() )
                     except:
                         log.error( 'Thread system failure - no report on potentially locked thread.' )

                  elif time.time() > self.last_profile:
                     self.last_profile=self.last_profile + 1/3.0
                     try:
                         self.run_profile()
                     except:
                         import traceback
                         traceback.print_exc()
                  
            finally:
                log.debug( 'Left thread-watcher loop.' )
                if ql:
                    self.queue_lock.release()
                    ql=0

                sys.stdout = self.old_stdout
                chdir_lock.acquire()
                os.chdir(self.cwd)
               
            if self.error:
                log.error( 'Error during job threading: %s' % self.error )
                raise err.error, self.error

            log.info( 'Thread scheduler run completed.' )
            log.trace( 'exit' )

        def write(self, text):
            if 0:
                f=open("c:\dev\TH%d" % thread.get_ident(), "a")
                f.write(
                    "== THREAD %d/%d(%d) jobs=%d ==\n%s\n====================\n" %
                    ( thread.get_ident(),
                      self.running_threads,
                      len(self.output_hash),
                      len(self.data),
                      text ));

            self.output_hash[thread.get_ident()].append( (self.old_stdout.write, text ))

        def lock(self):
            self.genlock.acquire()
    
        def unlock(self):
            self.genlock.release()

        def add_job(self, job):
            self.queue_lock.acquire()
            ChainGang_nothreads.add_job(self, job)
            self.start_jobs()
            self.queue_lock.release()

        def reschedule_by_id(self, id):
            self.queue_lock.acquire()
            self.lock()
            ret=ChainGang_nothreads.reschedule_by_id(self, id)
            self.unlock()
            self.queue_lock.release()
            return ret

        def dump_dependencies_on(self, id):
            self.queue_lock.acquire()
            self.lock()
            ret=ChainGang_nothreads.dump_dependencies_on(self, id)
            self.unlock()
            self.queue_lock.release()
            return ret

        def pop(self):
            self.queue_lock.acquire()
            ret=ChainGang_nothreads.pop(self)
            #if ret:
            #    def FOO(x):
            #        return x.count_subjobs()
            #    print "SUBJOBS: %d (%s)" % (
            #        ret.count_subjobs(),
            #        repr( map(FOO,self.data)  )
            #        )
                
            self.queue_lock.release()
            return ret


        def flush_buffer(self):
            ## Write the output of this invocation to stdout
            
            self.write_lock.acquire()
            try:
                id=thread.get_ident()
                output = self.output_hash[id]
                self.output_hash[id]=[]
                for (fun, args) in output:
                    apply(fun, args)
            finally:
                self.write_lock.release()

        def doit(self, todo):
            chdir_lock.acquire()
            os.chdir(self.cwd)
            try:
                #self.old_stdout.write("Processing %s\n" % repr(todo))
                ChainGang_nothreads.doit(self, todo)
                self.flush_buffer()
            finally:
                chdir_lock.release()

        def init_profile(self):
            if os.environ.get("CHAINGANG_PROFILE"):
               self.last_profile=time.time()
               self.last_profile_marker=time.time()
               self.profile_log=open("chaingaing_profile.html","w")
               self.thread_columns={"---":0}
               self.last_profile_line="<table cellpadding=0 cellspacing=0>\n"
               self.last_profile_line_time=time.time()
               self.rows=1
               self.free_columns=[]
               self.action_colors={
                  "make_objects":"#44ff44",
                  "make_objects2":"#888800",
                  "make_all":"#ffff00",
                  "low_make_all":"#00ff00",
                  "make_copy":"#0000ff",
                  "make_clean_depend":"#00ffff",
                  "make_depend":"#0088ff",
                  "make_clean":"#00ff88",
                  "run_umake":"#ff0000",
                  "compile_remotely":"#008800",
                  "get":"#880088",
               }
            else:
               self.last_profile=time.time()+ 86400 * 365 * 10 # 10 years

        def run_profile(self):
            log.trace( 'entry' )
            global current_job
            import inspect

            height=int( (time.time() - self.last_profile_line_time) * 3 )
            if not height:
                return

            x={}
            defelement=("#ffffff","")
            ret=[ defelement ] * (len(self.thread_columns) + len(self.free_columns))

            done={"---":1}

            frame=0
            for (tid, (frame, event, arg)) in self.traces.items():
               current_task="-"
               cjob=current_job.x.get(tid,None)
               if cjob:
                   current_task=cjob.name
               done[tid]=1
               if self.thread_columns.get(tid):
                   ret[self.thread_columns.get(tid)]=("#000000","Unknown")

               for frame, filename, line, function_name, context, index in inspect.getouterframes(frame):
                   if filename[-10:]!="compile.py":
                       continue
                   color=self.action_colors.get(function_name)
                   if not color:
                       continue

                   column=self.thread_columns.get(tid, -1)
                   if column == -1:
                       if self.free_columns:
                           column=self.free_columns.pop()
                       else:
                           column=len(self.thread_columns)
                           ret.append(defelement)

                       self.thread_columns[tid]=column
                   ret[column]=(color, current_task +" "+function_name)
                   break


            del frame
            l=self.heap.len()

            c=["#000000","#004400","#008800","#00cc00","#00ff00","#44ff00","#88ff00","#ccff00","#ffff00","#ffcc00","#ff8800","#ff4400","#ff0000"]
            if l >= len(c):
               l=len(c)-1
            color=c[l]
            ret[0]=(color, "%d tasks left" % self.heap.len() )

            SEP="<td bgcolor=\"#ffffff\"><spacer type=block width=5 height=1></td>"
            out=[SEP]

            for c in ret:
                out.append("<td bgcolor=%s><spacer type=block width=20 height=HEIGHT title=\"%s\"></td>" % (c))
                out.append(SEP)
            out=string.join(out,"")

            if time.time() - self.last_profile_marker >= 60:
                if height > 1:
                    tmp=string.replace(self.last_profile_line,"HEIGHT",str( height - 1 ))
                    self.profile_log.write("<tr>%s</tr>\n" % tmp)
                    
                    self.last_profile_line_time = (height -1) /3.0

                height=1
                tmp=string.replace(self.last_profile_line,"#ffffff","#000000")
                self.last_profile_line=tmp
                self.last_profile_marker = self.last_profile_marker + 60
    
            if out != self.last_profile_line:
                tmp=string.replace(self.last_profile_line,"HEIGHT",str( height ))
                tmp=string.replace(tmp,"SEP",SEP)
                self.profile_log.write("<tr>%s</tr>\n" % tmp)
                self.profile_log.flush()
                self.last_profile_line=out
                self.last_profile_line_time=self.last_profile_line_time + height/3.0

            for tid in self.thread_columns.keys():
                if done.has_key(tid):
                    continue
                self.free_columns.append(self.thread_columns[tid])
                del self.thread_columns[tid]

            log.trace( 'exit' )

        def status(self):
            import inspect
        
            status = "Available threads:\n"
        
            for (tid, (frame, event, arg)) in self.traces.items():
                status = status + " =========== %s ==========\n" % repr(tid)
        
                if frame:
                    status = status + "    stack:\n"
                    for frame, filename, line, function_name, context, index in inspect.getouterframes(frame):
                       status = status + "      " + function_name + " @ " + filename + " # " + str(line) + "\n"
    
                status = status + "\n"
     
            return status

        
        def worker(self):
            log.trace( 'entry' )
            ## Uncomment this for deadlock debugging!!
            sys.settrace(self.trace)
            
            self.output_hash[thread.get_ident()]=[]
            try: # release lock
                try: # error handler
                    ChainGang_nothreads.worker(self)

                except err.error, e:
                    print "***Thread error trapped!"
                    e.SetTraceback(sys.exc_info())
                    self.error=e
                    print e.Text()
                except:
                    print "***Thread error trapped!"
                    e = err.Error()
                    e.Set("Error in threaded call")
                    e.SetTraceback(sys.exc_info())
                    self.error=e
                    print e.Text()
            finally:
                self.flush_buffer()
                del self.output_hash[thread.get_ident()]

                self.queue_lock.acquire()
                self.running_threads = self.running_threads - 1
                self.queue_lock.release()

            sys.settrace(None)
            if self.traces.has_key(thread.get_ident()):
                del self.traces[thread.get_ident()]

            log.trace( 'exit' )

    def ChainGang(todo, num=default_concurrancy):
        if num>1:
            return ChainGang_threaded(todo, num)
        return ChainGang_nothreads(todo, num)

    class Pool(Heap):
        def __init__(self):
            Heap.__init__(self)
            self.lock=Lock()
            self.waitlist=[]

        def acquire(self, wait = 1):
            while 1:
                self.lock.acquire()
                if not wait or self.len():
                    ret=self.pop()
                    self.lock.release()
                    return ret
                l=Lock()
                l.acquire()
                self.waitlist.append(l)
                self.lock.release()
                l.acquire()

        def release(self, x):
            self.lock.acquire()
            self.push(x)
            if self.waitlist:
                self.waitlist.pop().release()
            self.lock.release()


## Recursive lock class
class Rlock:
    def __init__(self):
        global Lock
        global threadid
        self.l1=Lock()
        self.l2=Lock()
        self.id=None
        self.count=0

    def acquire(self):
        global get_thread_ident
        id=get_thread_ident()
        self.l1.acquire()

        if self.id != id:
            self.l1.release()
            self.l2.acquire()
            self.l1.acquire()
            self.id=id
            self.count=0
            
        self.count = self.count + 1
        self.l1.release()

    def release(self):
        global get_thread_ident
        id=get_thread_ident()
        self.l1.acquire()
        if self.id == id:
            self.count = self.count - 1
            if self.count == 0:
                self.l2.release()
                self.id=None
        else:
            raise "Release while not owned!"
        self.l1.release()


chdir_lock=Rlock()
chdir_lock.acquire()
createprocess_lock=Lock()


def ProcessModules_anyorder(modules, func, num=default_concurrancy):
    jobs=[]
    for m in modules:
        jobs.append(ChainGangJob(m.name, func, (m,)))

    ChainGang( jobs, num ).run()

def nulljob():
    #print "NULLJOB"
    pass

class JobTmp:
    def __init__(self, first, last = None):
        self.first=first
        self.last=last or first

def compute_module_dependencies(modules, func):
    dirhash={}
    jobtmp={}

    jobtmp[""]=JobTmp(ChainGangJob("NULL",nulljob, ()))

    for m in modules:
        job=ChainGangJob(m.name,func, (m,))
        if jobtmp.has_key(m.name):
            print "Warning, two modules with name=%s" % m.name
            jobtmp[m.name].last.add_sub_job(job)
            jobtmp[m.name].last=job
        else:
            jobtmp[m.name]=JobTmp(job)

    module_names=jobtmp.keys()
    module_names.sort()

    for module_name in module_names:
        name=module_name
        if not name:
            continue
        #print "MODULE: %s" % repr(name)

        while 1:
            pos = string.rfind(name,"/")
            if pos == -1:
                #print "  >>> standalone"
                name=""
            else:
                name = name[:pos]

            #print "?:: %s" % repr(name)

            if jobtmp.has_key(name):
                #print "  >>> submodule"
                jobtmp[name].last.add_sub_job(jobtmp[module_name].first)
                break

    tmp=ChainGang_nothreads([], 1)
    #print jobtmp[""].first.subjobs
    jobtmp[""].first.run(tmp)
    return tmp.heap.data


current_job=Tlocal()
current_chaingang=Tlocal()

##
##
##
def ProcessModules(modules, func, num=default_concurrancy):
    jobs=compute_module_dependencies(modules, func)

    ChainGang( jobs, num ).run()


def print_jobs(jobs):
    toprint=jobs[:]
    done={}
    print "============================================="
    for job in toprint:
        n=job.name
        if done.get(n):
            continue
        done[n]=1
        deps=[]
        for sj in job.subjobs:
            toprint.append(sj)
            deps.append(sj.name)
        print "%s: %s" % (n, string.join(deps))
    print "============================================="

##
##
##
def ProcessModules_grouped(modules, func,
                           num=default_concurrancy,
                           group_size=-1):

    log.trace( 'entry' )
    ## Smaller groups with more threads for better
    if group_size == -1:
        import math
        group_size = int(50*math.log(2)/math.log(1.0+num))

        g2=len(modules)/num/2
        if g2 < group_size:
            group_size=g2

        if group_size < 1:
            group_size=1

    print "Group size: %d  Modules: %d  Threads: %d" % (group_size, len(modules), num)
        
    jobs=compute_module_dependencies(modules, func)

    ## Make all job use arrays
    tmp=jobs[:]
    for j in tmp:
        j.args = ([ j.args[0] ], ) 
        tmp.extend(j.subjobs)

    ## Group top-level jobs together
    newjobs=[]
    groups={}
    for j in jobs:
        m=j.args[0][0]
        if m.cvs_path:
            newjobs.append(j)
        else:
            key=repr ( (m.type,
                        m.cvs_root,
                        m.cvs_tag,
                        m.cvs_tag_type,
                        m.cvs_date) )
            
            if groups.has_key(key):
                nj=groups[key]
                nj.args = (nj.args[0] + j.args[0], )
                nj.subjobs.extend(j.subjobs)
            else:
                groups[key]=j
                nj=j

            if len(nj.args[0]) >= group_size:
                newjobs.append(nj)
                del groups[key]

    newjobs.extend(groups.values())
    for job in newjobs:
        job.ordinal=len(job.args[0])

    ChainGang( newjobs, num ).run()
    log.trace( 'exit' )
