/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: xmlconfig.cpp,v 1.18 2008/01/23 06:07:35 npatil Exp $
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


//  $Id: xmlconfig.cpp,v 1.18 2008/01/23 06:07:35 npatil Exp $

#ifdef _UNIX 
#include <sys/stat.h> 
#endif 

#include "hxtypes.h"
#include "hxcom.h"
#include "looseprs.h"
#include "cbbqueue.h"
#include "hxstrutl.h"
#include "ihxpckts.h"
#include "chxpckts.h"
#include "hxmon.h"	// for registry property type enums
#include "hxerror.h"
#include "xmlconfig.h"
#include "errmsg_macros.h"

#ifdef _WINCE
#include "hlxclib/windows.h"
#endif

#define MAX_TAG_SIZE	32768
#define MAX_ERROR_CHARS 20	// Number of chars to print if an error occurs

static const XMLConfigAlias aliases[] = {
    //Note: Attribute specs here should not have quotes in them!
    //      %0 is replaced with the name of the alias (leftmost column)
    //      %n with the value of the nth attribute
    //      %  args can only appear in the values of attributes
    
    //Alias This Tag    To This         With these 3 args      and these flags
    {"FSMount",		"List",	"Name=FSMount", NULL, NULL,         0},
    {"Plugin",          "List", "Name=%1",      NULL, NULL,         0},
    {"MimeTypes",       "List", "Name=MimeTypes", NULL, NULL,       0},
    {"Type",            "List", "Name=%1",      NULL, NULL,         0},
    {"Ext",             "Var",  NULL,           NULL, NULL,         0},
    {"IPBindings",      "List", "Name=IPBindings", NULL, NULL,      0},
    {"HTTPDeliverable", "List", "Name=HTTPDeliverable", NULL, NULL,      0},
    {"Path",     	"Var",	NULL, NULL, NULL,     0},
    {"ConnectControl",  "List", "Name=ConnectControl", NULL, NULL,  0},
    {"Multicast",	"List", "Name=Multicast", NULL, NULL,0},
    {"Address",         "Var",  NULL,           NULL, NULL,         0},
    {"NetMask",         "Var",  NULL,           NULL, NULL,         0},
    {"AccessLogging",   "List", "Name=AccessLogging", NULL, NULL,   0},
    {"LiveArchive",     "List", "Name=LiveArchive", NULL, NULL,     0},
    {"FarmSplit",       "List", "Name=FarmSplit", NULL, NULL,     0},
    {"Directory",       "List", "Name=%1",      NULL, NULL,         0},
    {"AccessControl",  	"List", "Name=AccessControl", NULL, NULL,     0},
    {"Rule",     	"List", "Name=%1", NULL, NULL,     0},
    {"Access",     	"Var",	NULL, NULL, NULL,     0},
    {"Transmission",    "Var", 	NULL, NULL, NULL,     0},
    {"To",     		"Var",	NULL, NULL, NULL,     0},
    {"From",     	"Var",	NULL, NULL, NULL,     0},
    {"Ports",     	"List", "Name=Ports", NULL, NULL,     0},
    {"Num",     	"Var", NULL, NULL, NULL,     0},
    {"Proxy",     	"List", "Name=Proxy", NULL, NULL,     0},
    {NULL, NULL, NULL, NULL, NULL, 0}
};

#if defined (_WINDOWS ) || defined (_WIN32) || defined(_SYMBIAN)
#define OS_SEPARATOR_CHAR	'\\'
#define OS_SEPARATOR_STRING	"\\"
#elif defined (_UNIX)
#define OS_SEPARATOR_CHAR	'/'
#define OS_SEPARATOR_STRING	"/"
#elif defined (_MACINTOSH) 
#ifdef _MAC_MACHO
#define OS_SEPARATOR_CHAR       '/' 
#define OS_SEPARATOR_STRING     "/" 
#else
#define OS_SEPARATOR_CHAR       ':' 
#define OS_SEPARATOR_STRING     ":" 
#endif
#endif // defined (_WINDOWS ) || defined (_WIN32)

XMLConfig::XMLConfigListNode::~XMLConfigListNode()
{
    if(m_pList)
    {
	delete m_pList;
    }
    if(m_name)
	delete [] m_name;
    if(m_value)
	delete [] m_value;
}

XMLConfig::XMLConfigList::~XMLConfigList()
{
    CHXSimpleList::Iterator i;

    for(i = Begin(); i != End(); ++i)
    {
	XMLConfigListNode* node = (XMLConfigListNode*)(*i);
	delete node;
    }
}

XMLConfig::XMLConfig():m_filename(NULL), m_lRefCount(0), m_pList(NULL),
                       m_ulMajor(0), m_ulMinor(0), m_ActiveSetsOutstanding(0),
                       m_pRegistry(NULL), m_pMessages(NULL), m_szServerversion(NULL),
                       m_pReconfigureResponse(NULL)
{
#ifdef HELIX_FEATURE_CLIENT
    // Client doesn't use this class, but if it doesn, then we need to 
    // fix the direct link to CHXBuffer and use CCF to create IHXBuffer*
    HX_ASSERT(FALSE);
#endif
}

XMLConfig::XMLConfig(IHXRegistry2* pRegistry, IHXErrorMessages* pMessages, const char* szserverversion,
                     UINT32 dwMajor, UINT32 dwMinor)
     : m_filename(0), m_lRefCount(0), m_pList(NULL), m_ulMajor(dwMajor), m_ulMinor(dwMinor)
       , m_ActiveSetsOutstanding(0), m_pReconfigureResponse(NULL)
{
#ifdef HELIX_FEATURE_CLIENT
    // Client doesn't use this class, but if it doesn, then we need to 
    // fix the direct link to CHXBuffer and use CCF to create IHXBuffer*
    HX_ASSERT(FALSE);
#endif

    if (pRegistry)
    {
        m_pRegistry = pRegistry;
        m_pRegistry->AddRef();
    }

    if (pMessages)
    {
        m_pMessages = pMessages;
        m_pMessages->AddRef(); 
    }

    m_szServerversion = new_string(szserverversion);
    
    XMLConfigAlias* alias = (XMLConfigAlias*) &aliases[0];
    char lwr[256]; /* Flawfinder: ignore */
    m_vserver = -1;

    while(alias->from)
    {
	SafeStrCpy(lwr, alias->from, 256);
	strlwr(lwr);
	m_alias_dict.enter(lwr, alias);
	alias++;
    }
}
XMLConfig::~XMLConfig()
{
    delete[] m_szServerversion; 
    HX_RELEASE(m_pReconfigureResponse);
    HX_RELEASE(m_pRegistry);
    HX_RELEASE(m_pMessages); 
}

HX_RESULT
XMLConfig::init(IHXRegistry2* pRegistry, IHXErrorMessages* pMessages, const char* szserverversion,
                     UINT32 dwMajor, UINT32 dwMinor)
{
    m_ulMajor = dwMajor;
    m_ulMinor = dwMinor;
    if (pRegistry)
    {
        m_pRegistry = pRegistry;
        m_pRegistry->AddRef();
    }

    if (pMessages)
    {
        m_pMessages = pMessages;
        m_pMessages->AddRef(); 
    }

    m_szServerversion = new_string(szserverversion);
    
    XMLConfigAlias* alias = (XMLConfigAlias*) &aliases[0];
    char lwr[256]; /* Flawfinder: ignore */
    m_vserver = -1;

    while(alias->from)
    {
	SafeStrCpy(lwr, alias->from, 256);
	strlwr(lwr);
	m_alias_dict.enter(lwr, alias);
	alias++;
    }
    return HXR_OK;
}

STDMETHODIMP
XMLConfig::QueryInterface(REFIID riid, void** ppvObj)
{
    if(IsEqualIID(riid, IID_IUnknown))
    {
        AddRef();
        *ppvObj = (IUnknown*)(IHXRegConfig*)this;
        return HXR_OK;
    }
    else if (IsEqualIID(riid, IID_IHXRegConfig))
    {
	AddRef();
	*ppvObj = (IHXRegConfig*)this;
	return HXR_OK;
    }

    *ppvObj = NULL;
    return HXR_NOINTERFACE;
}

STDMETHODIMP_(UINT32)
XMLConfig::AddRef()
{
    return InterlockedIncrement(&m_lRefCount);    
}

STDMETHODIMP_(UINT32)
XMLConfig::Release()
{
    if (InterlockedDecrement(&m_lRefCount) > 0)
    {
        return m_lRefCount;
    }

    delete this;
    return 0;
}

HX_RESULT
XMLConfig::Write(const char* name, const char* filename)
{
    FILE* outfile = fopen(filename, "w");
    if(!outfile)
    {
	return HXR_INVALID_FILE;
    }

    fprintf(outfile, "<?XML Version=\"1.0\" ?>\n");
    fprintf(outfile, "<!-- Auto generated by %s %d.%d -->\n\n",
            m_szServerversion, m_ulMajor, m_ulMinor);

#ifdef _UNIX
    chmod(filename, 0700);
#endif //UNIX

    return DumpConfig(name, 0, outfile, m_pRegistry);
}

