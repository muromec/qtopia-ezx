/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: sysinfo.cpp,v 1.16 2005/08/23 23:37:46 richardjones Exp $ 
 *   
 * Portions Copyright (c) 1995-2003 RealNetworks, Inc. All Rights Reserved.  
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

#include "hlxclib/stdio.h"
#include "hlxclib/stdlib.h"
#include "hlxclib/time.h"
#include "hlxclib/string.h"

#include "hxtypes.h"
#include "platform_config.h"
#include "hxcom.h"
#include "hxerror.h"
#include "hxassert.h"
#include "server_version.h"

#ifdef _WINDOWS
#include "hlxclib/sys/socket.h"
#else
#include <sys/time.h>
#endif

#include "sysinfo.h"

#ifdef _UNIX
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

#if !defined(_HPUX) && !defined(_AIX) && !defined(_FREEBSD) && !defined(_OPENBSD) && !defined(_NETBSD) && !defined(_MAC_UNIX)
#include <sys/procfs.h>
#endif

#define SYSINFO_MAX_THREADS 512
#define SYSINFO_MAX_PROCS   128

extern int g_GlobalProcessList[];
extern char* g_GlobalProcListFiles[];
#endif


extern UINT32* g_pCPUCount;


#if defined(_FREEBSD) || defined(_OPENBSD) || defined(_NETBSD)
#include <sys/sysctl.h>
#include <sys/dkstat.h>
#endif
#if defined(_NETBSD)
#include <sys/sched.h>
#endif

#ifdef _UNIX
static inline UINT32
ComputeTimeDelta(struct timeval* pStartTime, struct timeval* pEndTime);
#endif


/***********************************************************************
 * CSYSINFO
 */
CSysInfo::CSysInfo():
    m_ulLastUserCPUUsage(0),
    m_ulLastKernCPUUsage(0),
    m_ulLastAggCPUUsage(0)
{
}


CSysInfo::~CSysInfo()
{
}


UINT32
CSysInfo::GetNumberofCPU (void)
{
    HX_ASSERT(0);
    return 1;
}


HX_RESULT
CSysInfo::InitCPUCalc (void)
{
    InitProcUsage();
    GetTotalCPUUsage(&m_ulLastAggCPUUsage);
    return HXR_OK;
}


#ifdef _UNIX
HX_RESULT
CSysInfo::InitProcUsage (void)
{
    struct timezone tz;
    
    m_ulLastUserCPUUsage = 0;
    m_ulLastKernCPUUsage = 0;
    m_ulLastAggCPUUsage = 0;
    memset(&m_StartTime, 0, sizeof(struct timeval));
    gettimeofday(&m_StartTime, &tz);
    GetTotalProcUsage(&m_ulLastUserCPUUsage, &m_ulLastKernCPUUsage);

    return HXR_OK;
}


HX_RESULT
CSysInfo::GetCPUUsage (REF(INT32) lUserUsage, REF(INT32) lKernUsage, REF(INT32) lAggUsage)
{
    struct timeval  EndTime;
    struct timezone tz;
    UINT32 ulDuration = 0;
    UINT32 ulUserUsedTime = 0;
    UINT32 ulKernUsedTime = 0;
    UINT32 ulCumulativeUsedTime = 0;

    if (gettimeofday(&EndTime, &tz) == 0)
    {
        // Compute total duration in microseconds
        ulDuration = ComputeTimeDelta(&m_StartTime, &EndTime);
        memcpy(&m_StartTime, &EndTime, sizeof(struct timeval));
    }

    GetTotalProcUsage(&ulUserUsedTime, &ulKernUsedTime);

    lUserUsage = (ulUserUsedTime - m_ulLastUserCPUUsage) / (ulDuration * (*g_pCPUCount));
    m_ulLastUserCPUUsage = ulUserUsedTime;

    lKernUsage = (ulKernUsedTime - m_ulLastKernCPUUsage) / (ulDuration * (*g_pCPUCount));
    m_ulLastKernCPUUsage = ulKernUsedTime;


    GetTotalCPUUsage(&ulCumulativeUsedTime);
    lAggUsage = (ulCumulativeUsedTime - m_ulLastAggCPUUsage) / (ulDuration * (*g_pCPUCount));
#ifdef _LINUX
    //see CLinuxSysInfo::GetTotalCPUUsage for why this is backwards on Linux:
    lAggUsage = 100 - lAggUsage;
#endif
    m_ulLastAggCPUUsage = ulCumulativeUsedTime;

    return HXR_OK;
}


