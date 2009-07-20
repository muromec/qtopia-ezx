/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: xmlwrite.cpp,v 1.7 2005/03/14 19:36:32 bobclark Exp $
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

/* 
 * $Id: xmlwrite.cpp,v 1.7 2005/03/14 19:36:32 bobclark Exp $
 *
 * XMLWriter Class Implementation File
 * -----------------------------------
 * 
 * Author:	Consumer Group
 *		RealNetworks Inc., Copyright (C) 1997, All rights reserved
 *		January 12, 1999
 *
 * Abstraction:
 * This file contains the implementation of the XMLWriter class and all it's 
 * helper classes.
 *
 */

// Includes for this file...
#include "xmlwrite.h"
#include "looseprs.h"
#include "hxslist.h"
#include "chxdataf.h"
#include "hxcom.h"
#include "hxbuffer.h"
#include "hxstrutl.h"
//#include <fcntl.h>


// For debugging...
#include "hxassert.h"
#include "hxheap.h"

#ifdef _DEBUG
#undef HX_THIS_FILE             
static const char HX_THIS_FILE[] = __FILE__;
#endif

#define MAX_WRITER_BUFFER 1024

#if defined _WINDOWS
#define WRITER_EOL "\n"
#elif defined _MACINTOSH
#define WRITER_EOL "\r"
#else
#define WRITER_EOL "\n"
#endif


/*
 * XMLWriter
 * ---------
 * Constructor.
 *
 * input:
 * void
 *
 * output:
 * N/A
 *
 */
XMLWriter::XMLWriter(void)
{
}



/*
 * ~XMLWriter
 * ----------
 * Destructor.
 *
 * input:
 * void
 *
 * output:
 * N/A
 *
 */
XMLWriter::~XMLWriter(void)
{
    // Clear it...
    Clear();
}



/*
 * CreateTag
 * ---------
 * Creates a tag in the writer and returns a pointer to it.
 *
 * input:
 * const char *name	    - Name for the tag.
 *
 * output:
 * XMLWriterTag *	    - Pointer to the newly created tag.
 *
 */
XMLWriterTag *
XMLWriter::CreateTag(const char *name)
{
    // Create the tag...
    XMLWriterTag *newTag = new XMLWriterTag;
    HX_ASSERT(newTag != NULL);

    newTag->SetName(name);
    
    // Add the tag to the array...
    m_tags.AddTail(newTag);
    return newTag;
}








/*
 * Write
 * -----
 * Writes all the data to a IHXBuffer object and returns a pointer to the buffer.
 *
 * input:
 * IHXBuffer *		- Pointer to an IHXBuffer.  User must delete it.
 * INT32& length	- Number of characters written out to the buffer.
 *
 * output:
 * HXBOOL			- TRUE if successful.
 *
 */
HXBOOL
XMLWriter::Write(IHXBuffer *buffer, INT32& numChars)
{
    HX_ASSERT(buffer != NULL);

    // Get the size required...
    INT32 length = GetLength();

    // Set the size for the buffer...
    buffer->SetSize(length + length/2);

    // Go through the tags and write them to the buffer...
    LISTPOSITION position = m_tags.GetHeadPosition();
    while (position != NULL)
	((XMLWriterTag *)(m_tags.GetNext(position)))->Write(buffer, numChars);

    // Ok..
    return TRUE;
}





/* 
 * GetLength
 * ---------
 * Returns the entire length required for the tree of xml.
 *
 * input:
 * void
 *
 * output:
 * INT32	    - Length to write the xml.
 *
 */
INT32
XMLWriter::GetLength(void) const
{
    INT32 length = 0;
    // Go through the list of tags and sum up their individual lengths....
    LISTPOSITION position = m_tags.GetHeadPosition();
    while (position != NULL)
    {
	XMLWriterTag *tag = (XMLWriterTag *)(m_tags.GetNext(position));
	length += tag->GetLength();
    }
    return length;
}