HX_RESULT
XMLConfig::DumpConfig(const char* name, int indent, FILE* outfile,
		      IHXRegistry2* pRegistry)
{
    IHXValues* pValues;
    const char* propName;
    HX_RESULT res;
    UINT32 prop_id;

    if(HXR_OK != pRegistry->GetPropListByName(name,
					  pValues))
	return HXR_FAIL;

    char* point = (char*)strrchr(name, '.');
    if(point)
    {
	if(strcasecmp(point + 1, "XXXBADLIST") == 0)
	    return HXR_FAIL;

	for(int j = 0; j < indent - 1; j++)
	    fprintf(outfile, "  ");
	fprintf(outfile, "<List %s>\n", point + 1);
    }
    
    res = pValues->GetFirstPropertyULONG32(propName, prop_id);
    while(res == HXR_OK)
    {
	HXPropType type = pRegistry->GetTypeById(prop_id);
	switch(type)
	{
	    case PT_COMPOSITE:
		DumpConfig(propName, indent+1, outfile, pRegistry);
		break;
	    case PT_INTEGER:
	    {
		INT32 val;
		if(HXR_OK == pRegistry->GetIntById(prop_id, val))
		{
		    
		    fprintf(outfile, "%*.*s<Var %s=\"%ld\"/>\n",
			    indent * 2, indent * 2, "  ",
			    strrchr(propName, '.') + 1,
			    val);
		}
		break;
	    }
	    case PT_INTREF:
		break;
	    case PT_STRING:
	    {
		IHXBuffer* pBuffer;
		if(HXR_OK == pRegistry->GetStrById(prop_id, pBuffer) && pBuffer)
		{
		    fprintf(outfile, "%*.*s<Var %s=\"%s\"/>\n",
			    indent * 2, indent * 2, "  ",
			    strrchr(propName, '.') + 1,
			    pBuffer->GetBuffer());
		    pBuffer->Release();
		}
		break;
	    }
	    case PT_BUFFER:
	    {
		IHXBuffer* pBuffer;
		pRegistry->GetBufById(prop_id, pBuffer);
		if(pBuffer)
		{
		    fprintf(outfile, "%*.*s<Var %s=\"%s\"/>\n",
			    indent * 2, indent * 2, "  ",
			    strrchr(propName, '.') + 1,
			    pBuffer->GetBuffer());
		    pBuffer->Release();
		}
		break;
	    }
	    case PT_UNKNOWN:
	    default:
		break;
	}

	res = pValues->GetNextPropertyULONG32(propName, prop_id);
    }
    if(point)
	fprintf(outfile, "%*.*s</List>\n", (indent - 1) * 2, (indent-1)*2, "  ");
    fflush(outfile);
    pValues->Release();
    return HXR_OK;
}

void
XMLConfig::ExpandAttribute(XMLTag* tag, const char* attribute)
{
    const char* equals;
    char subst[2048]; /* Flawfinder: ignore */
    const char* a;
    UINT32 arg;
    UINT32 i;

    equals = strchr(attribute, '=');
    // Aliases are hardcoded.  Don't enter wrong ones, dumbass.
    HX_ASSERT(equals);

    tag->new_attribute()->name = new char[equals - attribute + 1];
    strncpy(tag->m_cur_attribute->name, attribute, /* Flawfinder: ignore */
	    equals - attribute);
    tag->m_cur_attribute->name[equals - attribute] = 0;

    // Attributes already present in the tag with the same name
    // as an aliased attribute override the alias.
    for(i = 0; i < tag->m_numAttributes - 1; i++)
    {
	if(tag->attribute(i)->name && 
	   (strcasecmp(tag->attribute(i)->name, 
		       tag->m_cur_attribute->name) == 0))
	{
	    break;
	}
    }
    if(i < tag->m_numAttributes - 1)
    {
	// Found a duplicate attribute with this name, throw out the
	// alias version.
	delete tag->m_cur_attribute;
	tag->m_numAttributes--;
    }
    else if(!strchr(equals, '%'))
    {
	tag->m_cur_attribute->value = new_string(equals+1);
    }
    else
    {
	for(a = equals, i = 0; *a; a++, i++)
	{
	    switch(*a)
	    {
		case '%':
		    arg = *(a+1) - '0';
		    if(arg > 0)
		    {
			if(arg - 1 < tag->m_numAttributes)
			{
			    SafeStrCpy(&subst[i], tag->attribute(arg - 1)->value, 2048-i);
			    i+= strlen(tag->attribute(arg - 1)->value);
			}
			else
			{
			    ERRMSG(m_pMessages,
				   "%s: Alias %s requires at least %d arguments",
				   m_filename,
				   tag->m_name, arg);
			    return;
			}
		    }
		    else
		    {
			SafeStrCpy(&subst[i], tag->m_name, 2048-i);
			i += strlen(tag->m_name);
		    }
		    break;
		default:
		    subst[i] = *a;
	    }
	}
	subst[i] = 0;
	tag->m_cur_attribute->value = new_string(subst);
    }
}

// XMLConfig::Expand
//
// Do alias and loose syntax expansion
HXBOOL
XMLConfig::Expand(XMLTag* tag, CBigByteQueue* queue)
{
    UINT32 i;

    Dict_entry* ent;
    XMLConfigAlias* alias;
    char* lwr = new char[strlen(tag->m_name) + 1];
    HXBOOL ignore = FALSE;

    strcpy(lwr, tag->m_name); /* Flawfinder: ignore */
    strlwr(lwr);
    if((ent = m_alias_dict.find(lwr)) != NULL)
    {
	alias = (XMLConfigAlias*)ent->obj;
	delete [] tag->m_name;
	tag->m_name = new_string(alias->to);
	if(tag->m_type == XMLPlainTag)
	{
	    if(alias->attr1)
	    {
		ExpandAttribute(tag, alias->attr1);
	    }
	    if(alias->attr2)
	    {
		ExpandAttribute(tag, alias->attr2);
	    }
	    if(alias->attr3)
	    {
		ExpandAttribute(tag, alias->attr3);
	    }
	}
	if((alias->flags & AL_TAGIFY) && (tag->m_type != XMLEndTag))
	{
	    UINT32 bytesAvail = queue->GetQueuedItemCount();
	    BYTE* buf = new BYTE[bytesAvail];
	    queue->DeQueue(buf, bytesAvail);
	    HXBOOL isList = FALSE;
	    char ibuf[1024]; /* Flawfinder: ignore */

	    if(!tag->m_need_close && strcasecmp(alias->to, "list") == 0)
	    {
		isList = TRUE;
		ignore = TRUE;
		if(tag->get_attribute("name"))
		    SafeSprintf(ibuf, 1024, "<List Name=\"%s\">",
			    tag->get_attribute("name"));
		else
		    SafeSprintf(ibuf, 1024, "<List Name=\"%s\">",
			    alias->from);
		queue->EnQueue(ibuf, strlen(ibuf));
	    }
	    UINT32 i;
	    for(i = 0; i < tag->m_numAttributes; i++)
	    {
		if (tag->attribute(i)->name)
		{
		    SafeSprintf(ibuf, 1024, "<Var Name=\"%s\" Value=\"%s\"/>",
			   (tag->attribute(i)->name ? tag->attribute(i)->name : ""),
			    tag->attribute(i)->value);
		    queue->EnQueue(ibuf, strlen(ibuf));
		}
	    }
	    if(isList)
	    {
		sprintf(ibuf, "</List>"); /* Flawfinder: ignore */
		queue->EnQueue(ibuf, strlen(ibuf));
	    }
	    queue->EnQueue(buf, bytesAvail);
	}
    }
    delete [] lwr;

    if(tag->m_type != XMLPlainTag || ignore)
	return ignore;

    if(strcasecmp(tag->m_name, "list") == 0)
    {
	if(tag->m_numAttributes < 1)
	{
	    return ignore;
	}
	
	for(i = 0; i < tag->m_numAttributes; i++)
	{
	    if(!tag->attribute(i)->name)
	    {
		tag->attribute(i)->name = new_string("name");
	    }
	    else if(strcasecmp(tag->attribute(i)->name, "name") != 0)
	    {
		ERRMSG(m_pMessages,
		       "%s: Unknown List Attribute %s",
		       m_filename,
		       tag->attribute(i)->name);
		return ignore;
	    }
	}
    }
    else if(strcasecmp(tag->m_name, "var") == 0)
    {
	if(tag->m_numAttributes < 1)
	{
	    ERRMSG(m_pMessages,
		   "%s: Var tag requires at least one attribute",
		   m_filename);
	    return ignore;
	}

	if(tag->m_numAttributes == 1)
	{
	    // Special case, <Var x=y> converts to <Var name=x value=y>
	    tag->new_attribute()->name = new_string("value");
	    tag->attribute(1)->value = tag->attribute(0)->value;
	    tag->attribute(0)->value = tag->attribute(0)->name;
	    if(!tag->attribute(0)->value)
	    {
		char* val = new char[20];
		sprintf(val, "elem%ld", tag->elem); /* Flawfinder: ignore */
		tag->attribute(0)->value = val;
	    }
	    tag->attribute(0)->name = new_string("name");
	}
    }
    else if(strcasecmp(tag->m_name, "server") == 0)
    {
	if(tag->m_numAttributes < 1)
	{
	    ERRMSG(m_pMessages,
		   "%s: Server tag requires at least one attribute",
		   m_filename);
	    return ignore;
	}
	if(!tag->get_attribute("number"))
	{
	    tag->attribute(0)->name = new_string("number");
	}
    }
    return ignore;
}