HX_RESULT
CSysInfo::GetTotalProcUsage (UINT32* plUserUsage, UINT32* plKernUsage)
{
    *plUserUsage = 0;
    *plKernUsage = 0;

    for (UINT32 i=0; i < SYSINFO_MAX_THREADS; i++) 
    {
        if (g_GlobalProcessList[i] == 0)
        {
            break;
        }

        UINT32 ulUserUsage = 0;
        UINT32 ulKernUsage = 0;
        GetProcUsageById(i, &ulUserUsage, &ulKernUsage);
        *plUserUsage += ulUserUsage;
        *plKernUsage += ulKernUsage;
    }

    return HXR_OK;
}
#else

HX_RESULT
CSysInfo::InitProcUsage (void)
{
    HX_ASSERT(0);
    return HXR_NOTIMPL;
}

HX_RESULT
CSysInfo::GetCPUUsage (REF(INT32) lUserUsage, REF(INT32) lKernUsage, REF(INT32) lAggUsage)
{
    HX_ASSERT(0);
    return HXR_NOTIMPL;
}

HX_RESULT
CSysInfo::GetTotalProcUsage (UINT32* plUserUsage, UINT32* plKernUsage)
{
    HX_ASSERT(0);
    return HXR_NOTIMPL;
}
#endif


HX_RESULT
CSysInfo::GetMemUsage (REF(UINT32) ulMemUsage)
{
    return HXR_NOTIMPL;
}


HX_RESULT
CSysInfo::GetProcUsageById(UINT32 id, UINT32* plUserUsage, UINT32* plKernUsage)
{
    HX_ASSERT(0);
    return HXR_NOTIMPL;
}


HX_RESULT
CSysInfo::GetTotalCPUUsage (UINT32* plUsage)
{
    HX_ASSERT(0);
    return HXR_NOTIMPL;
}


char*
CSysInfo::SkipToken (const char* p)
{
    while (isspace(*p)) 
    {
        p++;
    }
    while (*p && !isspace(*p)) 
    {
        p++;
    }
    return (char*)p;
}


/***********************************************************************
 * LINUX
 */
#if defined(_LINUX)

CLinuxSysInfo::CLinuxSysInfo()
{
}

CLinuxSysInfo::~CLinuxSysInfo()
{
}

UINT32
CLinuxSysInfo::GetNumberofCPU(void)
{
    int nNumProc = sysconf(_SC_NPROCESSORS_ONLN);
    if (nNumProc < 1)
    {
        HX_ASSERT(0); //Shouldn't happen!
        nNumProc = 1;
    }
    
    return nNumProc;
}


#ifdef PTHREADS_SUPPORTED
// On Linux with PTHREADS_SUPPORTED, we get the user and system
// time from the /proc/{pid}/task/{tid...}/stat files. 
// The values are in ticks.
#include <dirent.h>
HX_RESULT
CLinuxSysInfo::GetTotalProcUsage (UINT32* plUserUsage, UINT32* plKernUsage)
{
    int fd = 0;
    UINT32 i = 0;
    char szbuff[4096 + 1];
    char szDirName[64];
    char szFileName[64];
    char* pszbuff = NULL;
    struct dirent* pEntry;
  
    DIR* pDir = 0;
    sprintf(szDirName, "/proc/%d/task", getpid());

    if ((pDir = opendir(szDirName)) != NULL)
    {
        while ((pEntry = readdir(pDir)) != NULL)
        {
            //if (strcmp(pEntry->d_name, ".") == 0)
            if ((strcmp(pEntry->d_name, ".") == 0) || (strcmp(pEntry->d_name, "..") == 0))
            {
                continue;
            }
            // We purposly allow ".." here because this is a simple
            // way to also open /proc/{pid}/stat in additon to
            // /proc/{pid}/task/*/stat

            sprintf(szFileName, "%s/%s/stat", szDirName, pEntry->d_name);
            fd = open(szFileName, O_RDONLY);
            if (fd == -1)
            {
                continue;
            }

            UINT32 len=0;
            len = read(fd, szbuff, 4096);
            close(fd);
            szbuff[len]='\0';
            pszbuff = szbuff;
            for (i=0; i < 13; i++) 
            {
                char* p = SkipToken(pszbuff);
                pszbuff = p;
            }

            UINT32 lUser = strtoul(pszbuff, &pszbuff, 0);
            UINT32 lKern = strtoul(pszbuff, &pszbuff, 0);
            *plUserUsage += lUser;
            *plKernUsage += lKern;
        }
        closedir(pDir);
        return HXR_OK;
    }

    *plUserUsage = 0;
    *plKernUsage = 0;

    return HXR_FAIL;
}


