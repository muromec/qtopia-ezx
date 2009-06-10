/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: symbiannameonlyff.h,v 1.3 2007/07/16 16:04:22 atewari Exp $
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
 * terms of the GNU General Public License Version 2 (the
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


#ifndef _SYMBIAN_NAMEONLYFF_H_
#define _SYMBIAN_NAMEONLYFF_H_

#include "findfile.h"
#include "hxslist.h"
#include <f32file.h>

// Forward declarations
class CHXString;
class CHXDataFile;

//
// HXSymbianNameOnlyFindFile iterates through the content of the provided list file
// returns entries one at a time. Delimiter is \n
// In case of PlatSec, dll names are read from a config file. 
// 
class HXSymbianNameOnlyFindFile : public CFindFile
{
public:
    HXSymbianNameOnlyFindFile(IUnknown* pContext);

    ~HXSymbianNameOnlyFindFile();

    // Public Methods from base class CFindFile
    // Overrided methods
    virtual char*   FindFirst();
    virtual char*   FindNext();

protected:

    HX_RESULT Open();

    //
    // pure virtual methods from CFindFile.
    // These methods are not used in the derived class
    //
	HXBOOL	OS_OpenDirectory (const char* dirName);
	char*	OS_GetNextFile ();
	void	OS_CloseDirectory ();

	HXBOOL	OS_InitPattern ();
	HXBOOL	OS_FileMatchesPattern (const char* fname);
	void 	OS_FreePattern ();
    
private:

    enum ParsingState
    {
        eParsingWhiteSpace=0,
        eParsingName=1,
        eParsingComment=2
    };

    IUnknown*		   m_pContext;  // Context stored for pref QI
    CHXString          m_ConfigFileName; // name of the config file.
    CHXDataFile*       m_pFile;     // to read m_ConfigFileName
    CHXSimpleList      m_entries; // entries from all the matching files.
    UINT32             m_idx; // index into entries list
    TParse             m_parser; 
    HBufC*             m_pPluginDLLFileName; // Contents of PluginDLLFileName 
    TPtrC              m_ScanDir;  // Directory to Scan
    TPtrC              m_FilePattern; //Filename Pattern to match
    RFs                m_Fs; 
    ParsingState       m_State;
    char*              m_pBuffer;   // for reading file data
    char*              m_pPos;      // for parsing the data
    INT32              m_Count;     // for parsing
    CHXString          m_DllName;   // name of the Dll


private: 

	void ProcessFile(const TDesC& aFileName);
    HX_RESULT ReadPatternFromPrefs(); //
    void ResetList();

    char* _FindFirst();
    char* _FindNext(); 
};

#endif /* _SYMBIAN_NAMEONLYFF_H_ */
