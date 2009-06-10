/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxpeff.cpp,v 1.8 2007/07/06 21:58:51 jfinnecy Exp $
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

#include "hlxclib/string.h"
#include "hxpeff.h"
#include "hxfcntl.h"
#include "hlxclib/string.h"
#include "hlxclib/stdio.h"

#include "hxheap.h"
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif

CHXPeff::CHXPeff()
{
	mFile=NULL;
	mBigEndian=FALSE;
	mPeffDataStart=0;

	//
	//  initialize mBigEndian;
	//

	TestBigEndian();
}


CHXPeff::~CHXPeff()
{
	if (mFile)
	{
		close();
	}
}


HX_RESULT	CHXPeff::open(const char* path, IUnknown* pContext)
{
	if (mFile)
	{
		return HXR_RESOURCE_CLOSE_FILE_FIRST;
	}

	//
	//	Create a file object for reading this file in.
	//
	mFile=CHXDataFile::Construct(pContext);

	if (!mFile)
	{
		return HXR_OUTOFMEMORY;
	}

	//
	//	Openthe file
	//

	HX_RESULT	e=HXR_OK;

	e=mFile->Open((const char*)path,O_RDONLY);
	if (e != HXR_OK)
	{
		return e;
	}
	
	//
	//	Read the data in the file header, and determine where to start reading from in the file.
	//	

	e=FindPeffDataStart();

	if (e != HXR_OK)
	{
		return e;
	}

	return HXR_OK;

	
}


HX_RESULT	CHXPeff::close(void)
{
	if (mFile)
	{
		delete mFile;
		mFile=NULL;
	}
	else
	{
		return HXR_FILE_NOT_OPEN;
	}

	return HXR_OK;
}




HX_RESULT	CHXPeff::FindSectionNamed(const char*	name,ULONG32&  size, ULONG32&  pos)
{

	HX_IMAGE_SECTION_HEADER	header;

	size=0;
	pos=0;

	//
	//	Seek to the beginning of the PEFF headers section.
	//
	if (HXR_OK != GetSectionHeaderNamed(name,header))
	{	
		return HXR_AT_END;		
	}
	
	if (HXR_OK != mFile->Seek(header.PointerToRawData,0))
	{
		return HXR_AT_END;
	}

	size=header.SizeOfRawData;
	pos=header.PointerToRawData;
	
	return	HXR_OK;
}


HX_RESULT	CHXPeff::GetSectionHeaderNamed(const char*	name,HX_IMAGE_SECTION_HEADER& h)
{

	//
	//	Seek to the beginning of the PEFF headers section.
	//
	if (HXR_OK !=mFile->Seek(mPeffDataStart,0))
	{	
		return HXR_AT_END;		
	}

	HX_IMAGE_SECTION_HEADER	header;

	for (mCurrentSection=1; mCurrentSection <= mNumberOfSections; mCurrentSection++)
	{
		GetSectionHeader(header);
		if (strcmp((const char*)header.Name,name)==0)
		{

			//
			//	The user wanted the section header. So we give it to them.
			//
			//	We use sizeof here, because we want the user to get
			//	header as it is in memory.  (Makes sure that padding is not an issue.)
			//
			memcpy(&h,&header,sizeof(HX_IMAGE_SECTION_HEADER)); /* Flawfinder: ignore */
	
			return HXR_OK;
		}

	}
	
	return	HXR_AT_END;
}


//
//
//	HELPER FUNCTIONS for reading in bytes,words,and dwords from a file.
//
//




HX_RESULT	CHXPeff::ReadByte(UCHAR&   sz)
{
	if (mFile->Read((char*)&sz,1)==1)
	{
		return HXR_OK;		
	}

	return HXR_AT_END;
}


HX_RESULT	CHXPeff::ReadWord(UINT16&	w)
{
	
	if (mFile->Read((char*)&w,2)==2)
	{
		ReverseWORD(w);
		return HXR_OK;
	}

	return HXR_AT_END;
}

HX_RESULT	CHXPeff::ReadDWord(ULONG32&	dw)
{
	if (mFile->Read((char*)&dw,4)==4)
	{
		ReverseDWORD(dw);
		return HXR_OK;
	}

	return HXR_AT_END;
}



/*
	Notes about the headers. 

    This code forcibly reads in the data from the file with very small reads.
	This is not the most efficient mechanism, but this makes sure that each platform
	will be able to read in the data without attempting to do a read with the size of
	the expanded/padded structure.

    DO NOT ATTEMPT TO MAKE MORE EFFICIENT UNLESS YOU UNDERSTAND THE PROBLEM!

    
*/