HX_RESULT
CLinuxSysInfo::GetProcUsageById(UINT32 id, UINT32* plUserUsage, UINT32* plKernUsage)
{
    HX_ASSERT(0);
    return HXR_NOTIMPL;
}



#else //old LinuxThreads threading model

HX_RESULT
CLinuxSysInfo::GetTotalProcUsage (UINT32* plUserUsage, UINT32* plKernUsage)
{
    return CSysInfo::GetTotalProcUsage(plUserUsage, plKernUsage);
}


// On Linux, we get the user and system time from /proc/$pid/stat file in ticks
HX_RESULT
CLinuxSysInfo::GetProcUsageById(UINT32 id, UINT32* plUserUsage, UINT32* plKernUsage)
{
    int fd = 0;
    UINT32 i = 0;
    char szbuff[4096 + 1];
    char* pszbuff = NULL;
  
    if ((fd = open(g_GlobalProcListFiles[id], O_RDONLY)) != -1) 
    {
        UINT32 len=0;
        len = read(fd, szbuff, 4096);
        close(fd);
        szbuff[len] = '\0';
        pszbuff = szbuff;
        for (i=0; i < 13; i++) 
        {
            char* p = SkipToken(pszbuff);
            pszbuff = p;
        }

        *plUserUsage += strtoul(pszbuff, &pszbuff, 0);
        *plKernUsage += strtoul(pszbuff, &pszbuff, 0);

        return HXR_OK;
    }
    else 
    {
        *plUserUsage = 0;
        *plKernUsage = 0;
        return HXR_FAIL;
    }
}
#endif

HX_RESULT
CLinuxSysInfo::GetTotalCPUUsage(UINT32* pulUsage)
{
    int fd = 0;
    char szbuff[4097]; 
    char* pszbuff = NULL;
    
    if ((fd = open("/proc/stat", O_RDONLY)) != -1)
    {
        UINT32 len = 0;
        len = read(fd, szbuff, 4096);
        close(fd);
        szbuff[len] = '\0';
        pszbuff = SkipToken(szbuff);
        UINT32 lUser = strtoul(pszbuff, &pszbuff, 0); // User mode time in ticks
        UINT32 lNice = strtoul(pszbuff, &pszbuff, 0); // Nice mode time in ticks
        UINT32 lKern = strtoul(pszbuff, &pszbuff, 0); // System mode time in ticks
        UINT32 lIdle = strtoul(pszbuff, &pszbuff, 0); // Idle time in ticks

        //XXXDC testing has shown that user+nice+kern is not accurate with
        //current kernels, at least with RHEL4-based kernels.
        //So... we have to return the idle time and reverse the logic
        //in the calling routine to determine the total system busy time
        //*pulUsage += lUser + lNice + lKern;
        *pulUsage += lIdle;

        return HXR_OK;
    }
    return HXR_FAIL;
}


/***********************************************************************
 * BSD
 */
#elif defined(_FREEBSD) || defined(_OPENBSD) || defined(_NETBSD)

CBSDSysInfo::CBSDSysInfo()
{
}

CBSDSysInfo::~CBSDSysInfo()
{
}

UINT32
CBSDSysInfo::GetNumberofCPU(void)
{
    int mib[2];
    int nNumProc;
    size_t nLen;

    mib[0] = CTL_HW;
    mib[1] = HW_NCPU;
    nLen = sizeof(nNumProc);
    sysctl(mib, 2, &nNumProc, &nLen, NULL, 0);

    if (nNumProc < 1)
    {
        HX_ASSERT(0); //Shouldn't happen!
        nNumProc = 1;
    }
    return nNumProc;
}