/* 
 * Clear
 * -----
 * Clear the structure from memory.
 *
 * input:
 * void
 *
 * output:
 * void
 *
 */
void
XMLWriter::Clear(void)
{
    // If there are any tags in the list, delete them...
    LISTPOSITION position = m_tags.GetHeadPosition();
    while(position != NULL)
    {
	delete (XMLWriterTag *)(m_tags.GetNext(position));
    }
}





//*****************************************************************************************************************
// XMLWriterAttribute Implementation
//*****************************************************************************************************************


 
/*
 * XMLWriterAttribute
 * ------------------
 * Writer attribute constructor.
 *
 * input:
 * void
 *
 * output:
 * N/A
 *
 */
XMLWriterAttribute::XMLWriterAttribute(void) : m_name(NULL), m_value(NULL)
{
}

 


/* 
 * ~XMLWriterAttribute
 * -------------------
 * Writer attribute destructor.
 *
 * input:
 * void
 *
 * output:
 * N/A
 *
 */
XMLWriterAttribute::~XMLWriterAttribute(void)
{
    // Delete the strings...
    if (m_name != NULL) delete [] m_name;
    if (m_value != NULL) delete [] m_value;
    m_name = m_value = NULL;
}




/* 
 * Write
 * -----
 * Write to the given buffer.
 *
 * input:
 * IHXBuffer *buffer		    - Buffer to write to.
 * INT32 loc			    - Pointer into the buffer from where to begin to write.
 *
 * output:
 * void
 *
 */
void
XMLWriterAttribute::Write(IHXBuffer *buffer, INT32& loc)
{
    HX_ASSERT(buffer != NULL);
    
    // If either the name or the value are empty, do not write anything out...
    if (m_name == NULL) return;

    // Properly format what we will write out...
    char temp[MAX_WRITER_BUFFER]; /* Flawfinder: ignore */
    temp[0] = '\0';
    char tabs[MAX_WRITER_BUFFER]; /* Flawfinder: ignore */
    tabs[0] = '\0';

    // Create the proper indentation...
    for (INT32 i = 0; i < m_depth; i++)
	SafeStrCat(tabs, "\t", MAX_WRITER_BUFFER);

    // Create the output string...
    SafeSprintf(temp, MAX_WRITER_BUFFER, "%s%s=\"%s\"", tabs, m_name, m_value);

    // Write to file...
    SafeStrCpy((char *)(buffer->GetBuffer()) + loc,  temp, buffer->GetSize()-loc);
    loc += strlen(temp);
}





/*
 * GetLength
 * ---------
 * Returns the length required to print this attribute.
 *
 * input:
 * void
 *
 * output:
 * INT32	    - Length required.
 *
 */
INT32 
XMLWriterAttribute::GetLength(void) const
{
    // Get the lengths of the strings and return their sum...
    INT32 nameLength = strlen(m_name);
    INT32 valueLength = strlen(m_value);
    return m_depth + nameLength + valueLength + 2;
}





/*
 * SetName
 * -------
 * Set the name for the attribute.
 *
 * input:
 * const char *name	    - Name of the attribute.
 *
 * output:
 * void
 *
 */
void 
XMLWriterAttribute::SetName(const char *name)
{
    if (m_name != NULL) delete [] m_name;
    m_name = NULL;
    if (name == NULL) return;
    m_name = new char[strlen(name) + 1];
    strcpy(m_name, name); /* Flawfinder: ignore */
}




/*
 * SetValue
 * --------
 * Set the value for the attribute.
 *
 * input:
 * const char *value		- Value of the attribute.
 *
 * output:
 * void
 *
 */
void
XMLWriterAttribute::SetValue(const char *value)
{
    if (m_value != NULL) delete [] m_value;
    m_value = NULL;
    if (value == NULL) return;
    m_value = new char[strlen(value) + 1];
    strcpy(m_value, value); /* Flawfinder: ignore */
}




//*****************************************************************************************************************
// XMLWriterTag Implementation
//*****************************************************************************************************************