XMLConfigVarType
XMLConfig::GetVarType(XMLConfigListNode* node)
{
    char* pos = node->m_value;

    if (0 == *pos)
    {
         return CfgVarString;
    }
    for(; *pos && isdigit(*pos); pos++)
	;
    if(*pos == 0 && *(node->m_value) != 0)
    {
	node->m_int = atol(node->m_value);
	return CfgVarInt;
    }
    
    if(strcasecmp(node->m_value, "true") == 0)
    {
	node->m_bool = TRUE;
	return CfgVarBool;
    }
    else if(strcasecmp(node->m_value, "false") == 0)
    {
	node->m_bool = FALSE;
	return CfgVarBool;
    }
    return CfgVarString;
}

void
XMLConfig::StuffRegistry(XMLConfigList* list)
{
    CHXSimpleList::Iterator i;

    char* regname = list->m_parentnode->get_registry_name();
    if(m_pRegistry->GetTypeByName(regname) == PT_UNKNOWN)
    { 
	m_pRegistry->AddComp(regname);
    }
    delete [] regname;

    for(i = list->Begin(); i != list->End(); ++i)
    {
	XMLConfigListNode* node = (XMLConfigListNode*)(*i);
	switch(node->m_type)
	{
	    case CfgVar:
		regname = node->get_registry_name();
		switch(GetVarType(node))
		{
		    case CfgVarInt:
			// printf("%s = %ld\n", regname, node->m_int);
                        if(m_pRegistry->GetTypeByName(regname) == PT_UNKNOWN)
			{
			    m_pRegistry->AddInt(regname, node->m_int);
			}
			else
			{
			    m_pRegistry->SetIntByName(regname, node->m_int);
			}
			break;
		    case CfgVarString:
		    {
			CHXBuffer* pBuffer = new CHXBuffer;
			pBuffer->AddRef();
			pBuffer->Set((UINT8*)node->m_value,
				     strlen(node->m_value) + 1);
			// printf("%s = %s\n", regname, (const char *)pBuffer->GetBuffer());
			if(m_pRegistry->GetTypeByName(regname) == PT_UNKNOWN)
			{
			    m_pRegistry->AddStr(regname, pBuffer);
			}
			else
			{
			    m_pRegistry->SetStrByName(regname, pBuffer);
			}
			pBuffer->Release();
			break;
		    }
		    case CfgVarBool:
			// printf("%s = %ld\n", regname, node->m_bool);
			if(m_pRegistry->GetTypeByName(regname) == PT_UNKNOWN)
			{
			    m_pRegistry->AddInt(regname, node->m_bool);
			}
			else
			{
			    m_pRegistry->SetIntByName(regname, node->m_bool);
			}
			break;
		}
		delete [] regname;
		break;
	    case CfgList:
		StuffRegistry(node->m_pList);
		break;
	}
    }
}


// 
// Method: Read
//
// Parameters:
//
//	filename:    The file to be read.
//	pWinRegKey:  The Windows registry key to be filled in (can be
//		  NULL if no registry writing is needed).
//	pServRegKey: The Server registry key to be filled in.
//  bStuffRegistry: Set bStuffRegistry parameter to FALSE if the 
//      values read by the function should not be added into the registry.
//      bStuffRegistry has a default value of TRUE.
// 
// Notes: the bIncludedFile is for internal recursive use only.  Do not 
// set it when calling from outside this object.
HX_RESULT
XMLConfig::Read(char* filename, 
		char* pServRegKey, 
		HXBOOL  bIncludedFile,
        HXBOOL bStuffRegistry)
{
    HX_RESULT hResult = HXR_OK;
    FILE* fp=0;
    XMLParser parser;
    XMLTag*   tag = 0;
    CBigByteQueue queue(MAX_TAG_SIZE);
    int l=0;
    HXBOOL filedone = FALSE;
    UINT32 indent = 0;

    HX_ASSERT(filename && strlen(filename));

    HX_VECTOR_DELETE(m_filename);
    m_filename = new_string(filename);

    fp = fopen(filename, "r");
    if (!fp)
    {
	ERRMSG(m_pMessages, "%s: file not found", filename);
	return HXR_FILE_NOT_FOUND;
    }

    if(!m_pList)
    {
	m_pList = new XMLConfigList;
	m_pList->m_parentnode = new XMLConfigListNode;
	m_pList->m_parentnode->m_name = new_string(pServRegKey);
	m_pList->m_parentnode->m_parent = NULL;
        m_pList->m_parentnode->m_type = CfgList;
        m_pList->m_parentnode->m_pList = m_pList;
    }

    //Remove all the elements in the stack. If not, the error in one file gets carried over to another.
    while(m_pListStack.Pop());

    // read the config file into the byte queue
    INT32 nBufSize = 16384;
    BYTE* pBuf = new BYTE[nBufSize];
    while (!filedone && hResult == HXR_OK)
    {
        l = fread(pBuf, 1, nBufSize, fp);
        if (l > 0)
        {
	    if (!queue.EnQueue(pBuf, l))
            {
                if (!queue.Grow(l))
                {
                    ERRMSG(m_pMessages,
                           "error expanding data structure while parsing %s",
                           filename);
                    hResult = HXR_FAIL;
                }
                if (!queue.EnQueue(pBuf, l))
                {
                    ERRMSG(m_pMessages,
                           "unknown error while parsing %s",
                           filename);
                    hResult = HXR_FAIL;
                }
            }
        }
        else
        {
    	    filedone = TRUE;
        }
    }

    UINT32 bytesUsed=0;
    UINT32 bytesAvail=0;
    BYTE* p=0;
    while (hResult == HXR_OK && (bytesAvail = queue.GetQueuedItemCount()) > 0)
    {
	if (bytesAvail > nBufSize)
        {
            HX_VECTOR_DELETE(pBuf);
            pBuf = new BYTE[bytesAvail];
            nBufSize = bytesAvail;
        }
	p = pBuf;

	queue.DeQueue(pBuf, bytesAvail);
	bytesUsed = bytesAvail;

        HX_DELETE(tag);
	XMLParseResult res = parser.Parse((const char*&)p, bytesAvail, tag);
	queue.EnQueue(p, bytesAvail - (p - pBuf));

	switch(res)
	{
	    case XMLPNoClose:
		// A tag opener was found, but no closer yet.
		if (filedone)
		{
		    pBuf[bytesAvail - 1] = '\0';
		    if (bytesAvail > MAX_ERROR_CHARS)
		    {
			pBuf[MAX_ERROR_CHARS - 1] = '\0';
		    }
		    ERRMSG(m_pMessages,
		    "%s: Missing close for tag '%s'...", m_filename, pBuf);
		    hResult = HXR_FAIL;
		}
		break;
	    case XMLPNoTagType:
		*p = 0;
		ERRMSG(m_pMessages,
		       "%s: Badly formed tag '%s'", m_filename, pBuf);
		break;
	    case XMLPBadAttribute:
		*p = 0;
		ERRMSG(m_pMessages,
		       "%s: Badly formed attribute in '%s'", m_filename,
		       pBuf);
		break;
	    case XMLPPlainText:
		//Some text.  What do we care?
		break;
	    case XMLPBadEndTag:
		*p = 0;
		ERRMSG(m_pMessages,
		       "%s: Unexpected end tag '</%s>'", m_filename, tag->m_name);
		hResult = HXR_FAIL;
		break;
	    case XMLPComment:
		// A comment, fine.
		if(tag != NULL)
		{
		    //Some kind of comment directive
		    XMLAttribute* pAttr = tag->attribute(0);
		    if(pAttr &&
			pAttr->name)
		    {
			if(strcasecmp(pAttr->name, "include") == 0)
			{
			    char* path;
			    char* newfile = new char[1024];
			    if(!strchr(tag->attribute(0)->value, 
				       OS_SEPARATOR_CHAR) &&
			       ((path = strrchr(m_filename, OS_SEPARATOR_CHAR))
				!= NULL))
			    {
				path++;
                                if (path - m_filename < 1024)
                                {
                                    strncpy(newfile, m_filename, path - m_filename);
                                    newfile[path - m_filename] = '\0';
                                }
				SafeStrCat(newfile, tag->attribute(0)->value, 1024);
			    }
			    else
			    {
				SafeStrCpy(newfile, tag->attribute(0)->value, 1024);
			    }
			    char* filename = m_filename;
			    m_filename = NULL;
			    Read(newfile, pServRegKey, TRUE);
			    delete[] m_filename;
			    m_filename = filename;
                            delete[] newfile;
			}
		    }
		}
		break;
	    case XMLPDirective:
		// A directive.  We don't handle any yet.
		break;
	    case XMLPProcInst:
		// A processing Instruction, whee!
		break;
	    case XMLPTag:
	    {
		if(Expand(tag, &queue))
		    break;

		XMLConfigListNode* node = new XMLConfigListNode;
		node->m_parent = m_pList->m_parentnode;
		node->m_vserver = m_vserver;

		if(tag->m_need_close)
		{
		    if(strcasecmp(tag->m_name, "list") == 0)
		    {
			const char* name = tag->get_attribute("name");
			if(!name)
			{
			    ERRMSG(m_pMessages,
				   "%s: List tag requires a name attribute",
				   m_filename);
			    // Process anyway, give it a dummy name
			    name = "XXXBADLIST";
			}
			node->m_type = CfgList;
			node->m_name = new_string(name);
			node->m_num = tag->elem;
			m_pList->AddTail(node);

			m_pListStack.Push(m_pList);
			node->m_pList = new XMLConfigList;
			m_pList = node->m_pList;
			m_pList->m_parentnode = node;
		    }
		    else if(strcasecmp(tag->m_name, "server") == 0)
		    {
			const char* server = tag->get_attribute("number");
			if(!server)
			{
			    ERRMSG(m_pMessages,
				   "%s: Server tag requires a number, defaulting to 0",
				   m_filename);
			    m_vserver = 0;
			}
			else
			{
			    char vserver_str[64]; /* Flawfinder: ignore */
			    m_vserver = atol(server);
			    sprintf(vserver_str, "server%ld", m_vserver); /* Flawfinder: ignore */
			    if(m_pRegistry->GetTypeByName(vserver_str) == PT_UNKNOWN)
			    {
				m_pRegistry->AddComp(vserver_str);
				sprintf(vserver_str, "server%ld.config", /* Flawfinder: ignore */
					m_vserver);
				m_pRegistry->AddComp(vserver_str);
			    }
			}
		    }
		}
		else if(tag->m_type != XMLEndTag)
		{
		    const char* name = tag->get_attribute("name");
		    const char* value = tag->get_attribute("value");
		    if(name && value)
		    {
			node->m_type  = CfgVar;
			node->m_name  = new_string(name);
			node->m_value = new_string(value);
			node->m_num   = tag->elem;
			m_pList->AddTail(node);
		    }
		}
		else
		{
		    // An End Tag
		    if(strcasecmp(tag->m_name, "list") == 0)
		    {
			m_pList = (XMLConfigList*)m_pListStack.Pop();
		    }
		    else if(strcasecmp(tag->m_name, "server") == 0)
		    {
			if(m_vserver < 0)
			{
			    ERRMSG(m_pMessages,
				   "%s: </Server> Tag with no matching <Server>",
				   m_filename);
			}
			else
			{
			    m_vserver = -1;
			}
		    }
		}
		break;
	    }
	}
    }

    if(hResult == HXR_OK && !bIncludedFile)
    {
        //bStuffRegistry is set to FALSE when the values read are not 
        //to be added into the registry and retained in the m_pList member variable.
        //Reading .uas files (user agent settings) is one place where it is set to FALSE.
        if(bStuffRegistry)
        {
            StuffRegistry(m_pList);
            delete m_pList->m_parentnode;
        }
    }

    if (hResult == HXR_OK && !m_pListStack.IsEmpty())
    {
        ERRMSG(m_pMessages,
	   "%s: <List> tag with no matching </List>",
	   m_filename);
	hResult = HXR_FAIL;
    }

    // cleanup
    if (fp) fclose(fp);
    HX_VECTOR_DELETE(pBuf);
    HX_DELETE(tag);

    return hResult;
}