HX_RESULT
CBsdSysInfo::GetTotalProcUsage (UINT32* plUserUsage, UINT32* plKernUsage)
{
    for (UINT32 i=0; i < SYSINFO_MAX_THREADS; i++) 
    {
        UINT32 ulUserUsage = 0;
        UINT32 ulKernUsage = 0;
        GetProcUsageById(i, &ulUserUsage, &ulKernUsage);
        *plUserUsage += ulUserUsage;
        *plKernUsage += ulKernUsage;
    }
}

// On FreeBSD, we get the user and system time from /proc/$pid/status file in secs and microsecs.
HX_RESULT
CBsdSysInfo::GetProcUsageById (UINT32 id, UINT32* plUserUsage, UINT32* plKernUsage)
{
    int fd = 0;
    UINT32 i = 0;
    char szbuff[4096 + 1];
    char* pszbuff = NULL;
  
    if ((fd = open(g_GlobalProcListFiles[id], O_RDONLY)) != -1) 
    {
        UINT32 len=0;
        len = read(fd, szbuff, 4096);
        close(fd);
        szbuff[len]='\0';
        pszbuff = szbuff;
        for (i=0; i < 8; i++)
        {
            char* p = SkipToken(pszbuff);
            pszbuff = p;
        }

        *plUserUsage += strtoul(pszbuff, &pszbuff, 0);
        *plKernUsage += strtoul(pszbuff, &pszbuff, 0);

        return HXR_OK;
    }
    else 
    {
        *plUserUsage = 0;
        *plUserUsage = 0;
        return HXR_FAIL;
    }
}

HX_RESULT
CBsdSysInfo::GetTotalCPUUsage(UINT32* pUsage)
{
    static long cp_time[CPUSTATES];
    size_t nLen = sizeof(cp_time);

#if defined(_FREEBSD)
    if (sysctlbyname("kern.cp_time", &cp_time, &nLen, NULL, 0) < 0)
#elif defined(_NETBSD)
    int mib[2] = { CTL_KERN, KERN_CP_TIME };
    if (sysctl(mib, 2, &cp_time, &nLen, NULL, 0) < 0)
#else //_OPENBSD
    int mib[2] = { CTL_KERN, KERN_CPTIME };
    if (sysctl(mib, 2, &cp_time, &nLen, NULL, 0) < 0)
#endif
    {
        return HXR_FAIL;
    }

    *pUsage += cp_time[CP_USER] + cp_time[CP_NICE] + cp_time[CP_SYS] + cp_time[CP_INTR];
    return HXR_OK;
}


/***********************************************************************
 * HP-UX
 */
#elif defined(_HPUX)
#include <sys/pstat.h>
#include <sys/dk.h>

CHPSysInfo::CHPSysInfo()
{
}

CHPSysInfo::~CHPSysInfo()
{
}

UINT32
CHPSysInfo::GetNumberofCPU(void)
{
    struct pst_dynamic pst;
    int numproc = 1;

    if (pstat_getdynamic(&pst, sizeof(pst), (size_t)1, 0) != -1)
    {
        numproc = pst.psd_proc_cnt;
    }
    return numproc;
}

HX_RESULT
CHPSysInfo::GetProcUsage (UINT32 id, float* pfUsage)
{
    pst_status pst;
    if (pstat_getproc(&pst, sizeof(pst), (size_t)0, g_GlobalProcessList[id]) != -1) 
    {
        *pfUsage += pst.pst_pctcpu * 100;
    }
    
    return HXR_OK;
}

HX_RESULT
CHPSysInfo::GetProcUsageById (UINT32 id, UINT32* plUserUsage, UINT32* plKernUsage)
{
    pst_status pst;
    if (pstat_getproc(&pst, sizeof(pst), (size_t)0, g_GlobalProcessList[id]) != -1) 
    {
        *plUserUsage += pst.pst_cptickstotal;
        //*plKernUsage += ?
    }
    
    return HXR_OK;
}

HX_RESULT
CHPSysInfo::GetTotalCPUUsage(UINT32* plUsage)
{
    int count = 0;
    pst_processor pst[SYSINFO_MAX_PROCS];

    if ((count = pstat_getprocessor(pst, sizeof(pst_processor), (*g_pCPUCount), 0)) != -1)
    {
        for (int i = 0; i < count; i++)
        {
            *plUsage += pst[i].psp_cpu_time[CP_USER] +
                        pst[i].psp_cpu_time[CP_NICE] +
                        pst[i].psp_cpu_time[CP_SYS];
        }
    }
    return HXR_OK;
}