HX_RESULT	CHXPeff::InitializeDosHeader(void)
{

	//
	//	Load the DOS Image file header into the 
	//	member variable mDosHeader
	//

	//
	//	Start at the beginning of the file.
	//
	mFile->Seek(0,0);

	UCHAR*	bytes;

	bytes=(UCHAR*)&mDosHeader.e_magic;
	
	//
	//	Needs to be read as two bytes because the e_magic is written as a string would be.
	//
	IF_ERROR_RETURN(ReadByte(bytes[0]));
	IF_ERROR_RETURN(ReadByte(bytes[1]));

	IF_ERROR_RETURN(ReadWord(mDosHeader.e_cblp));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_cp));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_crlc));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_cparhdr));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_minalloc));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_maxalloc));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_ss));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_sp));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_csum));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_ip));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_cs));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_lfarlc));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_ovno));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_res[0]));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_res[1]));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_res[2]));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_res[3]));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_oemid));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_oeminfo));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_res2[0]));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_res2[1]));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_res2[2]));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_res2[3]));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_res2[4]));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_res2[5]));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_res2[6]));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_res2[7]));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_res2[8]));
	IF_ERROR_RETURN(ReadWord(mDosHeader.e_res2[9]));
	IF_ERROR_RETURN(ReadDWord(mDosHeader.e_lfanew));

	return HXR_OK;
}



HX_RESULT	CHXPeff::InitializePeffHeader(void)
{

	//
	//	Seek into the first PEFF header.
	//
	IF_ERROR_RETURN(mFile->Seek(mDosHeader.e_lfanew+SIZE_OF_NT_SIGNATURE,0));


	IF_ERROR_RETURN(ReadWord(mPeffHeader.Machine));
	IF_ERROR_RETURN(ReadWord(mPeffHeader.NumberOfSections));
	IF_ERROR_RETURN(ReadDWord(mPeffHeader.TimeDateStamp));
	IF_ERROR_RETURN(ReadDWord(mPeffHeader.PointerToSymbolTable));
	IF_ERROR_RETURN(ReadDWord(mPeffHeader.NumberOfSymbols));
	IF_ERROR_RETURN(ReadWord(mPeffHeader.SizeOfOptionalHeader));
	IF_ERROR_RETURN(ReadWord(mPeffHeader.Characteristics));

	//
	//	INITIALIZE THE NUMBER OF SECTIONS MEMBER
	//

	mNumberOfSections=mPeffHeader.NumberOfSections;

	return HXR_OK;

}



