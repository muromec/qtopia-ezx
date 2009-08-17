/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: fsys_wrap.h,v 1.3 2004/05/03 19:02:48 tmarshall Exp $ 
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

#ifndef _FSYS_WRAP_H_
#define _FSYS_WRAP_H_

#include "hxfiles.h"
#include "simple_callback.h"
#include "ihxpckts.h"

class Process;
class FileSystemWrapper;
class FileObjectWrapper;
class FSManager;

class GetFileSystemInfoCallback : public SimpleCallback
{
public:
    GetFileSystemInfoCallback(FileSystemWrapper* fsw) :
	m_fsw(fsw)
    {
    };
    void func(Process* p);

    FileSystemWrapper* m_fsw;
};

class InitFileSystemCallback : public SimpleCallback
{
public:
    InitFileSystemCallback(FileSystemWrapper* fsw,
			   IHXValues* options) :
	m_fsw(fsw),
	m_options(options)
    {
	if(m_options)
	    m_options->AddRef();
    };

    ~InitFileSystemCallback()
    {
	if(m_options)
	    m_options->Release();
    }

    void func(Process* p);
    FileSystemWrapper* m_fsw;
    IHXValues* m_options;
};

class FileSystemWrapper : public IHXFileSystemObject
{
private:
    LONG32		    m_lRefCount;
    Process*                m_myproc;
    Process*                m_fs_proc;
    /*
     * The following pointer is in the m_fs_proc domain.  On some operating
     * systems this pointer will be invalid in other process spaces
     */
    IHXFileSystemObject*   m_pRealFS;
    FSManager*              m_response;

    ~FileSystemWrapper();

protected:
    VOLATILE int            m_got_fsys_info;
    const char*         m_pShortName;
    const char*         m_pProtocol;

    friend class GetFileSystemInfoCallback;
    friend class CreateFileCallback;
    friend class InitFileSystemCallback;

public:
    FileSystemWrapper(FSManager*, Process* proc, Process* fsproc,
			IHXFileSystemObject* pFS);

    // *** IUnknown methods ***
    STDMETHOD(QueryInterface)	(THIS_
    	    	    	    	REFIID riid,
    	    	    	    	void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)	(THIS);
    STDMETHOD_(ULONG32,Release)	(THIS);

    // *** IHXFileSystemObject methods ***
    STDMETHOD(InitFileSystem) (THIS_ IHXValues* values);

    STDMETHOD(GetFileSystemInfo)    (THIS_
				     REF(const char*) /*OUT*/ pShortName,
				     REF(const char*) /*OUT*/ pProtocol);
    STDMETHOD(CreateFile)	(THIS_
				 IUnknown**    /*OUT*/	ppFileObject);
    STDMETHOD(CreateDir)        (THIS_
				 IUnknown**     /*OUT*/     ppDirObject);

    HX_RESULT AsyncCreateDone(HX_RESULT, FileObjectWrapper*);
    HX_RESULT AsyncCreateFile();
};

#endif