// This implementation will be used if we GetProcUsage directly in terms of % CPU
// For each of the procs. There is a way to do this for HP

#if 0
HX_RESULT
CHPSysInfo::GetCPUUsage (REF(INT32) ulServerUsage, REF(INT32) ulAggUsage)
{
    struct timeval  EndTime;
    struct timezone tz;
    INT32 ulDuration = 0;
    float fPctCPU = 0.0;
    UINT32 ulCumulativeUsedTime = 0;

    if (gettimeofday(&EndTime, &tz) == 0)
    {
        // Compute total duration in microseconds
        ulDuration = ComputeTimeDelta(&m_StartTime, &EndTime);
        memcpy(&m_StartTime, &EndTime, sizeof(struct timeval));

    }
    for (UINT32 i=0; i < SYSINFO_MAX_THREADS; i++) 
    {
        float fProcPctCPU = 0.0;
        if (g_GlobalProcessList[i] == 0) 
        {
            break;
        }
        GetProcUsage(i, &fProcPctCPU);
        fPctCPU += fProcPctCPU;
    }
    ulServerUsage = fPctCPU / (*g_pCPUCount);
    //    m_ulLastServerCPUUsage = ulUsedTime;

    GetTotalCPUUsage(&ulCumulativeUsedTime);
    ulAggUsage = (ulCumulativeUsedTime - m_ulLastAggCPUUsage) / (ulDuration * (*g_pCPUCount));

    m_ulLastAggCPUUsage = ulCumulativeUsedTime;
    return HXR_OK;
}
#endif


/***********************************************************************
 * AIX
 */
#elif defined(_AIX)
#ifdef _DEBUG
#undef _DEBUG
#endif
#include <procinfo.h>
#include <sys/types.h>
#include <sys/trcctl.h>
#include <nlist.h>
#include <sys/sysinfo.h>

#define KMEM "/dev/kmem"

extern "C" int getprocs(procsinfo*, int, fdsinfo*, int, pid_t*, int);

CAIXSysInfo::CAIXSysInfo(void)
{
    struct nlist nlst[] = { { "sysinfo", 0, 0, 0, 0, 0 },
                            { NULL, 0, 0, 0, 0, 0 } };

    if ((m_kmemfd = open(KMEM, O_RDONLY)) == -1) 
    {
         return;
    }

    // get kernel symbol offsets
    if (knlist(nlst, 1, sizeof(struct nlist)) != 0) {
         return;
    }

    m_ulSysinfoOffset = nlst[0].n_value;
}

CAIXSysInfo::~CAIXSysInfo()
{
}

UINT32
CAIXSysInfo::GetNumberofCPU(void)
{
    return __TRC_SYSCPUS;
}

HX_RESULT
CAIXSysInfo::GetProcUsageById (UINT32 id, UINT32* plUserUsage, UINT32* plKernUsage)
{
    procsinfo pInfo;
    pid_t pid = g_GlobalProcessList[id];

    if (getprocs(&pInfo, sizeof(pInfo), NULL, sizeof(fdsinfo), &pid, 1) != -1)
    {
        *plUserUsage += (pInfo.pi_ru.ru_utime.tv_sec * 100);
        *plUserUsage += (pInfo.pi_ru.ru_utime.tv_usec * 1e-7);
        *plKernUsage += (pInfo.pi_ru.ru_stime.tv_sec * 100);
        *plKernUsage += (pInfo.pi_ru.ru_stime.tv_usec * 1e-7);
    }
    return HXR_OK;
}

HX_RESULT
CAIXSysInfo::GetTotalCPUUsage (UINT32* plUsage)
{
    struct sysinfo s_info;
    getkval(m_ulSysinfoOffset, (caddr_t)&s_info, sizeof(s_info));
    *plUsage = s_info.cpu[CPU_USER] + s_info.cpu[CPU_KERNEL] + s_info.cpu[CPU_WAIT];
    return HXR_OK;
}