char*
XMLConfig::XMLConfigListNode::get_registry_name(INT32 vserver)
{
    char* name;
    if(m_parent)
    {
	char* parentname = m_parent->get_registry_name(
	    vserver < 0 ? m_vserver : vserver);
	name = new char[strlen(parentname) + strlen(m_name) + 2];
	sprintf(name, "%s.%s", parentname, m_name); /* Flawfinder: ignore */
	delete [] parentname;
	return name;
    }
    else
    {
	if(vserver >= 0)
	{
	    name = new char[26 + strlen(m_name) + 2];
	    sprintf(name, "server%ld.%s", vserver, m_name); /* Flawfinder: ignore */
	}
	else
	{
	    name = new_string(m_name);
	}
	return name;
    }
}

HX_RESULT
XMLConfig::WriteToFile(const char* pKeyName, const char* pFileName)
{
    HXBOOL bCreateFileIfNotFound = FALSE;
    return WriteToFile(pKeyName, NULL, pFileName, bCreateFileIfNotFound);
}

///XXXPM This func does not correctly handle the original file not
///being there.
HX_RESULT
XMLConfig::WriteToFile(const char* pKeyName, const char* pChildKeyName, const char* pFileName, HXBOOL bCreateFileIfNotFound)
{
    UINT32 ulListsToIgnore = 0;
    /*
     * Make sure we have a valid file name to use.
     */
    if (!pFileName)
    {
	return HXR_FAIL;
    }

    /*
     * Copy filename to filename.bak.
     */
    char *pszBackupFileName = new char[strlen(pFileName) + strlen(".bak") + 1];
    sprintf(pszBackupFileName, "%s.bak", pFileName); /* Flawfinder: ignore */
    FILE* fpOld = fopen(pFileName, "r");
    if (!fpOld && !bCreateFileIfNotFound )
    {
        HX_VECTOR_DELETE(pszBackupFileName);
        return HXR_FAIL;
    }

    INT32 nBufSize=16384;
    BYTE* pBuf = new BYTE[nBufSize];

    //Do not create backup if the file does not exist.
    //This happens when creating a new User Agent Settings file
    //using Helix Server Admin GUI.
    if (fpOld)
    {
        FILE* fpBackup = fopen(pszBackupFileName, "w");
        if (!fpBackup)
        {
            //Failed to create a backup file.
            HX_VECTOR_DELETE(pszBackupFileName);
            fclose(fpOld);
            return HXR_FAIL;
        }
        int iGot = 0;
        while (!feof(fpOld))
        {
            iGot = fread(pBuf, sizeof(char), nBufSize, fpOld);
            if (iGot)
            {
                fwrite(pBuf, sizeof(char), iGot, fpBackup);
            }
        }
        fclose(fpOld);
        fclose(fpBackup);

#ifdef _UNIX
        chmod(pszBackupFileName, 0700);
#endif //UNIX

    }

    CBigByteQueue queue(MAX_TAG_SIZE);
    HX_RESULT hResult = HXR_OK;
    HXBOOL filedone = 0;

    fpOld = fopen(pszBackupFileName, "r");

    HX_VECTOR_DELETE(pszBackupFileName);

    FILE* fpNew = fopen(pFileName, "w");
    if (!fpNew)
    {
        return HXR_FAIL;
    }

    if (fpOld)
    {
        //Read from backup file.
        while (!filedone && hResult == HXR_OK)
        {
            int l = fread(pBuf, 1, nBufSize, fpOld);
            if (l > 0)
            {
                if (!queue.EnQueue(pBuf, l))
                {
                    if (!queue.Grow(l))
                    {
                        ERRMSG(m_pMessages,
                            "error expanding data structure while parsing");
                        hResult = HXR_FAIL;
                    }
                    if (!queue.EnQueue(pBuf, l))
                    {
                        ERRMSG(m_pMessages,
                            "unknown error while parsing");
                        hResult = HXR_FAIL;
                    }
                }
            }
            else
            {
                filedone = TRUE;
            }
        }
        fclose(fpOld);
        fpOld=0;
    }

    XMLTag*   tag = 0;
    XMLTag*   wspacetag = 0;
    XMLParser parser;

    /*
     * Files are all aready.  Now we need to build our config levels list.
     */
    CHXSimpleList levlist;
    /*
     *  Add for level 0.
     */
    CHXSimpleList* pNewList = new CHXSimpleList;
    IHXValues* pValues = 0;

    if (pChildKeyName && pChildKeyName[0])
    {
        //Need to write only the given child key (i.e. pChildKeyName) 
        //into the output file.
		NEW_FAST_TEMP_STR(pPropName, MAX_REGISTRY_NAME, strlen(pKeyName) + strlen(pChildKeyName) + 2);
        sprintf(pPropName, "%s.%s", pKeyName, pChildKeyName);
        HXPropType propType = m_pRegistry->GetTypeByName(pPropName);
		DELETE_FAST_TEMP_STR(pPropName);

        XMLPropInfo* pInfo = new XMLPropInfo;
        pInfo->m_pName = new char[strlen(pChildKeyName) + 1];
        strcpy(pInfo->m_pName, pChildKeyName);
        pInfo->m_Type = propType;
        pNewList->AddHead((void*)pInfo);
    }
    else
    {
        //Need to write all keys under the pKeyName from registry.
        _AddPropsToList(pNewList, pKeyName, m_pRegistry);
    }

    XMLConfigString curr_level;
    curr_level.AddLevel(pKeyName);

    UINT32 bytesUsed=0;
    UINT32 bytesAvail=0;
    BYTE* p=0;

    while (hResult == HXR_OK && (bytesAvail = queue.GetQueuedItemCount()) > 0)
    {
	if (bytesAvail > nBufSize)
        {
            HX_VECTOR_DELETE(pBuf);
            pBuf = new BYTE[bytesAvail];
            nBufSize = bytesAvail;
        }
	p = pBuf;

	queue.DeQueue(pBuf, bytesAvail);
	bytesUsed = bytesAvail;

        HX_DELETE(tag);
	XMLParseResult res = parser.Parse((const char*&)p, bytesAvail, tag);
	queue.EnQueue(p, bytesAvail - (p - pBuf));

	switch(res)
	{
	    case XMLPNoClose:
		// A tag opener was found, but no closer yet.
		if (filedone)
		{
		    pBuf[bytesAvail - 1] = '\0';
		    if (bytesAvail > MAX_ERROR_CHARS)
		    {
			pBuf[MAX_ERROR_CHARS - 1] = '\0';
		    }
		    ERRMSG(m_pMessages,
		    "%s: Missing close for tag %s...", m_filename, pBuf);
		    hResult = HXR_FAIL;
		}
		break;
	    case XMLPNoTagType:
		*p = 0;
		ERRMSG(m_pMessages,
		       "%s: Badly formed tag %s", m_filename, pBuf);
		break;
	    case XMLPBadAttribute:
		*p = 0;
		ERRMSG(m_pMessages,
		       "%s: Badly formed attribute in %s", m_filename,
		       pBuf);
		break;
	    case XMLPPlainText:
		/* 
		 * Just plaintext.  Dump this out directly.
		 */
		if (wspacetag)
		{
		    delete wspacetag;
		}
		wspacetag = tag;
		tag = 0;
		/*
		fwrite(tag->m_cur_attribute->value, sizeof(char),
		    strlen(tag->m_cur_attribute->value), fpNew);
		    */
		break;
	    case XMLPBadEndTag:
		*p = 0;
		ERRMSG(m_pMessages,
		       "%s: Unexpected tag %s", m_filename, pBuf);
		break;
	    case XMLPComment:
		/*
		 * Just a comment.  Dump it out.  Comment lives from
		 * p to pBuf.
		 */
		if (wspacetag)
		{
		    fwrite(wspacetag->m_cur_attribute->value, sizeof(char),
			strlen(wspacetag->m_cur_attribute->value), fpNew);
		    delete wspacetag;
		    wspacetag = 0;
		}

		fwrite(pBuf, sizeof(char), p - pBuf, fpNew);
		break;
	    case XMLPDirective:
		// A directive.  We don't handle any yet.
		break;
	    case XMLPProcInst:
                /*
		 * Just dump processing instructions as we read them.
		 */
		if (wspacetag)
		{
		    fwrite(wspacetag->m_cur_attribute->value, sizeof(char),
			strlen(wspacetag->m_cur_attribute->value), fpNew);
		    delete wspacetag;
		    wspacetag = 0;
		}

		fwrite(pBuf, sizeof(char), p - pBuf, fpNew);
		break;
	    case XMLPTag:
	    {
		if(Expand(tag, &queue))
		    break;

		if(tag->m_need_close)
		{
		    if(strcasecmp(tag->m_name, "list") == 0)
		    {
			if (ulListsToIgnore)
			{
			    ulListsToIgnore++;
			}
			else
			{
			    const char* name = tag->get_attribute("name");
			    if(!name)
			    {
				ERRMSG(m_pMessages,
				       "%s: List tag requires a name attribute",
				       m_filename);
				// Process anyway, give it a dummy name
				name = "XXXBADLIST";
			    }
			    /*
			     * Remove current tag from list, push current list,
			     * create new list, add next level's props.
			     */
			    _RemovePropFromList(pNewList, name);
			    curr_level.AddLevel(name);
			    if (!_PropExists(&curr_level, m_pRegistry))
			    {
				curr_level.RemoveLevel();
				ulListsToIgnore++;
			    }
			    else
			    {
				levlist.AddHead((void*)pNewList);
				if (wspacetag)
				{
				    fwrite(wspacetag->m_cur_attribute->value, sizeof(char),
					strlen(wspacetag->m_cur_attribute->value), fpNew);
				    delete wspacetag;
				    wspacetag = 0;
				}

				fprintf(fpNew, "<List Name=\"%s\">", name);
				pNewList = new CHXSimpleList;
				_AddPropsToList(pNewList, curr_level.CharStar(), m_pRegistry);
			    }
			}
			
		    }
		}
		else if(tag->m_type != XMLEndTag)
		{
		    if (!ulListsToIgnore)
		    {
			const char* name = tag->get_attribute("name");
			const char* value = tag->get_attribute("value");
			if (name)
			{
			    curr_level.AddLevel(name);
			    _RemovePropFromList(pNewList, name);
			    /*
			     * Make sure this prop is still set.
			     */
			    if (_PropExists(&curr_level, m_pRegistry))
			    {
				if (wspacetag)
				{
				    fwrite(wspacetag->m_cur_attribute->value, sizeof(char),
					strlen(wspacetag->m_cur_attribute->value), fpNew);
				    delete wspacetag;
				    wspacetag = 0;
				}

				char* p = _GetPropValueString(curr_level.CharStar(), m_pRegistry);

				if (p)
				{
				    fprintf(fpNew, "<Var %s=\"%s\"/>",
					name,
					p);
				    delete[] p;
				}
			    }
			    curr_level.RemoveLevel();
			}
		    }
		}
		else
		{
		    // An End Tag
		    if(strcasecmp(tag->m_name, "list") == 0)
		    {
			if (ulListsToIgnore)
			{
			    ulListsToIgnore --;
			}
			else
			{
			    /*
			     * Need to add the stuff at this level that was
			     * was not in the config file.
			     */
			    if (pNewList->GetCount() == 0)
			    {
				if (wspacetag)
				{
				    fwrite(wspacetag->m_cur_attribute->value, sizeof(char),
					strlen(wspacetag->m_cur_attribute->value), fpNew);
				}
			    }
			    else
			    {
				fprintf(fpNew, "\n");
				AppendPropsToFile(fpNew, curr_level, pNewList,
				    4, m_pRegistry, pKeyName);
				_IndentFile(fpNew, 4, curr_level, pKeyName);
			    }
			    delete wspacetag;
			    wspacetag = 0;

			    fprintf(fpNew, "</List>");
			    curr_level.RemoveLevel();
			    delete pNewList;
			    pNewList = (CHXSimpleList*)levlist.RemoveHead();
			}
		    }
		}
		break;
	    }
	}

	// Give up if we've lost all hope
	if (HXR_OK != hResult)
	{
	    break;
	}
    }
    if (hResult == HXR_OK && wspacetag)
    {
	fwrite(wspacetag->m_cur_attribute->value, sizeof(char),
	    strlen(wspacetag->m_cur_attribute->value), fpNew);
	delete wspacetag;
	wspacetag = 0;
    }

    /*
     * Ok, done with the whole file. Make sure that all of the base level stuff
     * gets added.
     */
     //XXXPM Calculate this later.
    if (hResult == HXR_OK)
    {
        int indent = 4;
        AppendPropsToFile(fpNew, curr_level, pNewList, indent, m_pRegistry, pKeyName);
    }

    // cleanup
    if (fpNew) fclose(fpNew);
    if (fpOld) fclose(fpOld);
    HX_DELETE(pNewList);
    HX_VECTOR_DELETE(pBuf);
    HX_DELETE(tag);

    return hResult;
}