/* 
 * XMLWriterTag
 * ------------
 * Constructor.
 *
 * input:
 * void
 *
 * output:
 * N/A
 *
 */
XMLWriterTag::XMLWriterTag(void) : m_type(XMLPlainTag), m_name(NULL), m_comment(NULL), m_depth(0)
{
}



/* 
 * ~XMLWriterTag
 * -------------
 * Destructor.
 *
 * input:
 * void
 *
 * output:
 * N/A
 *
 */
XMLWriterTag::~XMLWriterTag(void)
{
    // Go through the list of children and delete each of them.  This will then be
    // repeated by the children's children and we will recursively delete the tree.
    LISTPOSITION tagPosition = m_tags.GetHeadPosition();
    while (tagPosition != NULL)
    {
	delete (XMLWriterTag *)(m_tags.GetNext(tagPosition));
    }

    // Delete the attributes...
    LISTPOSITION attrPosition = m_attributes.GetHeadPosition();
    while (attrPosition != NULL)
    {
	delete (XMLWriterAttribute *)(m_attributes.GetNext(attrPosition));
    }   

    // Delete the strings...
    if (m_name != NULL) delete [] m_name;
    if (m_comment != NULL) delete [] m_comment;
    m_name = NULL;
    m_comment = NULL;
}






/*
 * GetLength
 * ---------
 * Returns the length required to print this and all it's attributes.
 *
 * input:
 * void
 *
 * output:
 * INT32		    - The length required.
 *
 */
INT32
XMLWriterTag::GetLength(void) const
{
    INT32 length = 0;

    // Get the combined attributes lengths...
    LISTPOSITION position = m_attributes.GetHeadPosition();
    while (position != NULL)
    {
	length += ((XMLWriterAttribute *)(m_attributes.GetNext(position)))->GetLength();
    }

    // Get the combined lengths of the child tags...
    position = m_tags.GetHeadPosition();
    while (position != NULL)
    {
	length += ((XMLWriterTag *)(m_tags.GetNext(position)))->GetLength();
    }

    // Tack on the length required in the different cases...
    length += 2*m_depth + ((m_name != NULL)? 2*(strlen(m_name)) : 0) + 20 + ((m_comment != NULL)? strlen(m_comment) : 0);   // 20 is arbitrary, based on all the stuff needed for brackets and slashes.
    return length;
}




/*
 * Write
 * -----
 * Write out this tag to the given file object.
 *
 * input:
 * IHXBuffer *buffer		- Pointer to buffer to use.
 * INT32& loc			- Pointer to a location in the buffer where to print to.
 *
 * output:
 * void
 *
 */