HX_RESULT
CAIXSysInfo::GetKernelValue (unsigned long ulOffset, caddr_t pAddr, int nSize)
{
    int bupper_2gb = 0;

    /* reads above 2Gb are done by seeking to offset%2Gb, and supplying
     * 1 (opposed to 0) as fourth parameter to readx (see 'man kmem')
     */
    if (ulOffset > 1<<31)
    {
        bupper_2gb = 1;
        ulOffset &= 0x7fffffff;
    }

    if (lseek(m_kmemfd, ulOffset, SEEK_SET) != ulOffset)
    {
        return HXR_FAIL;
    }

    if (readx(m_kmemfd, pAddr, nSize, bupper_2gb) != nSize)
    {
        return HXR_FAIL;
    }

    return HXR_OK;
}

/***********************************************************************
 * SOLARIS
 */
#elif defined(_SOLARIS)
#include <sys/fcntl.h>
#include <sys/sysinfo.h>

CSolarisSysInfo::CSolarisSysInfo():
    m_pkc(NULL)
{
}

CSolarisSysInfo::~CSolarisSysInfo()
{
    if (m_pkc != NULL)
    {
        kstat_close(m_pkc);
    }
}

UINT32
CSolarisSysInfo::GetNumberofCPU(void)
{
    int nNumProc = sysconf(_SC_NPROCESSORS_ONLN);
    if (nNumProc < 1)
    {
        HX_ASSERT(0); //Shouldn't happen!
        nNumProc = 1;
    }
    
    return nNumProc;
}

HX_RESULT
CSolarisSysInfo::InitCPUCalc (void)
{
    InitProcUsage();
    if (m_pkc != NULL)
    {
        kstat_close(m_pkc);
        m_pkc = NULL;
    }

    GetTotalCPUUsage(&m_ulLastAggCPUUsage);
    kstat_close(m_pkc);
    m_pkc = NULL;
    return HXR_OK;
}


HX_RESULT
CSolarisSysInfo::GetTotalProcUsage (UINT32* plUserUsage, UINT32* plKernUsage)
{
    return CSysInfo::GetTotalProcUsage(plUserUsage, plKernUsage);
}


HX_RESULT
CSolarisSysInfo::GetProcUsageById (UINT32 id, UINT32* plUserUsage, UINT32* plKernUsage)
{
    int fd = 0;
    prusage_t prbuff;
    double cp_time = 0.0;
    char pcProcUsage[PATH_MAX];
  
    snprintf(pcProcUsage,PATH_MAX,"%s/usage",g_GlobalProcListFiles[id]);
    if ((fd = open(pcProcUsage, O_RDONLY)) != -1) 
    {
        if(read(fd, &prbuff, sizeof(prusage_t)) != -1)
        {
            *plUserUsage = (prbuff.pr_utime.tv_sec * 100.0) +
                            (prbuff.pr_utime.tv_nsec / 10000000);
            *plKernUsage = (prbuff.pr_stime.tv_sec * 100.0) +
                            (prbuff.pr_stime.tv_nsec / 10000000);
        }
        else 
        {
            close(fd);
            return HXR_FAIL;
        }
        close(fd);
        return HXR_OK;
    }
    return HXR_FAIL;
}


HX_RESULT
CSolarisSysInfo::GetTotalCPUUsage(UINT32* pulUsage)
{
    kstat_t* ks = NULL;
    kid_t nkcid;
    cpu_stat_t cpuStat;
    int ulCPU = 0;

    if (m_pkc == NULL)
    {
        m_pkc = kstat_open();
    }

    if (m_pkc != NULL)
    {
        nkcid = kstat_chain_update(m_pkc);
        if (nkcid == -1)
        {
            perror("kstat_chain_update");
            kstat_close(m_pkc);
            m_pkc = NULL;
            return HXR_FAIL;
        }
     
        for (ks = m_pkc->kc_chain; ks; ks = ks->ks_next)
        {
            if (strncmp(ks->ks_name, "cpu_stat", 8) == 0)
            {
                if (kstat_read(m_pkc, ks, &cpuStat) == -1)
                {
                    perror("kstat_read");
                    kstat_close(m_pkc);
                    m_pkc = NULL;
                    return HXR_FAIL;
                }

                ulCPU++;
                if (ulCPU > (*g_pCPUCount))
                {
                    break;
                }

                *pulUsage += cpuStat.cpu_sysinfo.cpu[CPU_USER] +
                             cpuStat.cpu_sysinfo.cpu[CPU_KERNEL] +
                             cpuStat.cpu_sysinfo.cpu[CPU_WAIT];
            }
        }
        return HXR_OK;
    }
    perror("kstat_open");
    return HXR_FAIL;
}