void
XMLConfig::_AddPropsToList(CHXSimpleList* pList, const char* pName,
			  IHXRegistry2* preg)
{
    IHXValues* pValues = 0;
    UINT32 ul;
    const char* pPropName;
    preg->GetPropListByName(pName, pValues);

    if (pValues)
    {
	if (HXR_OK == pValues->GetFirstPropertyULONG32(pPropName, ul))
	{
	    XMLPropInfo* pInfo = new XMLPropInfo;
	    pInfo->m_pName = new char[strlen(pPropName) + 1 - strlen(pName)];
	    strcpy(pInfo->m_pName, &(pPropName[strlen(pName) + 1])); /* Flawfinder: ignore */
	    pInfo->m_Type = preg->GetTypeByName(pPropName);
	    pList->AddHead((void*)pInfo);

	    while (HXR_OK == pValues->GetNextPropertyULONG32(pPropName, ul))
	    {
		XMLPropInfo* pInfo = new XMLPropInfo;
		pInfo->m_pName = new char[strlen(pPropName) + 1 - strlen(pName)];
		strcpy(pInfo->m_pName, &(pPropName[strlen(pName) + 1]));
		pInfo->m_Type = preg->GetTypeByName(pPropName);
		pList->AddHead((void*)pInfo);
	    }
	}
	pValues->Release();
    }
}

void
XMLConfig::_RemovePropFromList(CHXSimpleList* pList, const char* pName)
{
    LISTPOSITION pos;
    pos = pList->GetHeadPosition();

    XMLPropInfo* pProp;

    
    while (pos)
    {
	pProp = (XMLPropInfo*)pList->GetAt(pos);
	if (!pProp)
	{
	    return;
	}
	if (!strcasecmp(pProp->m_pName, pName))
	{
	    pList->RemoveAt(pos);
	    delete pProp;
	    return;
	}
	pList->GetNext(pos);

    }
}

char*
XMLConfig::_GetPropValueString(const char* pName, IHXRegistry2* pReg)
{
    int vartype;
    IHXBuffer* pBuffer;
    INT32 l;
    char* ret;

    vartype = pReg->GetTypeByName(pName);
    if(vartype == PT_INTEGER)
    {
	if(HXR_OK == pReg->GetIntByName(pName, l))
	{
	    char num[32]; /* Flawfinder: ignore */
	    sprintf(num, "%d", l); /* Flawfinder: ignore */
	    ret = new char[strlen(num) + 1];
	    strcpy(ret, num); /* Flawfinder: ignore */
	    return ret;
	}
	else
	{
	    return 0;
	}
    }
    else if(vartype == PT_STRING)
    {
	if(HXR_OK == pReg->GetStrByName(pName,
	    pBuffer) && pBuffer)
	{
	    ret = new char[pBuffer->GetSize()+1];
	    strcpy(ret, (const char*)pBuffer->GetBuffer()); /* Flawfinder: ignore */
	    pBuffer->Release();
	    return ret;
	}
	else
	{
	    return 0;
	}
    }
    return 0;
}

