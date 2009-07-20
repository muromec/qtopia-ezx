/* ***** BEGIN LICENSE BLOCK *****
* Source last modified: $Id
* 
* Portions Copyright (c) 1995-2004 RealNetworks, Inc. All Rights Reserved.
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
* Alternatively, the contents of this file may be used under the
* terms of the GNU General Public License Version 2 or later (the
* "GPL") in which case the provisions of the GPL are applicable
* instead of those above. If you wish to allow use of your version of
* this file only under the terms of the GPL, and not to allow others
* to use your version of this file under the terms of either the RPSL
* or RCSL, indicate your decision by deleting the provisions above
* and replace them with the notice and other provisions required by
* the GPL. If you do not delete the provisions above, a recipient may
* use your version of this file under the terms of any one of the
* RPSL, the RCSL or the GPL.
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


#include "symbiannameonlyff.h"
#include "chxdataf.h"
#include "hxstring.h"
#include "hxprefs.h"
#include "hxprefutil.h"
#include "chxliteprefs.h"

#include "symbian_string.h"
#define READ_SIZE 1024

HXSymbianNameOnlyFindFile::HXSymbianNameOnlyFindFile(IUnknown* pContext) :
CFindFile(NULL, NULL, NULL),
m_ScanDir(NULL,0),
m_FilePattern(NULL,0),
m_pPluginDLLFileName(NULL),
m_pContext(pContext),
m_pFile(NULL),
m_State(eParsingWhiteSpace),
m_pBuffer(NULL),
m_pPos(NULL),
m_Count(NULL)
{
    if(pContext)
    {
        HX_ADDREF(m_pContext);
    }
}


void HXSymbianNameOnlyFindFile::ResetList()
{
    CHXSimpleList::Iterator iter; 
    for(iter = m_entries.Begin(); iter!=m_entries.End(); ++iter)
    {
	    CHXString* name = (CHXString*) *iter;
	    HX_DELETE(name);
    }
    m_entries.RemoveAll();
}

HXSymbianNameOnlyFindFile::~HXSymbianNameOnlyFindFile()
{
    HX_RELEASE(m_pContext);
    m_Fs.Close(); 
    ResetList();
    HX_DELETE(m_pPluginDLLFileName); 
    HX_DELETE(m_pBuffer);
    HX_DELETE(m_pFile);
}


HX_RESULT HXSymbianNameOnlyFindFile::ReadPatternFromPrefs()
{
    HX_RESULT rv = HXR_FAIL;
    CHXString aFilePattern; 
   
    if(m_pContext != NULL)
    {
        IHXPreferences *pPref = NULL;
        if(m_pContext->QueryInterface(IID_IHXPreferences, (void**) &pPref) == HXR_OK)
        {
            CHXString preference("PluginDLLFileName");
			CHXSymbianString::PrefixNameSpace(preference);
            if ( SUCCEEDED(ReadPrefCSTRING(pPref, preference, aFilePattern) ) )
            {
                m_pPluginDLLFileName = CHXSymbianString::StringToHBuf(aFilePattern);
                if(m_pPluginDLLFileName)
                {
                    if(KErrNone == m_parser.Set(*m_pPluginDLLFileName,NULL,NULL))
                    {
                        m_ScanDir.Set(m_parser.Path());
                        m_FilePattern.Set(m_parser.NameAndExt());
                        rv = HXR_OK; 
                    }
                }
            }
            HX_RELEASE(pPref);
        }
    } 
    return rv;
}

HX_RESULT HXSymbianNameOnlyFindFile::Open()
{
    HX_RESULT rv = HXR_FAIL;
    CDir*   aFileList = NULL; 
    
    if(SUCCEEDED(ReadPatternFromPrefs()))
    {
        if( KErrNone == m_Fs.Connect() )
        {
            TFindFile aFileFinder(m_Fs); 
            TInt err = aFileFinder.FindWildByDir(m_FilePattern, m_ScanDir, aFileList); 
            while (err == KErrNone)
            {
                for(TInt i = 0 ; i < aFileList->Count(); i++)
                {
                    TParse aFullEntry; 
                    aFullEntry.Set( (*aFileList)[i].iName,& aFileFinder.File(), NULL); 
                    const TDesC& aFileName = aFullEntry.FullName(); 
                    ProcessFile(aFileName);
                }
                delete aFileList; 
                err = aFileFinder.FindWild(aFileList); 
            }
        }
    }
}


void HXSymbianNameOnlyFindFile::ProcessFile(const TDesC& aFile)
{
    CHXString aFileName; 
    CHXSymbianString::DesToString(aFile,aFileName);

        m_pFile = CHXDataFile::Construct(m_pContext);
	if (m_pFile)
	{
		HX_RESULT hxr = m_pFile->Open(aFileName, O_RDONLY, TRUE);
		if ( FAILED(hxr) ) return; 
	}
	char* str = _FindFirst(); 
	while(str)
	{
		m_entries.AddTail(new CHXString(str));
		str = _FindNext(); 
	}
	// We are done with this file.
	HX_DELETE(m_pFile);
}


char* HXSymbianNameOnlyFindFile::FindFirst()
{
    Open();
	m_idx = 0 ;
    return FindNext();
}

char* HXSymbianNameOnlyFindFile::FindNext()
{
	LISTPOSITION pos = m_entries.FindIndex(m_idx); 
	if(pos) 
	{
		CHXString* ret = NULL; 
		ret =  (CHXString*) m_entries.GetAt(pos); 
		m_idx++;
		return (char*)  (const char*) (*ret); 
	}
	else
		return NULL;
}




char* HXSymbianNameOnlyFindFile::_FindFirst()
{
    HX_RESULT hxr = HXR_FAIL;
    char* strReturn = NULL;
   

    // Reset members
    m_State = eParsingWhiteSpace;
    m_Count = 0; // bytes we have read
    m_pPos  = NULL;
    
    if(m_pBuffer == NULL)
    {
        m_pBuffer = new char [READ_SIZE];
    }
    
    if(m_pBuffer)
    {
        memset(m_pBuffer, 0, READ_SIZE);
        m_started = TRUE;
        strReturn = _FindNext();
    }

	return strReturn;
}



char* HXSymbianNameOnlyFindFile::_FindNext()
{
    if(!m_started)
    {
        return NULL;
    }

    HXBOOL bGotName = FALSE; // will be TRUE when we get the name
    
    m_DllName.Empty();
    while(!bGotName)
    {
        if(m_Count == 0)
        {
            // need to read more data
            m_Count = m_pFile->Read(m_pBuffer, READ_SIZE);

            if (m_Count <= 0)
            {
                // end of file
                break;
            }
            m_pPos = m_pBuffer;
        }
        
        switch(m_State)
        {
        case eParsingWhiteSpace:
            {
                switch (*m_pPos)
                {
                case '#':
                case ';':
                    {
                        m_State = eParsingComment;
                    }
                    break;
                    
                default:
                    {
                        m_State    = eParsingName;
                        m_DllName  = *m_pPos; // don't lose this char
                    }
                }
                ++m_pPos;
                --m_Count;
                break;
            }
            
        case eParsingName:
            {
                if (LitePrefs::ParseToken(m_pPos, m_Count, '\n', m_DllName))
                {
                    m_State = eParsingWhiteSpace;
                    bGotName = TRUE;
                }
                break;
            }
            
        case eParsingComment:
            {
                // skip to end of line
                if (LitePrefs::SkipToken(m_pPos, m_Count, '\n'))
                {
                    m_State = eParsingWhiteSpace;
                }
                break;
            }
            
        default:
            HX_ASSERT(FALSE);
            break;
            
        } // switch
    } //  End of while(!bGotName)
    
    if(bGotName)
    {
        // const char *tmp = m_DllName;
        return ((char *) ((const char*) m_DllName));
    }
    else
    {
        return NULL;
    }

}

HXBOOL HXSymbianNameOnlyFindFile::OS_OpenDirectory (const char* dirName)
{
  return FALSE;
}

char* HXSymbianNameOnlyFindFile::OS_GetNextFile ()
{
    return NULL;
}
void HXSymbianNameOnlyFindFile::OS_CloseDirectory ()
{
    // NOT IMPL
}

HXBOOL HXSymbianNameOnlyFindFile::OS_InitPattern ()
{
    return FALSE;
}
HXBOOL HXSymbianNameOnlyFindFile::OS_FileMatchesPattern (const char* fname)
{
    return FALSE;
}

void HXSymbianNameOnlyFindFile::OS_FreePattern ()
{
    // NOT IMPL
}