void
XMLWriterTag::Write(IHXBuffer *buffer, INT32& loc)
{
    char temp[MAX_WRITER_BUFFER]; /* Flawfinder: ignore */
    char tabs[MAX_WRITER_BUFFER]; /* Flawfinder: ignore */
    temp[0] = tabs[0] = '\0';
    
    // See the depth of this tag and create a string composed of depth number of tabs...
    for (INT32 i = 0; i < m_depth; i++)
	SafeStrCat(tabs,  "\t", MAX_WRITER_BUFFER);

    // Switch on the type of tag...
    switch (m_type)
    {
    case XMLPlainTag:
	{
	    // If the name field is empty, return...
	    if (m_name == NULL) return;

	    // If there are attributes....
	    if (m_attributes.GetCount() > 0)
	    {
		// Open the open tag...
		SafeSprintf(temp, MAX_WRITER_BUFFER, "%s<%s%s", tabs, m_name, WRITER_EOL);
		SafeStrCpy((char *)(buffer->GetBuffer()) + loc,  temp, buffer->GetSize()-loc);
		loc += strlen(temp);

		// Write the attributes...
		WriteAttributes(buffer, loc);

		// Close the open tag...
		SafeSprintf(temp, MAX_WRITER_BUFFER, ">%s", WRITER_EOL);
		SafeStrCpy((char *)(buffer->GetBuffer()) + loc,  temp, buffer->GetSize()-loc);
		loc += strlen(temp);

		// Write any other tags...
		WriteTags(buffer, loc);
		
		// Write the closing tag...
		SafeSprintf(temp, MAX_WRITER_BUFFER, "%s</%s>%s", tabs, m_name, WRITER_EOL);
		SafeStrCpy((char *)(buffer->GetBuffer()) + loc,  temp, buffer->GetSize()-loc);
		loc += strlen(temp);
	    }

	    // When there are no attributes, just open and close the tag...
	    else
	    {
		// Open the tag...
		SafeSprintf(temp, MAX_WRITER_BUFFER, "%s<%s>%s", tabs, m_name, WRITER_EOL);
		SafeStrCpy((char *)(buffer->GetBuffer()) + loc,  temp, buffer->GetSize()-loc);
		loc += strlen(temp);

		// Write the other tags...
		WriteTags(buffer, loc);

		// Close the tag...
		SafeSprintf(temp, MAX_WRITER_BUFFER, "%s</%s>%s", tabs, m_name, WRITER_EOL);
		SafeStrCpy((char *)(buffer->GetBuffer()) + loc,  temp, buffer->GetSize()-loc);
		loc += strlen(temp);
	    }

	}
	break;
    case XMLCommentTag:
	{
	    // If the comment field is empty, return...
	    if (m_comment == NULL) return;

	    // Otherwise, format it and write it out...
	    SafeSprintf(temp, MAX_WRITER_BUFFER, "%s<-- %s -->%s", tabs, m_comment, WRITER_EOL);
	    SafeStrCpy((char *)(buffer->GetBuffer()) + loc,  temp, buffer->GetSize()-loc);
	    loc += strlen(temp);
	}
	break;
    case XMLDirectiveTag:
	{
	}
	break;
    case XMLProcInstTag:
	{
	    // If there are no attributes, then don't print out...
	    if (m_attributes.IsEmpty()) return;
	    if (m_name == NULL) return;

	    // Otherwise, format the beginning...
	    SafeSprintf(temp, MAX_WRITER_BUFFER, "%s<?%s ", tabs, m_name);
	    SafeStrCpy((char *)(buffer->GetBuffer()) + loc,  temp, buffer->GetSize()-loc);
	    loc += strlen(temp);

	    // Write out the attributes...
	    WriteAttributes(buffer, loc);

	    // Format the ending...
	    SafeSprintf(temp, buffer->GetSize()-loc, "?>%s", WRITER_EOL);
	    SafeStrCpy((char *)(buffer->GetBuffer()) + loc,  temp, buffer->GetSize()-loc);
	    loc += strlen(temp);	
	}
	break;
    }
}






/* 
 * SetProcessingInstruction
 * ------------------------
 * Make this tag a processing instruction.
 *
 * input:
 * const char *instruction	    - Name of the instruction to use.
 *
 * output:
 * void
 *
 */
void 
XMLWriterTag::SetProcessingInstruction(const char *instruction)
{
    m_type = XMLProcInstTag;
    if (m_name != NULL) delete [] m_name;
    m_name = NULL;
    if (instruction != NULL)
    {
	m_name = new char[strlen(instruction) + 1];
	strcpy(m_name, instruction); /* Flawfinder: ignore */
    }
}






/*
 * WriteAttributes
 * ---------------
 * Write out the attributes for this tag.
 *
 * input:
 * IHXBuffer *buffer		- Buffer to write to.
 * INT32& loc			- Location to where we start writing.
 *
 * output:
 * void
 *
 */
void 
XMLWriterTag::WriteAttributes(IHXBuffer *buffer, INT32& loc)
{
    // If there are no attributes, return...
    if (m_attributes.IsEmpty()) return;

    // Write out the attributes to the buffer...
    LISTPOSITION position = m_attributes.GetHeadPosition();
    while (position != NULL)
    {
	((XMLWriterAttribute *)(m_attributes.GetNext(position)))->Write(buffer, loc);
	if (position != NULL) *(((char *)(buffer->GetBuffer())) + loc++) = '\n';
    }
}





