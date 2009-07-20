/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: xmlconfig.h,v 1.9 2007/08/28 06:36:27 hdeware Exp $
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
//   $Id: xmlconfig.h,v 1.9 2007/08/28 06:36:27 hdeware Exp $

#ifndef _XMLCONFIG_H_
#define _XMLCONFIG_H_

#include "hxslist.h"
#include "hxstack.h"
#include "dict.h"
#include "hxcfg.h"
#include "hxmon.h"
#include "hxcomm.h"
#include "hxstrutl.h"

class XMLTag;
class Process;
class ServerRegistry;
class HXRegistry;
class CBigByteQueue;

#define AL_TAGIFY 1
#define MAX_REGISTRY_NAME   1024

typedef enum
{
    CfgList,
    CfgVar
} XMLConfigType;

typedef enum
{
    CfgVarInt,
    CfgVarString,
    CfgVarBool
} XMLConfigVarType;

struct XMLConfigAlias
{
    const char* from;
    const char* to;
    const char* attr1;
    const char* attr2;
    const char* attr3;
    UINT32 flags;
};

#define DEF_XML_ALLOC 256

class XMLConfigString
{
    char*  m_pData;
    UINT32 m_uAlloc;
    UINT32 m_uLen;
 
public:
    XMLConfigString(XMLConfigString& Old)
    {
        m_uAlloc = (Old.m_uLen+1 < DEF_XML_ALLOC ? DEF_XML_ALLOC : Old.m_uLen+1);
        m_pData = new char[m_uAlloc];
        m_uLen = Old.m_uLen;
        memcpy(m_pData, Old.m_pData, m_uLen + 1);
    }
    XMLConfigString()
    {
        m_uAlloc = DEF_XML_ALLOC;
        m_pData = new char[m_uAlloc];
        m_uLen = 0;
        m_pData[0] = '\0';
    }
    ~XMLConfigString()
    {
        HX_VECTOR_DELETE(m_pData);
    }
    void Resize(UINT32 uNewLen)
    {
        if (uNewLen+1 > m_uAlloc)
        {
            m_uAlloc = uNewLen;
            char* pNew = new char[m_uAlloc];
            memcpy(pNew, m_pData, m_uLen+1);
            delete[] m_pData;
            m_pData = pNew;
        }
    }
    const char* CharStar()
    {
        return m_pData;
    }
    const char* Top()
    {
        UINT32 i = m_uLen;
        while (i && m_pData[i] != '.')
        {
            i--;
        }
        if (m_pData[i] == '.')
        {
            i++;
        }
        return m_pData+i;
    }
    void AddLevel(const char* pNew)
    {
        UINT32 uNewLen = strlen(pNew);
        if (m_uLen)
        {
            Resize(m_uLen+1+uNewLen);
            m_uLen += sprintf(m_pData+m_uLen, ".%s", pNew);
        }
        else
        {
            Resize(uNewLen);
            m_uLen = uNewLen;
            memcpy(m_pData, pNew, uNewLen+1);
        }
    }
    void RemoveLevel()
    {
        UINT32 i = m_uLen;
        while (i > 0 && m_pData[i] != '.')
        {
            i--;
        }
        m_pData[i] = '\0';
        m_uLen = i;
    }
};

class XMLPropInfo
{
public:
    char* m_pName;
    HXPropType m_Type;

    XMLPropInfo()
        : m_pName(0)
    {
    }

    ~XMLPropInfo()
    {
        if (m_pName)
        {
            delete[] m_pName;
        }
    }
};

