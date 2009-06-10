/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: hxpeff.h,v 1.6 2007/07/06 21:58:52 jfinnecy Exp $
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

#ifndef _hxpeff_h
#define _hxpeff_h

#include	"chxdataf.h"
#include	"hxtypes.h"
#include	"hxresult.h"
#include	"littobig.h"

/*
	MACROS
*/

//
//	Used to make reading in the headers easier to write.
//
#define	    IF_ERROR_RETURN(x)	if (x != HXR_OK) return x


/*
	DEFINITIONS
*/

#ifndef _WIN32

#define	IMAGE_DOS_SIGNATURE		0x5A4D
#define	IMAGE_OS2_SIGNATURE		0x454E
#define	IMAGE_OS2_SIGNATURE_LE	0x454C
#define	IMAGE_NT_SIGNATURE		0x00004550

#endif

#define SIZE_OF_NT_SIGNATURE	sizeof(ULONG32)

/*
	Structures
*/


/*
	This is the DOS file image header.
*/


typedef struct	_HX_IMAGE_DOS_HEADER	
{
	UINT16		e_magic;
	UINT16		e_cblp;
	UINT16		e_cp;
	UINT16		e_crlc;
	UINT16		e_cparhdr;
	UINT16		e_minalloc;
	UINT16		e_maxalloc;
	UINT16		e_ss;
	UINT16		e_sp;
	UINT16		e_csum;
	UINT16		e_ip;
	UINT16		e_cs;
	UINT16		e_lfarlc;
	UINT16		e_ovno;
	UINT16		e_res[4];
	UINT16		e_oemid;
	UINT16		e_oeminfo;
	UINT16		e_res2[10];
	ULONG32		e_lfanew;
} HX_IMAGE_DOS_HEADER, *PHX_IMAGE_DOS_HEADER;


/*
	This structure is the PEF file header.
*/


typedef	struct	_HX_IMAGE_FILE_HEADER
{

	UINT16		Machine;
	UINT16		NumberOfSections;
	ULONG32		TimeDateStamp;
	ULONG32		PointerToSymbolTable;
	ULONG32		NumberOfSymbols;
	UINT16		SizeOfOptionalHeader;
	UINT16		Characteristics;


} HX_IMAGE_FILE_HEADER, *PHX_IMAGE_FILE_HEADER;

#define	IMAGE_SIZEOF_FILE_HEADER				20


typedef struct _HX_IMAGE_DATA_DIRECTORY {
    ULONG32   VirtualAddress;
    ULONG32   Size;
} HX_IMAGE_DATA_DIRECTORY, *PHX_IMAGE_DATA_DIRECTORY;

#define IMAGE_NUMBEROF_DIRECTORY_ENTRIES    16


typedef	struct	_HX_IMAGE_FILE_OPTIONAL_HEADER
{

	UINT16	Magic;
	UCHAR	MajorLinkerVersion;
	UCHAR	MinorLinkerVersion;
	ULONG32	SizeOfCode;
	ULONG32	SizeOfInitializedData;
	ULONG32	SizeOfUninitializedData;
	ULONG32	AddressOfEntryPoint;
	ULONG32	BaseOfCode;
	ULONG32	BaseOfData;
	//
	//	NT Additional fields.
	//
	ULONG32	ImageBase;
	ULONG32 SectionAlignment;
	ULONG32	FileAlignment;
	UINT16	MajorOperatingSystemVersion;
	UINT16  MinorOperatingSystemVersion;
	UINT16  MajorImageVersion;
	UINT16	MinorImageVersion;
	UINT16	MajorSubsystemVersion;
	UINT16	MinorSubsystemVersion;
	ULONG32	Reserved1;
	ULONG32	SizeOfImage;
	ULONG32	SizeOfHeaders;
	ULONG32	CheckSum;
	UINT16	Subsystem;
	UINT16	DllCharacteristics;
	ULONG32	SizeOfStackReserve;
	ULONG32	SizeOfStackCommit;
	ULONG32	SizeOfHeapReserve;
	ULONG32	SizeOfHeapCommit;
	ULONG32	LoaderFlags;
	ULONG32	NumberOfRvaAndSizes;

	HX_IMAGE_DATA_DIRECTORY	DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES];
	

}	HX_IMAGE_FILE_OPTIONAL_HEADER,  *PHX_IMAGE_FILE_OPTIONAL_HEADER;


#define	 IMAGE_SIZEOF_SHORT_NAME	8

typedef	 struct _HX_IMAGE_SECTION_HEADER 
{

	UCHAR	Name[IMAGE_SIZEOF_SHORT_NAME];
	union
	{

		ULONG32		PhysicalAddress;
		ULONG32		VirtualSize;

	} Misc;

	ULONG32	VirtualAddress;
	ULONG32	SizeOfRawData;
	ULONG32	PointerToRawData;
	ULONG32	PointerToRelocations;
	ULONG32	PointerToLinenumbers;
	UINT16	NumberOfRelocations;
	UINT16	NumberOfLinenumbers;
	ULONG32	Characteristics;

} HX_IMAGE_SECTION_HEADER, *PHX_IMAGE_SECTION_HEADER;



class	CHXPeff : public CLittleEndianToBigEndian
{

public:

	CHXPeff();
	~CHXPeff();

	
	HX_RESULT	open(const char*  path, IUnknown* pContext);
	HX_RESULT	close();

	HX_RESULT	FindSectionNamed(const	char*	name, ULONG32&	size, ULONG32& pos);
	HX_RESULT	GetSectionHeaderNamed(const char*	name,HX_IMAGE_SECTION_HEADER& h);

//
//	Utility functions.
//
	HX_RESULT	GetDosHeader(HX_IMAGE_DOS_HEADER* header) { memcpy(header,&mDosHeader,sizeof(mDosHeader)); return HXR_OK;}; /* Flawfinder: ignore */
	HX_RESULT	GetPeffHeader(HX_IMAGE_FILE_HEADER* header) { memcpy(header,&mPeffHeader,sizeof(mPeffHeader)); return HXR_OK;}; /* Flawfinder: ignore */
	HX_RESULT	GetPeffOptHeader(HX_IMAGE_FILE_OPTIONAL_HEADER* header) { memcpy(header,&mOptionalPeffHeader,sizeof(mOptionalPeffHeader)); return HXR_OK;}; /* Flawfinder: ignore */


	CHXDataFile*	  mFile;

protected:

	HX_RESULT	ReadByte(UCHAR&   sz);
	HX_RESULT	ReadWord(UINT16&	w);
	HX_RESULT	ReadDWord(ULONG32&	dw);

	HXBOOL		mBigEndian;	// Used to store the test for big endianness.
	ULONG32		mPeffDataStart;

private:

	HX_RESULT				FindPeffDataStart(void);
	HX_RESULT				InitializeHeaderMembers(void);
	HX_RESULT				InitializePeffHeader(void);
	HX_RESULT				InitializeDosHeader(void);
	HX_RESULT				InitializeOptionalPeffHeader(void);
	HX_RESULT				GetSectionHeader(HX_IMAGE_SECTION_HEADER&	  header);
	
	HX_IMAGE_DOS_HEADER			mDosHeader;
	HX_IMAGE_FILE_HEADER			mPeffHeader;
	HX_IMAGE_FILE_OPTIONAL_HEADER		mOptionalPeffHeader;

	UINT16							mNumberOfSections;
	UINT16							mCurrentSection;
};


#endif //_hxpeff_h