/*
 * WriteTags
 * ---------
 * Write out the tags for this tag.
 *
 * input:
 * IHXBuffer *buffer		- Buffer to write to.
 * INT32& loc			- Location to where we start writing.
 *
 * output:
 * void
 *
 */
void 
XMLWriterTag::WriteTags(IHXBuffer *buffer, INT32& loc)
{
    // If there are no attributes, return...
    if (m_tags.IsEmpty()) return;

    // Get the size and call write on each...
    LISTPOSITION position = m_tags.GetHeadPosition();
    while (position != NULL)
    {
	((XMLWriterTag *)(m_tags.GetNext(position)))->Write(buffer, loc);
    }
}







/*
 * CreateTag
 * ---------
 * Create a tag with the given name, adding it to the child list and return it.
 *
 * input:
 * const char *name	    - Name of the tag.
 *
 * output:
 * XMLWriterTag *	    - Pointer to the newly created tag.
 *
 */
XMLWriterTag *
XMLWriterTag::CreateTag(const char *name)
{
    XMLWriterTag *newTag = new XMLWriterTag;
    HX_ASSERT(newTag != NULL);
    newTag->SetName(name);
    newTag->m_depth = m_depth + 1;
    m_tags.AddTail(newTag);
    return newTag;
}




/*
 * AddAttribute
 * ------------
 * Add an attribute to the tag and then return a pointer to that attribute.
 *
 * input:
 * const char *name	    - Name of the attribute.
 * const char *value	    - Value of the attribute.
 *
 * output:
 * XMLWriterAttribute *	    - Attribute that was added.
 *
 */
XMLWriterAttribute *
XMLWriterTag::AddAttribute(const char *name, const char *value)
{
    // Create an attribute...
    XMLWriterAttribute *newAttribute = new XMLWriterAttribute;
    HX_ASSERT(newAttribute != NULL);

    // Set the name and value...
    newAttribute->SetName(name);
    newAttribute->SetValue(value);
    newAttribute->m_depth = m_depth;

    // Add the attribute...
    m_attributes.AddTail(newAttribute);
    return newAttribute;
}







/* 
 * CreateAttribute
 * ---------------
 * Creates an attribute, adding it to the list of attributes, then returning the pointer to it.
 *
 * input:
 * void
 *
 * output:
 * XMLWriterAttribute *		    - The pointer to the newly created attribute.
 *
 */
XMLWriterAttribute *
XMLWriterTag::CreateAttribute(void)
{
    // Create the attribute...
    XMLWriterAttribute *newAttribute = new XMLWriterAttribute;
    HX_ASSERT(newAttribute != NULL);

    // Add the attribute to the list of attributes...
    m_attributes.AddTail(newAttribute);
    newAttribute->m_depth = m_depth;
    return newAttribute;
}







/*
 * SetComment
 * ----------
 * Set the comment and set the type as a comment.
 *
 * input:
 * const char *comment		- Comment to set.
 *
 * output:
 * void
 *
 */
void
XMLWriterTag::SetComment(const char *comment)
{
    if (m_comment != NULL) delete [] m_comment;
    m_comment = NULL;
    if (comment != NULL)
    {
	m_comment = new char[strlen(comment) + 1];
	strcpy(m_comment, comment); /* Flawfinder: ignore */
    }
    m_type = XMLCommentTag;
}






/* 
 * SetName
 * -------
 * Set the name of this tag.
 *
 * input:
 * const char *name	- Name to set for this tag.
 *
 * output:
 * void
 *
 */
void
XMLWriterTag::SetName(const char *name)
{
    if (m_name != NULL) delete [] m_name;
    m_name = NULL;
    if (name != NULL)
    {
	m_name = new char[strlen(name) + 1];
	strcpy(m_name, name); /* Flawfinder: ignore */
    }
}