void
XMLConfig::_CleanList(CHXSimpleList* pList)
{
    XMLPropInfo* pInfo;
    while (!pList->IsEmpty())
    {
	pInfo = (XMLPropInfo*)pList->RemoveHead();
	delete pInfo;
    }
}

void
XMLConfig::AppendPropsToFile(FILE* fp, XMLConfigString level,
    CHXSimpleList* pList, int indent_per_level,
    IHXRegistry2* hxreg, const char* pBase)
{
    XMLPropInfo* pInfo = 0;
    while (!pList->IsEmpty())
    {
	pInfo = (XMLPropInfo*)pList->RemoveHead();
	level.AddLevel(pInfo->m_pName);
	_AppendPropToFile(fp, level, indent_per_level, hxreg, pBase);
	level.RemoveLevel();
	delete pInfo;
    }
}

void
XMLConfig::_IndentFile(FILE* fp, int indent_per_level,
			 XMLConfigString level,
			 const char* pBase)
{
    int dots_to_ignore = 1;
    INT32 i;
    const char* pc;
    pc = pBase;
    while (*pc)
    {
	if (*pc == '.')
	{
	    dots_to_ignore ++;
	}
	pc++;
    }
    pc = level.CharStar();
    while (*pc)
    {
	if (*pc == '.')
	{
	    if (dots_to_ignore)
	    {
		dots_to_ignore--;
	    }
	    else
	    {
		for (i = 0; i < indent_per_level; i++)
		{
		    fprintf(fp, " ");
		}
	    }
	}
	pc ++;
    }
}

void
XMLConfig::_AppendPropToFile(FILE* fp, XMLConfigString level,
			     int indent_per_level,
			     IHXRegistry2* hxreg,
			     const char* pBase)
{
    /*
     * Indent a proper amount.
     */
    INT32 i;
    IHXBuffer* pBuf = 0;

    _IndentFile(fp, indent_per_level, level, pBase);

    HXPropType type = hxreg->GetTypeByName(level.CharStar());
    switch (type)
    {
    case PT_COMPOSITE:
	fprintf(fp, "<List Name=\"%s\">\n", level.Top());
	IHXValues* pValues;
	if (HXR_OK == hxreg->GetPropListByName(level.CharStar(),
	    pValues) && pValues)
	{
	    HX_RESULT res;
	    UINT32 ul;
	    const char* pName;
	    res = pValues->GetFirstPropertyULONG32(pName, ul);
	    while (res == HXR_OK)
	    {
		const char* pc = pName + strlen(pName);
		while (pc > pName)
		{
		    pc--;
		    if (*pc == '.')
		    {
			pc++;
			level.AddLevel(pc);
			_AppendPropToFile(fp, level, indent_per_level, hxreg, pBase);
			level.RemoveLevel();
			res = pValues->GetNextPropertyULONG32(pName, ul);
			break;
		    }
		}
	    }
	    pValues->Release();
	}
	_IndentFile(fp, indent_per_level, level, pBase);
	fprintf(fp, "</List>\n");
	break;

    case PT_INTEGER:
	if (HXR_OK == hxreg->GetIntByName(level.CharStar(), i))
	{
	    fprintf(fp, "<Var %s=\"%ld\"/>\n", level.Top(),
		i);
	}
	break;

    case PT_STRING:
	if (HXR_OK == hxreg->GetStrByName(level.CharStar(), pBuf)
	    && pBuf)
	{
	    fprintf(fp, "<Var %s=\"%s\"/>\n", level.Top(),
		(const char*)pBuf->GetBuffer());
	    pBuf->Release();
	    pBuf = 0;
	}
	break;
    }
}

int
XMLConfig::_PropExists(XMLConfigString* p, IHXRegistry2* preg)
{
    return (preg->GetPropStatusByName(p->CharStar()) == HXR_OK) ? TRUE : FALSE;
}

/*
 * IHXRegConfig::WriteKey
 */
STDMETHODIMP
XMLConfig::WriteKey(const char* pKeyName)
{
    /*
     * If there is a filename then we got it from a file.
     */
    if (m_filename)
    {
	WriteToFile(pKeyName, m_filename);
	return HXR_OK;
    }

    HX_ASSERT(0);
    return HXR_FAIL;
}

HX_RESULT
XMLConfig::Reconfigure(IHXReconfigServerResponse* pResp)
{
    if (m_pReconfigureResponse)
    {
	return HXR_UNEXPECTED;
    }
    m_pReconfigureResponse = pResp;
    m_pReconfigureResponse->AddRef();

    /*
     * If they started from a config file, then reload
     * from the config file.
     */
    if (m_filename)
    {
	return Reconfigure(m_filename);
    }

    /*
     * If we got here there was a booboo
     */
    m_pReconfigureResponse->ReconfigServerDone(HXR_OK, 0, 0);
    m_pReconfigureResponse->Release();
    m_pReconfigureResponse = 0;

    return HXR_OK;
}

HX_RESULT
XMLConfig::Reconfigure(const char* pFileName)
{
    if (!pFileName)
    {
	return HXR_FAIL;
    }

    FILE* fp = fopen(pFileName, "r");
    if (!fp)
    {
	return HXR_FAIL;
    }

    /*
     * Pretend that there is one outstanding just to keep
     * the done methods from getting called, seeing a 0,
     * and sending the response until the end of this 
     * func.
     */
    m_ActiveSetsOutstanding++;

    const char* pKeyName = "config";
    int l = 0;
    HXBOOL filedone = 0;
    XMLTag*   tag = 0;
    CBigByteQueue queue(MAX_TAG_SIZE);
    XMLParser parser;
    HX_RESULT hResult = HXR_OK;

    /*
     * Files are all aready.  Now we need to build our config levels list.
     */
    CHXSimpleList levlist;
    /*
     *  Add for level 0.
     */
    CHXSimpleList* pNewList = new CHXSimpleList;
    IHXValues* pValues = 0;

    _AddPropsToList(pNewList, pKeyName, m_pRegistry);
    XMLConfigString curr_level;
    curr_level.AddLevel(pKeyName);

    // read the config file into the byte queue
    INT32 nBufSize = 16384;
    BYTE* pBuf = new BYTE[nBufSize];
    while (!filedone && hResult == HXR_OK)
    {
        l = fread(pBuf, 1, nBufSize, fp);
        if (l > 0)
        {
            if (!queue.EnQueue(pBuf, l))
            {
                if (!queue.Grow(l))
                {
                    ERRMSG(m_pMessages,
                           "error expanding data structure while parsing %s",
                           pFileName);
                    hResult = HXR_FAIL;
                }
                if (!queue.EnQueue(pBuf, l))
                {
                    ERRMSG(m_pMessages,
                           "unknown error while parsing %s",
                           pFileName);
                    hResult = HXR_FAIL;
                }
            }
	}
	else
	{
            filedone = TRUE;
	}
    }

    UINT32 bytesUsed=0;
    UINT32 bytesAvail=0;
    BYTE* p=0;
    while (hResult == HXR_OK && (bytesAvail = queue.GetQueuedItemCount()) > 0)
    {
	if (bytesAvail > nBufSize)
        {
            HX_VECTOR_DELETE(pBuf);
            pBuf = new BYTE[bytesAvail];
            nBufSize = bytesAvail;
        }
	p = pBuf;

	queue.DeQueue(pBuf, bytesAvail);
	bytesUsed = bytesAvail;

        HX_DELETE(tag);
	XMLParseResult res = parser.Parse((const char*&)p, bytesAvail, tag);
	queue.EnQueue(p, bytesAvail - (p - pBuf));

	switch(res)
	{
	    case XMLPNoClose:
		// A tag opener was found, but no closer yet.
		if (filedone)
		{
		    pBuf[bytesAvail - 1] = '\0';
		    if (bytesAvail > MAX_ERROR_CHARS)
		    {
			pBuf[MAX_ERROR_CHARS - 1] = '\0';
		    }
		    ERRMSG(m_pMessages,
		    "%s: Missing close for tag %s...", m_filename, pBuf);
		    hResult = HXR_FAIL;
		}
		break;
	    case XMLPNoTagType:
		*p = 0;
		ERRMSG(m_pMessages,
		       "%s: Badly formed tag %s", m_filename, pBuf);
		break;
	    case XMLPBadAttribute:
		*p = 0;
		ERRMSG(m_pMessages,
		       "%s: Badly formed attribute in %s", m_filename,
		       pBuf);
		break;
	    case XMLPPlainText:
		break;
	    case XMLPBadEndTag:
		*p = 0;
		ERRMSG(m_pMessages,
		       "%s: Unexpected tag %s", m_filename, pBuf);
		break;
	    case XMLPComment:
		break;
	    case XMLPDirective:
		// A directive.  We don't handle any yet.
		break;
	    case XMLPProcInst:
		// A processing Instruction, whee!
		break;
	    case XMLPTag:
	    {
		if(Expand(tag, &queue))
		    break;

		if(tag->m_need_close)
		{
		    if(strcasecmp(tag->m_name, "list") == 0)
		    {
			const char* name = tag->get_attribute("name");
			if(!name)
			{
			    ERRMSG(m_pMessages,
				   "%s: List tag requires a name attribute",
				   m_filename);
			    // Process anyway, give it a dummy name
			    name = "XXXBADLIST";
			}
			/*
			 * Remove current tag from list, push current list,
			 * create new list, add next level's props.
			 */
			_RemovePropFromList(pNewList, name);
			curr_level.AddLevel(name);
			levlist.AddHead((void*)pNewList);
			pNewList = new CHXSimpleList;
			_AddPropsToList(pNewList, curr_level.CharStar(), m_pRegistry);
			
		    }
		}
		else if(tag->m_type != XMLEndTag)
		{
		    const char* name = tag->get_attribute("name");
		    const char* value = tag->get_attribute("value");
		    if (name)
		    {
			curr_level.AddLevel(name);
			_RemovePropFromList(pNewList, name);
			/*
			 * Here we need to try to set that value.
			 */
			_ResetProp(&curr_level, value, m_pRegistry);
			curr_level.RemoveLevel();
		    }
		}
		else
		{
		    // An End Tag

		    /*
		     * Here we need to try to delete everything that
		     * was not in the file.
		     */
		    if(strcasecmp(tag->m_name, "list") == 0)
		    {
			_HandlePropsRemovedFromFile(
			    &curr_level, pNewList,
			    m_pRegistry);
			curr_level.RemoveLevel();
			delete pNewList;
			pNewList = (CHXSimpleList*)levlist.RemoveHead();
		    }
		}
		break;
	    }
	}
    }
    
    _HandlePropsRemovedFromFile(&curr_level, pNewList, m_pRegistry);

    // cleanup
    if (fp) fclose(fp);
    HX_DELETE(tag);
    HX_DELETE(pNewList);
    HX_VECTOR_DELETE(pBuf);

    // send reconfig response
    m_ActiveSetsOutstanding--;
    MaybeSendReconfigResponse();

    return hResult;
}