class XMLConfig : public IHXRegConfig,
                  public IHXActivePropUserResponse
{
public:
    XMLConfig(IHXRegistry2* pRegistry, IHXErrorMessages* pMessages, const char* szserverversion,
                     UINT32 dwMajor, UINT32 dwMinor);
    virtual ~XMLConfig();

    /*
     * Com stuff.
     */
    STDMETHOD(QueryInterface) (REFIID riid, void** ppvObj);
    STDMETHOD_(ULONG32,AddRef)  (THIS);
    STDMETHOD_(ULONG32,Release) (THIS);

    /*
     * IHXRegConfig stuff.
     */
    STDMETHOD(WriteKey) (THIS_
                        const char* pKeyName);


    /************************************************************************
    * Called with status result on completion of set request.
    */
    STDMETHOD(SetActiveIntDone)   (THIS_
                                    HX_RESULT res,
                                    const char* pName,
                                    UINT32 ul,
                                    IHXBuffer* pInfo[],
                                    UINT32 ulNumInfo);

    STDMETHOD(SetActiveStrDone)   (THIS_
                                    HX_RESULT res,
                                    const char* pName,
                                    IHXBuffer* pBuffer,
                                    IHXBuffer* pInfo[],
                                    UINT32 ulNumInfo);

    STDMETHOD(SetActiveBufDone)   (THIS_
                                    HX_RESULT res,
                                    const char* pName,
                                    IHXBuffer* pBuffer,
                                    IHXBuffer* pInfo[],
                                    UINT32 ulNumInfo);

    STDMETHOD(DeleteActivePropDone) (THIS_
                                    HX_RESULT res,
                                    const char* pName,
                                    IHXBuffer* pInfo[],
                                    UINT32 ulNumInfo);

    /*
     * Uber reconfigure the server.
     */
    HX_RESULT Reconfigure(IHXReconfigServerResponse* pResp);

    /*
     * Reconfigure from file.
     */
    HX_RESULT Reconfigure(const char* pFilename);

    /*
     * Reconfigure from registry.
     */
    HX_RESULT ReconfigureFromReg(const char* pKeyname);

    IHXReconfigServerResponse* m_pReconfigureResponse;

    class XMLConfigListNode;

    class XMLConfigList : public CHXSimpleList
    {
    public:
        ~XMLConfigList();
        XMLConfigListNode* m_parentnode;
        friend class XMLConfig;
    };

    class XMLConfigListNode
    {
    public:
        XMLConfigListNode() : m_pList(NULL), m_name(NULL), m_value(NULL),
                              m_vserver(-1) {};
        XMLConfigList* m_pList;
        char* m_name;
        char* m_value;
        XMLConfigType m_type;
        UINT32 m_num;
        XMLConfigListNode* m_parent;
        UINT32 m_int;
        HXBOOL   m_bool;
        INT32  m_vserver;

        char* get_registry_name(INT32 vserver = -1);
        ~XMLConfigListNode();
        friend class XMLConfig;
    };
        
    CHXStack m_pListStack;
    XMLConfigList* m_pList;

    HX_RESULT Write(const char* name, const char* filename);
    HX_RESULT Read(char* filename, char* pServRegKey, HXBOOL bIncludedFile = FALSE, HXBOOL bStuffRegistry = TRUE);
//    STDMETHOD(Read) (THIS_ char* filename, char* pServRegKey, HXBOOL bIncludedFile = FALSE);
    HX_RESULT WriteToFile(const char* pKeyName, const char* pFilename);
    void _AddPropsToList(CHXSimpleList* pList, const char* pRoot, IHXRegistry2* pReg);
    void _RemovePropFromList(CHXSimpleList* pList, const char* pName);
    char* _GetPropValueString(const char* pName, IHXRegistry2* pReg);
    void _CleanList(CHXSimpleList* pList);
    void AppendPropsToFile(FILE* fp, XMLConfigString level,
        CHXSimpleList* pList, int indent_per_level,
        IHXRegistry2* hxreg,
        const char* pBase);
    void _AppendPropToFile(FILE* fp, XMLConfigString level,
                             int indent_per_level,
                             IHXRegistry2* hxreg,
                             const char* pBase);
    void _IndentFile(FILE* fp, int indent_per_level,
                         XMLConfigString level,
                         const char* pBase);
    int _PropExists(XMLConfigString* p, IHXRegistry2* preg);
    HX_RESULT _ResetProp(XMLConfigString*, const char*,
        IHXRegistry2*);
    void MaybeSendReconfigResponse();
    void _HandlePropsRemovedFromFile(
        XMLConfigString*, CHXSimpleList*,
        IHXRegistry2*);
    IHXBuffer* _GetDefaultValString(
        const char*, IHXRegistry2*);
    int _GetDefaultValInt(
        const char*, INT32*, IHXRegistry2*);

protected:
//Default Constructor
    XMLConfig();

    HX_RESULT init(IHXRegistry2* pRegistry, IHXErrorMessages* pMessages, const char* szserverversion,
                   UINT32 dwMajor, UINT32 dwMinor);
    void StuffRegistry(XMLConfigList* list);

    //Writes out only the specified child key into the output file. Used for saving .UAS files.
    HX_RESULT WriteToFile(const char* pKeyName, const char* pChildKeyName, const char* pFileName, HXBOOL bCreateFileIfNotFound);

//Properties
    char* m_filename;
    IHXRegistry2* m_pRegistry;
    IHXErrorMessages* m_pMessages;


private:
    INT32 m_lRefCount;
    char* m_szServerversion;
    UINT32 m_ulMajor;
    UINT32 m_ulMinor;

    UINT32 m_ActiveSetsOutstanding;
    Dict m_alias_dict;

    INT32 m_vserver;
    
    HX_RESULT DumpConfig(const char* name, int indent, FILE* outfile,
                         IHXRegistry2* hxreg);
    HXBOOL Expand(XMLTag*, CBigByteQueue* queue);
    void ExpandAttribute(XMLTag*, const char* attribute);
    void DumpList(XMLConfigList* list, int indent);
    XMLConfigVarType GetVarType(XMLConfigListNode* node);
};

#endif