HX_RESULT	CHXPeff::InitializeOptionalPeffHeader(void)
{

	//
	//	Seek to the second peff header.
	//
	IF_ERROR_RETURN(mFile->Seek(mDosHeader.e_lfanew+SIZE_OF_NT_SIGNATURE+IMAGE_SIZEOF_FILE_HEADER,0));

	IF_ERROR_RETURN(ReadWord(mOptionalPeffHeader.Magic));
	IF_ERROR_RETURN(ReadByte(mOptionalPeffHeader.MajorLinkerVersion));
	IF_ERROR_RETURN(ReadByte(mOptionalPeffHeader.MinorLinkerVersion));
	IF_ERROR_RETURN(ReadDWord(mOptionalPeffHeader.SizeOfCode));
	IF_ERROR_RETURN(ReadDWord(mOptionalPeffHeader.SizeOfInitializedData));
	IF_ERROR_RETURN(ReadDWord(mOptionalPeffHeader.SizeOfUninitializedData));
	IF_ERROR_RETURN(ReadDWord(mOptionalPeffHeader.AddressOfEntryPoint));
	IF_ERROR_RETURN(ReadDWord(mOptionalPeffHeader.BaseOfCode));
	IF_ERROR_RETURN(ReadDWord(mOptionalPeffHeader.BaseOfData));

	//
	//	NT Additional Fields
	//

	IF_ERROR_RETURN(ReadDWord(mOptionalPeffHeader.ImageBase));
	IF_ERROR_RETURN(ReadDWord(mOptionalPeffHeader.SectionAlignment));
	IF_ERROR_RETURN(ReadDWord(mOptionalPeffHeader.FileAlignment));

	IF_ERROR_RETURN(ReadWord(mOptionalPeffHeader.MajorOperatingSystemVersion));
	IF_ERROR_RETURN(ReadWord(mOptionalPeffHeader.MinorOperatingSystemVersion));
	IF_ERROR_RETURN(ReadWord(mOptionalPeffHeader.MajorImageVersion));
	IF_ERROR_RETURN(ReadWord(mOptionalPeffHeader.MinorImageVersion));
	IF_ERROR_RETURN(ReadWord(mOptionalPeffHeader.MajorSubsystemVersion));
	IF_ERROR_RETURN(ReadWord(mOptionalPeffHeader.MinorSubsystemVersion));

	IF_ERROR_RETURN(ReadDWord(mOptionalPeffHeader.Reserved1));
	IF_ERROR_RETURN(ReadDWord(mOptionalPeffHeader.SizeOfImage));
	IF_ERROR_RETURN(ReadDWord(mOptionalPeffHeader.SizeOfHeaders));
	IF_ERROR_RETURN(ReadDWord(mOptionalPeffHeader.CheckSum));
	
	IF_ERROR_RETURN(ReadWord(mOptionalPeffHeader.Subsystem));
	IF_ERROR_RETURN(ReadWord(mOptionalPeffHeader.DllCharacteristics));
	
	IF_ERROR_RETURN(ReadDWord(mOptionalPeffHeader.SizeOfStackReserve));
	IF_ERROR_RETURN(ReadDWord(mOptionalPeffHeader.SizeOfStackCommit));
	IF_ERROR_RETURN(ReadDWord(mOptionalPeffHeader.SizeOfHeapReserve));
	IF_ERROR_RETURN(ReadDWord(mOptionalPeffHeader.SizeOfHeapCommit));
	IF_ERROR_RETURN(ReadDWord(mOptionalPeffHeader.LoaderFlags));
	IF_ERROR_RETURN(ReadDWord(mOptionalPeffHeader.NumberOfRvaAndSizes));


	//
	//	We don't read the data directories here, because the caller asks for sections
	//	by NAME.  Thus we merely parse the sections themselves.
	//



	return HXR_OK;

}


HX_RESULT	CHXPeff::GetSectionHeader(HX_IMAGE_SECTION_HEADER&	  header)
{

	IF_ERROR_RETURN(mFile->Read((char*)&header.Name,IMAGE_SIZEOF_SHORT_NAME)!=IMAGE_SIZEOF_SHORT_NAME);

	//
	//	Misc is a union so we only need to read one of the fields.
	//
	IF_ERROR_RETURN(ReadDWord(header.Misc.PhysicalAddress));

	IF_ERROR_RETURN(ReadDWord(header.VirtualAddress));
	IF_ERROR_RETURN(ReadDWord(header.SizeOfRawData));
	IF_ERROR_RETURN(ReadDWord(header.PointerToRawData));
	IF_ERROR_RETURN(ReadDWord(header.PointerToRelocations));
	IF_ERROR_RETURN(ReadDWord(header.PointerToLinenumbers));

	IF_ERROR_RETURN(ReadWord(header.NumberOfRelocations));
	IF_ERROR_RETURN(ReadWord(header.NumberOfLinenumbers));
	
	IF_ERROR_RETURN(ReadDWord(header.Characteristics));

	return HXR_OK;
}


HX_RESULT	CHXPeff::InitializeHeaderMembers(void)
{

	HX_RESULT	result=HXR_OK;

	//
	//	FIRST READ THE DOS HEADER INTO THE MEMBER VARIABLE.
	//

	result=InitializeDosHeader();
	if (result != HXR_OK)
	{
		return result;
	}


	//
	//	Read the first PEFF header into memory.
	//

	result=InitializePeffHeader();
	if (result != HXR_OK)
	{
		return result;
	}

	//
	//	Okay read the PEFF optional header into memory. 
	//	This header is not actually optional for DLL's or EXE's.
	//
	result=InitializeOptionalPeffHeader();
	if (result != HXR_OK)
	{
		return result;
	}

	return HXR_OK;
}


HX_RESULT	CHXPeff::FindPeffDataStart(void)
{

	HX_RESULT	result=HXR_OK;

	result=InitializeHeaderMembers();

	if (result != HXR_OK)
	{
		return result;
	}
	mPeffDataStart=IMAGE_SIZEOF_FILE_HEADER+mPeffHeader.SizeOfOptionalHeader+mDosHeader.e_lfanew+SIZE_OF_NT_SIGNATURE;

	return HXR_OK;
}