#ifdef _WIN32
HX_RESULT
XMLConfig::ReconfigureFromReg(const char* pKeyname)
{
    HX_ASSERT(pKeyname && strlen(pKeyname));

    /* 
     * This should be a duplication of the above loop just using
     * the registry instead of a file.
     */

    /*
     * Pretend that there is one outstanding just to keep
     * the done methods from getting called, seeing a 0,
     * and sending the response until the end of this 
     * func.
     */
    m_ActiveSetsOutstanding++;

    CHXSimpleList levlist;
    CHXSimpleList indexlist;
    int curr_index = 0;
    UINT32 ulBufLen = 512;
    UINT32 ulRealBufLen = 512;
    char* pRegBuf = new char[512];

    UINT32 ulBufLen2 = 256;
    UINT32 ulRealBufLen2 = 256;
    char* pRegBuf2 = new char[256];

    UINT32 ulBufLen3 = 256;
    UINT32 ulRealBufLen3 = 256;
    char* pRegBuf3 = new char[256];
    FILETIME msbs;
    /*
     *  Add for level 0.
     */
    CHXSimpleList* pNewList = new CHXSimpleList;
    IHXValues* pValues = 0;

    _AddPropsToList(pNewList, "Config", m_pRegistry);
    XMLConfigString curr_level;
    curr_level.AddLevel("Config");

    char* pRegBase = new char[512];
    memset(pRegBase, 0, 512);
    SafeSprintf(pRegBase, 512, "Software\\RealNetworks\\%s\\%d.%d\\",
            m_szServerversion, m_ulMajor, m_ulMinor);
    UINT32 ulRegBase = strlen(pRegBase) + 1;

    HKEY hKey=0;
    LONG ret=0;
    HXBOOL bDone = 0;
    char* pc=0;
    /*
     * While we are not done...
     */
    while (!bDone)
    {
	pc = pRegBase + ulRegBase;
	SafeStrCpy(pc, curr_level.CharStar(), 512-ulRegBase);
	while (pc < pRegBase + 512 && *pc)
	{
	    if (*pc == '.')
	    {
		*pc = '\\';
	    }
	    pc++;
	}
	ret = RegOpenKeyEx(HKEY_CLASSES_ROOT, OS_STRING(pRegBase), 0, KEY_READ, &hKey);
	if (ret != ERROR_SUCCESS)
	{
	    //XXXPM fix this
	    HX_ASSERT(0);
	}

	ulBufLen = ulRealBufLen;
	//XXXPM handle buffer too small
	ret = RegEnumKeyEx(hKey, curr_index,
	    OS_STRING(pRegBuf), &ulBufLen, 0, NULL, NULL, &msbs);
	curr_index++;
	/*
	 * If it is a composite...
	 */
	if (ret == ERROR_SUCCESS)
	{
	    const char* name = pRegBuf;
	    _RemovePropFromList(pNewList, name);
	    curr_level.AddLevel(name);
	    levlist.AddHead((void*)pNewList);
	    indexlist.AddHead((void*)curr_index);
	    curr_index = 0;
	    pNewList = new CHXSimpleList;
	    _AddPropsToList(pNewList, curr_level.CharStar(), m_pRegistry);
	}
	/*
	 * It is a non composite value, so we need to get that value.
	 */
	else
	{
	    const char* name = pRegBuf;
	    const char* value = 0;
	    ulBufLen2 = ulRealBufLen2;
	    ulBufLen3 = ulRealBufLen3;
	    DWORD dwType;
	    //XXXPM handle buffer too small

#ifndef _WINCE
	    ret = RegEnumValue(hKey, 0, pRegBuf2, &ulBufLen2, 0, &dwType, 
		               (unsigned char*)pRegBuf3, &ulBufLen3);
#else
            ret = RegQueryValue(hKey, pRegBuf2, pRegBuf3, (long*)&ulBufLen3);
#endif
	    if (ret == ERROR_SUCCESS)
	    {
		value = OS_STRING(pRegBuf3);
		_ResetProp(&curr_level, value, m_pRegistry);
		curr_level.RemoveLevel();
		delete pNewList;
		pNewList = (CHXSimpleList*)levlist.RemoveHead();
		curr_index = (int)indexlist.RemoveHead();
	    }
	    else
	    {
		/*
		 * Closing a composite.
		 */
		_HandlePropsRemovedFromFile(&curr_level, pNewList, m_pRegistry);
		curr_level.RemoveLevel();
		delete pNewList;
		if (!levlist.IsEmpty())
		{
		    pNewList = (CHXSimpleList*)levlist.RemoveHead();
		    curr_index = (int)indexlist.RemoveHead();
		}
		else
		{
		    pNewList = 0;
		    bDone = 1;
		}
	    }
	}
	RegCloseKey(hKey);

    }

    delete[] pRegBase;
    delete[] pRegBuf;
    delete[] pRegBuf2;
    delete[] pRegBuf3;

    m_ActiveSetsOutstanding--;
    MaybeSendReconfigResponse();

    return HXR_OK;
}
#endif