/***********************************************************************
 * OSF/1 (Tru64)
 */
#elif defined(_OSF1)
#include <sys/table.h>

COSF1SysInfo::COSF1SysInfo()
{
}

COSF1SysInfo::~COSF1SysInfo()
{
}

UINT32
COSF1SysInfo::GetNumberofCPU(void)
{
    int nNumProc = sysconf(_SC_NPROCESSORS_ONLN);
    if (nNumProc < 1)
    {
        HX_ASSERT(0); //Shouldn't happen!
        nNumProc = 1;
    }
    
    return nNumProc;
}

HX_RESULT
COSF1SysInfo::GetProcUsageById (UINT32 id, UINT32* plUserUsage, UINT32* plKernUsage)
{
    int fd = 0;
    prstatus_t prbuff;
    double cp_time = 0.0;
  
    if ((fd = open(g_GlobalProcListFiles[id], O_RDONLY)) != -1) 
    {
        if (ioctl(fd, PIOCSTATUS, &prbuff) != -1) 
        {
            *plUserUsage += (prbuff.pr_utime.tv_sec * 100.0) +
                           (prbuff.pr_utime.tv_nsec / 10000000);
            *plKernUsage += (prbuff.pr_stime.tv_sec * 100.0) +
                            (prbuff.pr_stime.tv_nsec / 10000000);
        }
        else 
        {
            close(fd);
            return HXR_FAIL;
        }
        close(fd);
        return HXR_OK;
    }
    return HXR_FAIL;
}

HX_RESULT
COSF1SysInfo::GetTotalCPUUsage(UINT32* pulUsage)
{
    tbl_sysinfo stab;

    table(TBL_SYSINFO, 0, &stab, 1, sizeof(tbl_sysinfo));
    *pulUsage = (stab.usr + stab.si_nice + stab.sys) / 10;
    
    return HXR_OK;
}

/***********************************************************************
 * WINDOWS
 */
#elif defined(_WINDOWS)

extern const char* progname;

CWindowsSysInfo::CWindowsSysInfo():
    m_hPerfCPUUsageQuery(NULL),
    m_hPerfMemUsageQuery(NULL), 
    m_pUserCPUCounter(NULL),
    m_pKernCPUCounter(NULL),
    m_pAggCPUCounter(NULL),
    m_pMemCounter(NULL)
{    
}

CWindowsSysInfo::~CWindowsSysInfo()
{
    ClosePerformanceQuery();
}

UINT32
CWindowsSysInfo::GetNumberofCPU(void)
{
    SYSTEM_INFO info;
    GetSystemInfo(&info);
    return info.dwNumberOfProcessors;
}

HX_RESULT
CWindowsSysInfo::InitCPUCalc()
{
    HX_RESULT  hResult   = HXR_OK;
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    
    char szCounterPath[255];
    const char *szProgramName = ServerVersion::ExecutableName();

    // Open the CPU Usage Query
    pdhStatus = PdhOpenQuery (NULL, 1, &m_hPerfCPUUsageQuery);

    if (pdhStatus == ERROR_SUCCESS)
    {
        //
        // Add the Counters
        //

        // Add the User-space CPU Counter
        if (_snprintf(szCounterPath, 255, "\\Process(%s)\\%% User Time", szProgramName) < 0)
        {
            szCounterPath[254] = '\0';
        }
        pdhStatus = PdhAddCounter (m_hPerfCPUUsageQuery, szCounterPath, 0, &m_pUserCPUCounter);

        // Add the Kernel(privelidged)-space CPU Counter
        if (_snprintf(szCounterPath, 255, "\\Process(%s)\\%% Privileged Time", szProgramName) < 0)
        {
            szCounterPath[254] = '\0';
        }
        pdhStatus = PdhAddCounter (m_hPerfCPUUsageQuery, szCounterPath, 0, &m_pKernCPUCounter);

        // Add the Total Processor Usage Counter
        pdhStatus = PdhAddCounter(m_hPerfCPUUsageQuery, "\\Processor(_Total)\\% Processor Time", 0, &m_pAggCPUCounter);
    }
    else
    {
        hResult = HXR_FAIL;
    }

    // Open the Mem Usage Query
    pdhStatus = PdhOpenQuery (NULL, 1, &m_hPerfMemUsageQuery);

    if (pdhStatus == ERROR_SUCCESS)
    {
        if (_snprintf(szCounterPath, 255, "\\Process(%s)\\Working Set", szProgramName) < 0)
            szCounterPath[254] = '\0';
            // Add the Counters
            pdhStatus = PdhAddCounter (m_hPerfMemUsageQuery, szCounterPath,
                        0, &m_pMemCounter);
        }
    else
    {
        hResult = HXR_FAIL;
    }

    return hResult;
}

HX_RESULT
CWindowsSysInfo::GetCPUUsage (REF(INT32) lUserUsage, REF(INT32) lKernUsage, REF(INT32) lAggUsage)

{
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    PDH_FMT_COUNTERVALUE pValue;

    // Obtain the Counter Values
    pdhStatus = PdhCollectQueryData (m_hPerfCPUUsageQuery); 

    if (pdhStatus == ERROR_SUCCESS)
    {
            // Format the data we obtained
            pdhStatus = PdhGetFormattedCounterValue (m_pUserCPUCounter,
                    PDH_FMT_LONG | 0x00008000,        // 0x00008000 flag stands for NoCapTo100
            //which is an undocumented feature
                    NULL, &pValue);

            if (pdhStatus == ERROR_SUCCESS)
            {
                lUserUsage = pValue.longValue/(*g_pCPUCount);
            }
            
            pdhStatus = PdhGetFormattedCounterValue(m_pKernCPUCounter, PDH_FMT_LONG,
                                                    NULL, &pValue);
            if (pdhStatus == ERROR_SUCCESS)
            {
                lKernUsage = pValue.longValue/(*g_pCPUCount);
            }
            
            pdhStatus = PdhGetFormattedCounterValue(m_pAggCPUCounter, PDH_FMT_LONG,
                                                    NULL, &pValue);
            if (pdhStatus == ERROR_SUCCESS)
            {
                lAggUsage = pValue.longValue;
            }
    }

    return HXR_OK;
}

HX_RESULT
CWindowsSysInfo::GetMemUsage(REF(UINT32) ulMemUsage)
{
    PDH_STATUS pdhStatus = ERROR_SUCCESS;
    PDH_FMT_COUNTERVALUE pValue;

    // Obtain the Counter Values
    pdhStatus = PdhCollectQueryData (m_hPerfMemUsageQuery); 

    if (pdhStatus == ERROR_SUCCESS)
    {
            // Format the data we obtained
            pdhStatus = PdhGetFormattedCounterValue (m_pMemCounter,
                    PDH_FMT_LONG, NULL, &pValue);

            if (pdhStatus == ERROR_SUCCESS)
            {
                ulMemUsage = (UINT32)pValue.longValue;
                return HXR_OK;
            }   
    }

    return HXR_FAIL;
}

HX_RESULT
CWindowsSysInfo::ClosePerformanceQuery()
{
    HX_RESULT hr = HXR_OK;
    PDH_STATUS pdhStatus = ERROR_SUCCESS;

    // Close the Query
    if (m_hPerfCPUUsageQuery)
    {
        pdhStatus = PdhCloseQuery(m_hPerfCPUUsageQuery);
        if (pdhStatus != ERROR_SUCCESS)
        {
            hr = HXR_FAIL;
        }
    }

    if (m_hPerfMemUsageQuery)
    {
        pdhStatus = PdhCloseQuery(m_hPerfMemUsageQuery);
        if (pdhStatus != ERROR_SUCCESS)
        {
            hr = HXR_FAIL;
        }
    }

    return hr;
}

#endif



/***********************************************************************
 * UNIX
 */
#ifdef _UNIX
static inline UINT32
ComputeTimeDelta(struct timeval* pStartTime, struct timeval* pEndTime)
{
    INT32 nTimeDelta = 0;
    
    if (!pStartTime || !pEndTime)
    {
        return 0;
    }

    nTimeDelta += (pEndTime->tv_sec - pStartTime->tv_sec);
    if (pEndTime->tv_usec > pStartTime->tv_usec)
    {
        nTimeDelta += (pEndTime->tv_usec - pStartTime->tv_usec) / 1000000; 
    }
    else
    {
        nTimeDelta -= (pStartTime->tv_usec - pEndTime->tv_usec) / 1000000; 
    }

    return nTimeDelta;
}
#endif