HX_RESULT
XMLConfig::_ResetProp(XMLConfigString* plevel, const char* value, IHXRegistry2* preg)
{
    HX_RESULT res = HXR_FAIL;
    IHXActiveRegistry* pActiveReg = 0;
    LONG32 ulCurrent;
    LONG32 ulNew;
    preg->QueryInterface(IID_IHXActiveRegistry,
	(void**)&pActiveReg);
    if (pActiveReg == 0)
    {
	return HXR_FAIL;
    }

    /*
     * If the prop exists, then figure out the type.
     */
    if (_PropExists(plevel, preg))
    {
	HXPropType type;
	type = preg->GetTypeByName(plevel->CharStar());
	switch (type)
	{
	case PT_INTEGER:
	    ulCurrent = 0;
	    ulNew = atoi(value);

	    /*
	     * If it is an int and we are not changing its value, then say ok and
	     * exit.
	     */
	    preg->GetIntByName(plevel->CharStar(),
		ulCurrent);
	    if (ulCurrent == ulNew)
	    {
		res = HXR_OK;
		goto done;
	    }

	    /*
	     * Try to set it by hand first.
	     */
	    res = preg->SetIntByName(plevel->CharStar(),
		atoi(value));

	    /*
	     * If it failed because it was active, then use the active reg
	     * to set it and add it to our outstanding actives count.
	     */
	    if (res == HXR_PROP_ACTIVE)
	    {
		m_ActiveSetsOutstanding ++;
		res = pActiveReg->SetActiveInt(plevel->CharStar(),
			    ulNew, this);
		if (res != HXR_OK)
		{
		    m_ActiveSetsOutstanding --;
		}
	    }
	    /*
	     * res should be set for good by now.
	     */
	    goto done;
	    
	case PT_STRING:
	    IHXBuffer* pCurrent = 0;

	    /*
	     * See if we are changing this string value at all.
	     */
	    preg->GetStrByName(plevel->CharStar(),
		pCurrent);
	    if (pCurrent)
	    {
		/*
		 * If we are not changing it at all then say ok and exit.
		 */
		if (!strcmp((const char*)pCurrent->GetBuffer(),
		    value))
		{
		    pCurrent->Release();
		    res = HXR_OK;
		    goto done;
		}
		pCurrent->Release();
	    }

	    /*
	     * We are changing it so get a new buffer and set it.
	     */
	    IHXBuffer* pBuffer = new CHXBuffer();
            pBuffer->AddRef();
	    pBuffer->Set((const unsigned char*)value, strlen(value) + 1);

	    res = preg->SetStrByName(plevel->CharStar(),
		pBuffer);

	    /*
	     * If we failed to set it because it is active, then use the
	     * active registry and add to our outstanding actives count.
	     */
	    if (res == HXR_PROP_ACTIVE)
	    {
		m_ActiveSetsOutstanding++;
		res = pActiveReg->SetActiveStr(plevel->CharStar(),
			    pBuffer, this);
		if (res != HXR_OK)
		{
		    m_ActiveSetsOutstanding --;
		}
	    }
	    pBuffer->Release();
	    goto done;

	}
	
    }
    else
    {
	/*
	 * This is the case where this prop was not already there,
	 * so figure out the type from the data.
	 */
	HXBOOL bIsNum = TRUE;
	const char* pc = value;
	while(*pc)
	{
	    if (*pc < '0' || *pc > '9')
	    {
		bIsNum = FALSE;
		break;
	    }
	    pc++;
	}
	if (pc == value)
	{
	    bIsNum = FALSE;
	}

	if (bIsNum)
	{
	    /*
	     * It should be an Integer type so attempt to Set it through the
	     * active reg.  (This acts like an add through the active reg.  Could
	     * have tried to add it by hand first, but didn't.
	     */
	    m_ActiveSetsOutstanding++;
	    res = pActiveReg->SetActiveInt(plevel->CharStar(),
		atoi(value), this);
	    if (res != HXR_OK)
	    {
		m_ActiveSetsOutstanding--;
	    }

	}
	else
	{
	    /*
	     * It should be a string type.  We will just try to do it through active.
	     * (see last comment).
	     */
	    IHXBuffer* pBuffer = new CHXBuffer();
            pBuffer->AddRef();
	    pBuffer->Set((const unsigned char*)value,
		strlen(value) + 1);
	    m_ActiveSetsOutstanding++;
	    res = pActiveReg->SetActiveStr(plevel->CharStar(),
		pBuffer, this);
	    if (res != HXR_OK)
	    {
		m_ActiveSetsOutstanding--;
	    }
	    pBuffer->Release();
	}
    }
done:;
    pActiveReg->Release();
    return res;
}

/************************************************************************
 * IHXActivePropUserResponse stuff.
 *
 * Called with status result on completion of set request.
 */
STDMETHODIMP
XMLConfig::SetActiveIntDone(HX_RESULT res,
				const char* pName,
				UINT32 ul,
				IHXBuffer* pInfo[],
				UINT32 ulNumInfo)
{
    m_ActiveSetsOutstanding--;
    MaybeSendReconfigResponse();
    return HXR_OK;
}

STDMETHODIMP
XMLConfig::SetActiveStrDone(HX_RESULT res,
				const char* pName,
				IHXBuffer* pBuffer,
				IHXBuffer* pInfo[],
				UINT32 ulNumInfo)
{
    m_ActiveSetsOutstanding--;
    MaybeSendReconfigResponse();
    return HXR_OK;
}

STDMETHODIMP
XMLConfig::SetActiveBufDone(HX_RESULT res,
				const char* pName,
				IHXBuffer* pBuffer,
				IHXBuffer* pInfo[],
				UINT32 ulNumInfo)
{
    m_ActiveSetsOutstanding--;
    MaybeSendReconfigResponse();
    return HXR_OK;
}

STDMETHODIMP
XMLConfig::DeleteActivePropDone(HX_RESULT res,
				const char* pName,
				IHXBuffer* pInfo[],
				UINT32 ulNumInfo)
{
    m_ActiveSetsOutstanding--;
    MaybeSendReconfigResponse();
    return HXR_OK;
}

void
XMLConfig::MaybeSendReconfigResponse()
{
    /*
     * If everything has not happened yet,
     * then bail.
     */
    if (m_ActiveSetsOutstanding)
    {
	return;
    }

    m_pReconfigureResponse->ReconfigServerDone(HXR_OK,
	0, 0);
    m_pReconfigureResponse->Release();
    m_pReconfigureResponse = 0;
}

void
XMLConfig::_HandlePropsRemovedFromFile(XMLConfigString* plevel,
				       CHXSimpleList* pList,
				       IHXRegistry2* preg)
{
    XMLPropInfo* pInfo;
    while (!pList->IsEmpty())
    {
	pInfo = (XMLPropInfo*)pList->RemoveHead();
	plevel->AddLevel(pInfo->m_pName);
	switch (pInfo->m_Type)
	{
	case PT_STRING:
	    {
		IHXBuffer* pval;
		pval = _GetDefaultValString(
		    plevel->CharStar(), preg);
		if (!pval)
		{
		    if (!preg->DeleteByName(
			plevel->CharStar()))
		    {
			IHXActiveRegistry* pActiveReg = 0;
			preg->QueryInterface(IID_IHXActiveRegistry,
			    (void**)&pActiveReg);
			if (pActiveReg)
			{
			    m_ActiveSetsOutstanding++;
			    if (HXR_OK != pActiveReg->DeleteActiveProp(
				plevel->CharStar(),
				this))
			    {
				m_ActiveSetsOutstanding--;
			    }
			    pActiveReg->Release();
			}
		    }
		}
		else
		{
		    if (HXR_PROP_ACTIVE == preg->SetStrByName(plevel->CharStar(),
			pval))
		    {
			IHXActiveRegistry* pActiveReg = 0;
			preg->QueryInterface(IID_IHXActiveRegistry,
			    (void**)&pActiveReg);
			if (pActiveReg)
			{
			    m_ActiveSetsOutstanding++;
			    if (HXR_OK != pActiveReg->SetActiveStr(
				plevel->CharStar(), pval,
				this))
			    {
				m_ActiveSetsOutstanding--;
			    }
			    pActiveReg->Release();
			}
		    }
		    pval->Release();
		}
	    }
	    break;

	case PT_INTEGER:
	    {
		INT32 i;
		if (!_GetDefaultValInt(
		    plevel->CharStar(),
		    &i, preg))
		{
		    if (!preg->DeleteByName(
			plevel->CharStar()))
		    {
			IHXActiveRegistry* pActiveReg = 0;
			preg->QueryInterface(IID_IHXActiveRegistry,
			    (void**)&pActiveReg);
			if (pActiveReg)
			{
			    m_ActiveSetsOutstanding++;
			    if (HXR_OK != pActiveReg->DeleteActiveProp(
				plevel->CharStar(),
				this))
			    {
				m_ActiveSetsOutstanding--;
			    }
			    pActiveReg->Release();
			}
		    }
		}
		else
		{
		    if (HXR_PROP_ACTIVE == preg->SetIntByName(plevel->CharStar(),
			i))
		    {
			IHXActiveRegistry* pActiveReg = 0;
			preg->QueryInterface(IID_IHXActiveRegistry,
			    (void**)&pActiveReg);
			if (pActiveReg)
			{
			    m_ActiveSetsOutstanding++;
			    if (HXR_OK != pActiveReg->SetActiveInt(
				plevel->CharStar(),
				i, this))
			    {
				m_ActiveSetsOutstanding--;
			    }
			    pActiveReg->Release();
			}
		    }
		}
	    }
	    break;

	case PT_COMPOSITE:
	    ///XXXPM there will be a bug here if we have default vars in sub lists and 
	    //we try and delete the list.  The default vals will not be found here.
	    IHXActiveRegistry* pActiveReg = 0;
	    preg->QueryInterface(IID_IHXActiveRegistry,
		(void**)&pActiveReg);
	    if (pActiveReg)
	    {
		m_ActiveSetsOutstanding++;
		HX_RESULT res;
		res = pActiveReg->DeleteActiveProp(plevel->CharStar(),
		    this);
		if (res != HXR_OK)
		{
		    m_ActiveSetsOutstanding--;
		}
		pActiveReg->Release();
	    }
	    break;

	}

	plevel->RemoveLevel();
	delete pInfo;
    }
	
}

IHXBuffer*
XMLConfig::_GetDefaultValString(const char* pProp,
				IHXRegistry2* preg)
{
    IHXBuffer* pBuffer = 0;
    char configdefaults[256]; /* Flawfinder: ignore */
    HX_ASSERT(strlen(pProp) < 220);
    SafeSprintf(configdefaults, 256, "configdefaults%s", strchr(pProp, '.'));
    preg->GetStrByName(configdefaults, pBuffer);
    return pBuffer;
}

int
XMLConfig::_GetDefaultValInt(const char* pProp, INT32* pOut,
			     IHXRegistry2* preg)
{
    char configdefaults[256]; /* Flawfinder: ignore */
    HX_ASSERT(strlen(pProp) < 220);
    SafeSprintf(configdefaults, 256, "configdefaults%s", strchr(pProp, '.'));
    if (HXR_OK != preg->GetIntByName(configdefaults, *pOut))
    {
	return 0;
    }
    return 1;
}
