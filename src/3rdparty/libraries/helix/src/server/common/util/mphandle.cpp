/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: mphandle.cpp,v 1.4 2007/12/27 06:09:35 imakandar Exp $ 
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
#include "hxslist.h"
#include "debug.h"
#include "mphandle.h"
#include "hxassert.h"
#include "hxerror.h"
#include "hxcomm.h"
#include "plgnhand.h"

typedef int (*f_ospathcompare)(const char* s1, const char*s2, size_t n);

#ifdef _WIN32
f_ospathcompare pathcompare = strnicmp;
#else
f_ospathcompare pathcompare = strncmp;
#endif

/*
 * MountPointPath::Segment
 */
MountPointPath::Segment::Segment()
: m_pStart(0)
, m_pEnd(0)
{
}

MountPointPath::Segment::~Segment()
{
    m_pStart = m_pEnd = 0;
}

/*
 * MountPointPath
 */
MountPointPath::MountPointPath()
: m_pBuffer(0)
, m_NumSegments(0)
, m_pSegments(0)
{
    m_pRoot = "/";
}

MountPointPath::~MountPointPath()
{
    m_pBuffer = 0;
    delete[] m_pSegments;
}

void
MountPointPath::GiveBuffer(const char* pMount)
{
    m_NumSegments = 0;
    const char* p = pMount;
    m_pBuffer = pMount;
    int state = 0;
    int in;
    int at = 1;
    
    while (1)
    {
	state++;
	while (*p)
	{
	    while (*p && IsSlash(*p))
	    {
		p++;
	    }
	    in = 0;
	    if (*p && !IsSlash(*p))
	    {
		if (state == 2)
		{
		    m_pSegments[at].m_pStart = p;
		}
		else
		{
		    m_NumSegments++;
		}
	    }
	    while (*p && !IsSlash(*p))
	    {
		in = 1;
		p++;
	    }
	    if (state == 2 && in)
	    {
		m_pSegments[at].m_pEnd = p;
		at++;
	    }
	}
	if (state == 2)
	    break;
	delete[] m_pSegments;
	m_NumSegments++;
	m_pSegments = new Segment[m_NumSegments];
	m_pSegments[0].m_pStart = &(m_pRoot[0]);
	m_pSegments[0].m_pEnd = &(m_pRoot[1]);
	p = pMount;
    }

}

int
MountPointPath::GetSegment(int n, const char** ppBuffer, int* pLen)
{
    if (n >= m_NumSegments)
    {
	return 0;
    }
    (*ppBuffer) = m_pSegments[n].m_pStart;
    (*pLen) = m_pSegments[n].m_pEnd - m_pSegments[n].m_pStart;
    return 1;
}


/*
 * MountPointHandler
 */
MountPointHandler::MountPointNode::MountPointNode()
: m_pNodeName(0)
, m_pNodeList(0)
, m_pAMPList(0)
{
}

MountPointHandler::MountPointNode::~MountPointNode()
{
    delete[] m_pNodeName;
    HX_DELETE(m_pAMPList);
}

void
MountPointHandler::MountPointNode::SetName(const char* pName, int nLen)
{
    delete[] m_pNodeName;
    m_pNodeName = new char[nLen + 1];
    memcpy(m_pNodeName, pName, nLen);
    m_pNodeName[nLen] = 0;
}

MountPointHandler::MountPointHandler()
: m_pTree(0),
  m_TreeID(MP_TREE_OTHER),
  m_pMPDescList(0)
{
}

MountPointHandler::~MountPointHandler()
{
    CHXSimpleList::Iterator i;
    MPDescNode* pTemp = NULL;

    if (m_TreeID == MP_TREE_FSMOUNT && m_pMPDescList != NULL)
    {
        for (i = m_pMPDescList->Begin(); i != m_pMPDescList->End(); ++i)
        {
            pTemp = (MPDescNode*)(*i);
            delete [](pTemp->m_pMPDescription);
            delete pTemp;
        }
        delete m_pMPDescList;
    }
}
/*
 *	Method:		MountPointHandler::IgnoreAMP()
 *	Input:		void* pExtrData
 *	Return:		BOOL
 *	Description:	
 *		We maintain a list of Mount Points for which alternate
 *		locations does not make sense or are not allowed.
 *		This method checks the mount point against the ignore 
 *		list for AMP (Alternate Mount Point).
 *
 *		MountPointHandler::AddMount() depends on this method.
 */
BOOL 
MountPointHandler::IgnoreAMP(void* pExtrData)
{
    int i;
    BOOL bRet = TRUE;

    if (m_TreeID != MP_TREE_FSMOUNT)
    {
        // We are allowing AMPs only for mount points under FS Mount.
        // So, we directly ignore others.
        return bRet;
    }

    PluginHandler::FileSystem::PluginInfo*  pPluginInfo = 
        (PluginHandler::FileSystem::PluginInfo*) pExtrData;
    IHXValues*  pOptions = NULL;
    pPluginInfo->GetOptions(pOptions);
    IHXBuffer*  pShortName = NULL;
    pOptions->GetPropertyBuffer("ShortName", pShortName);
    for (i = 0; i < SIZE_IGNORE_AMP; i++)
    {
		// Check if this mount point is one for which AMP is to be ignored.
        if (!strcmp(arrIgnoreAMP[i], (const char*)pShortName->GetBuffer()))
        {
			// Log an error and ignore this mount point.
            IHXBuffer*  pMountPoint = NULL;
            IHXErrorMessages* pErrorMessages = NULL;
            pPluginInfo->m_pPlugin->GetErrorHandler(pErrorMessages);
            if (pErrorMessages)
            {
                char pErrMsg[2048];
                pOptions->GetPropertyBuffer("MountPoint", pMountPoint);
                sprintf(pErrMsg, "AMP not allowed for Mount Point: '%s', ignored",
                    pMountPoint->GetBuffer());
                pErrorMessages->Report(HXLOG_ERR, 0, 0, pErrMsg, 0);
                HX_RELEASE(pMountPoint);
            }
            bRet = TRUE;
            HX_RELEASE(pErrorMessages);
            break;
        }
    }
    if (i == SIZE_IGNORE_AMP)
    {
        bRet = FALSE;
    }

    HX_RELEASE(pShortName);
    HX_RELEASE(pOptions);

    return bRet;
}

/*
 *	Method:		MountPointHandler::CheckAndAddAMP()
 *	Input:		void* pExtrData, MountPointNode* pMPNode
 *	Return:		void
 *	Description:	
 *		Alternate Mount Point (AMP) feature maintains a list of PluginInfo
 *		(AMPList) for the mount points with same name.
 *		This method creates the AMPList for a mount point and checks for 
 *		duplicate BasePath and SearchOrder.
 *		If duplicate SearchOrder is found for same mount point,
 *		then it is ignored and error is logged.
 *		In case of duplicate BasePath only a warning is logged.
 *		Otherwise, new PluginInfo (pExtrData) will be added to list.
 *
 *		MountPointHandler::AddMount() depends on this method.
 */
void
MountPointHandler::CheckAndAddAMP(void* pExtrData, MountPointNode* pMPNode)
{
    IHXBuffer*  pNewShortName = NULL;
    IHXBuffer*  pAMPShortName = NULL;
    IHXBuffer*  pNewBasePath = NULL;
    IHXBuffer*  pAMPBasePath = NULL;
    UINT32      uNewSearchOrder = 0;
    UINT32      uAMPSearchOrder = 0;
    LISTPOSITION    position = 0, in_position = 0;
    IHXValues*  pNewOptions = NULL, *pAMPOptions = NULL;
    char        pErrMsg[2048];
    
    // Init list of AMPs.
    if (!pMPNode->m_pAMPList)
    {
        // Create AMP list. Add first AMP.
        pMPNode->m_pAMPList = new CHXSimpleList;
        pMPNode->m_pAMPList->AddTail(pExtrData);
        return;
    }

    if (m_TreeID != MP_TREE_FSMOUNT)
    {
        //we support AMPs for only FileSystem mount points
        return;
    }

    PluginHandler::FileSystem::PluginInfo*  pNewPluginInfo = 
        (PluginHandler::FileSystem::PluginInfo*) pExtrData;
    PluginHandler::FileSystem::PluginInfo*  pAMPPluginInfo = 0;
    IHXErrorMessages*   pErrorMessages = NULL;
    
    CHXSimpleList*& pAMPList = pMPNode->m_pAMPList;
    
    // Get BasePath & SearchOrder properties of New AMP.
    pNewPluginInfo->GetOptions(pNewOptions);
    
    pNewOptions->GetPropertyBuffer("BasePath", pNewBasePath);
    pNewOptions->GetPropertyBuffer("ShortName", pNewShortName);
    pNewOptions->GetPropertyULONG32("MountPointSearchOrder", uNewSearchOrder);

    // Check AMP List for duplicate BasePath or SearchOrder.
    // If found, ignore the mountpoint.
    position = pAMPList->GetHeadPosition();
    while (NULL != position)
    {
        pAMPPluginInfo = (PluginHandler::FileSystem::PluginInfo*)
            pAMPList->GetAt(position);

        pAMPPluginInfo->GetOptions(pAMPOptions);
        
        pAMPOptions->GetPropertyULONG32("MountPointSearchOrder",
            uAMPSearchOrder);

		// Check for duplicate SearchOrder
        if (uNewSearchOrder == uAMPSearchOrder)
        {
            pNewPluginInfo->m_pPlugin->GetErrorHandler(pErrorMessages);
            if (pErrorMessages)
            {
                sprintf(pErrMsg, "'%s' AMP with duplicate SearchOrder %d "
                    "ignored", pMPNode->m_pNodeName, uNewSearchOrder);
                pErrorMessages->Report(HXLOG_ERR, 0, 0, pErrMsg, 0);
            }
            HX_RELEASE(pNewBasePath);
            HX_RELEASE(pNewShortName);
            HX_RELEASE(pNewOptions);
            HX_RELEASE(pAMPOptions);
            HX_RELEASE(pErrorMessages);
            return;
        }

        pAMPOptions->GetPropertyBuffer("ShortName", pAMPShortName);
        pAMPOptions->GetPropertyBuffer("BasePath", pAMPBasePath);

        if (pAMPBasePath && pNewBasePath)
        {
			// Check for duplicate BasePath
            if (!strcmp((const char *)pNewBasePath->GetBuffer(),
                    (const char *)pAMPBasePath->GetBuffer()) &&
                !strcmp((const char *)pNewShortName->GetBuffer(),
                    (const char *)pAMPShortName->GetBuffer()))
            {
                pNewPluginInfo->m_pPlugin->GetErrorHandler(pErrorMessages);
                if (pErrorMessages)
                {
                    sprintf(pErrMsg, "'%s' AMP with duplicate BasePath and "
                        "type found", pMPNode->m_pNodeName);
                    pErrorMessages->Report(HXLOG_WARNING, 0, 0, pErrMsg, 0);
                }
                HX_RELEASE(pErrorMessages);
            }
        }

        HX_RELEASE(pAMPBasePath);
        HX_RELEASE(pAMPShortName);
        HX_RELEASE(pAMPOptions);

        if (in_position == 0 && uNewSearchOrder < uAMPSearchOrder)
        {
            // This is the place to insert new node, remember this.
            in_position = position;
        }

        pAMPList->GetNext(position);
    }

    // Add new AMP info at remembered position in the AMP list.
    if (0 == in_position)
    {
        pAMPList->AddTail(pExtrData);
    }
    else
    {
        pAMPList->InsertBefore(in_position, pExtrData);
    }

    HX_RELEASE(pNewBasePath);
    HX_RELEASE(pNewShortName);
    HX_RELEASE(pNewOptions);
}

void 
MountPointHandler::AddMPDescription(void *pExtrData)
{
    IHXBuffer* pLongName;
    const char* pMPDescription;
    PluginHandler::FileSystem::PluginInfo*  pPluginInfo = 
        (PluginHandler::FileSystem::PluginInfo*) pExtrData;
    IHXValues* pOptions = NULL;
    pPluginInfo->GetOptions(pOptions);
    if(HXR_OK == pOptions->GetPropertyBuffer("LongName", pLongName))
	{
        pMPDescription = strrchr((const char*)pLongName->GetBuffer(), '.');
        if (!m_pMPDescList)
        {
            m_pMPDescList = new CHXSimpleList;
        }
        
        pMPDescription++;   // bypass '.'
        MPDescNode* pDescNode = new MPDescNode;
        pDescNode->m_pMPDescription = new char[strlen(pMPDescription) + 2];
        strcpy(pDescNode->m_pMPDescription, pMPDescription);
        pDescNode->pExtrData = pExtrData;

        // Registry will take care of eliminating duplicate entries.
        // Because this is coming from registry, we need not bother.
        // Directly add to list.
        m_pMPDescList->AddTail(pDescNode);
    }
    pOptions->Release();
}

void*
MountPointHandler::AddMount(const char* pMountPoint, void* pExtrData)
{
    CHXSimpleList::Iterator i;
    MountPointNode* pNode;
    MountPointPath mpp;
    int at = 0;
    const char* pBuf;
    int nLen;
    void* pRetData = NULL;
    int needtoadd = 0;

    // Now add MPDescNode.
    if (m_TreeID == MP_TREE_FSMOUNT)
    {
        AddMPDescription(pExtrData);
    }

    mpp.GiveBuffer(pMountPoint);
    mpp.GetSegment(at, &pBuf, &nLen);
    if (!m_pTree)
    {
        m_pTree = new MountPointNode;
        m_pTree->SetName("/", 1);
    }
    pNode = m_pTree;

    while (1)
    {
        /*
         * At this point pNode is always a node which matches
         * segment at;
         */
        at++;
        if (!mpp.GetSegment(at, &pBuf, &nLen))
        {
            // Last segment of mountpoint or root
            // Create AMP List and/or add AMP.
            CheckAndAddAMP(pExtrData, pNode);
            pRetData = pExtrData;
            break;
        }
        needtoadd = 0;
        if (!pNode->m_pNodeList)
        {
            // This code is for Root Node and multi-segment MPs.
            // For multi-segment MPs, we maintain the hierarchy.
            needtoadd = 1;
            pNode->m_pNodeList = new CHXSimpleList;
        }
        else
        {
            MountPointNode* pTemp = NULL;
            for (i = pNode->m_pNodeList->Begin();
                i != pNode->m_pNodeList->End(); ++i)
            {
                pTemp = (MountPointNode*)(*i);
                if (!pathcompare(pTemp->m_pNodeName, pBuf, nLen) &&
                    pTemp->m_pNodeName[nLen] == 0)
                {
                    // Found duplicate MountPoint.
                    // Not all mountpoints can have AMPs, check ignore list.
                    if (IgnoreAMP(pExtrData))
                    {
                        return pRetData;
                    }

                    break;
                }
                pTemp = NULL;
            }
            if (pTemp == NULL)
            {
                // MP not found. We need to add new MP Node.
                needtoadd = 1;
            }
            else
            {
                // MP found. AMP List already updated.
                pNode = pTemp;
            }
        }
        if (needtoadd)
        {
            MountPointNode* pTemp = new MountPointNode;
            pNode->m_pNodeList->AddTail((void*)pTemp);
            pNode = pTemp;
            pNode->SetName(pBuf, nLen);
        }
    }
    return pRetData;
}

void* 
MountPointHandler::RemoveMPDescription(const char* pMPDescription)
{
    CHXSimpleList::Iterator i;
    MPDescNode* pTemp = NULL;

    if (m_TreeID == MP_TREE_FSMOUNT && m_pMPDescList != NULL)
    {
        for (i = m_pMPDescList->Begin(); i != m_pMPDescList->End(); ++i)
        {
            pTemp = (MPDescNode*)(*i);
            if (!strcasecmp(pMPDescription, pTemp->m_pMPDescription))
            {
                LISTPOSITION pos;
                m_pMPDescList->Find(pTemp, pos);
                m_pMPDescList->RemoveAt(pos);
                return pTemp->pExtrData;
            }
        }
    }

    return NULL;
}

void*
MountPointHandler::RemoveMount(const char* pMountPoint)
{
    HX_ASSERT(0);
    //XXX PM implement some day
    return NULL;
}

/*
 *	Method:		MountPointHandler::GetNextAMP()
 *	Input:		CHXSimpleList* pAMPList, void *pLast, BOOL& bBelongsTo
 *	Return:		void* ( PluginInfo* )
 *	Description:	
 *		This method parses the AMPList to find next alternate location
 *		for the content depending on the last location searched.
 *		It also returns a boolean bBelongsTo to signify whether the next
 *		alternate location has been found or the list has been exhausted.
 *
 *		MountPointHandler::GetMount() depends on this method.
 */
void *
MountPointHandler::GetNextAMP(CHXSimpleList* pAMPList, void *pLast, 
                              BOOL& bBelongsTo)
{
    LISTPOSITION    pos = 0;
    void*   pLastMatch = NULL;
    
    bBelongsTo = FALSE;
    if (pAMPList && !pAMPList->IsEmpty())
    {
        pos = pAMPList->GetHeadPosition();
        if (pLast != 0)
        {
            while (pos != 0)
            {
                if (pLast == pAMPList->GetAt(pos))
                {
                    // Found last used AMP PluginInfo. 
                    // Switch to next AMP PluginInfo.
                    bBelongsTo = TRUE;
                    pAMPList->GetNext(pos);
                    break;
                }
                pAMPList->GetNext(pos);
            }
        }
        if (pos != 0)
        {
            pLastMatch = pAMPList->GetAt(pos);
        }
        else
        {
            // AMP List exhausted.
            pLastMatch = NULL;
        }

    }

    return pLastMatch;
}

void*
MountPointHandler::GetMPDescription(const char* pMPDescription)
{
    CHXSimpleList::Iterator i;
    MPDescNode* pTemp = NULL;

    if (*pMPDescription == '/')
        pMPDescription++;

    for (i = m_pMPDescList->Begin(); i != m_pMPDescList->End(); ++i)
    {
        pTemp = (MPDescNode*)(*i);
        if (!strcasecmp(pMPDescription, pTemp->m_pMPDescription))
        {
            return pTemp->pExtrData;
        }
    }

    return NULL;
}

void*
MountPointHandler::GetMount(const char* pMountPoint, void* pLast)
{
    void* pLastMatch = NULL;
    void* pNextMatch = NULL;
    void* pParentFirstMP = NULL;
    MountPointPath mpp;
    MountPointNode* pNode = NULL;
    MountPointNode* pParent = NULL;
    CHXSimpleList::Iterator i;
    int at = 0;
    const char* pBuf;
    int nLen;
    BOOL    bBelongsTo = FALSE;

#if 0
    PrintTree();
    fflush(stdout);
#endif

    // Check if it's a mount point description instead of mount point name.
    // In Browse Content page now we are having link on description which 
    // will be appended to URL and passed on.
    // And, we are expecting the string 'MPDESC' along with this description
    // delimited by '~' char.
    const char* pID = strstr(pMountPoint, "~MPDESC");
    if (pID)
    {
        // URL can be of form:
        // '/admin/browse_content.html?/My Content 1~MPDESC/realvideo10.rm'
        // In this case we should consider only '/admin/browse_content.html'
        const char* pParam = strchr(pMountPoint, '?');
        if (pParam && pParam < pID)
        {
            goto go_normal;
        }

        nLen = pID - pMountPoint;
        char* pMPDescription = new char[nLen + 2];
        strncpy(pMPDescription, pMountPoint, nLen);
        pMPDescription[nLen] = '\0';

        pLastMatch = GetMPDescription(pMPDescription);
        // Do not return the same PluginInfo as last one as this could
        // fall in recursive loop.
        if (pLastMatch == pLast)
        {
            pLastMatch = NULL;
        }
        
        delete []pMPDescription;
        
        return pLastMatch;
    }

go_normal:
    mpp.GiveBuffer(pMountPoint);
    mpp.GetSegment(at, &pBuf, &nLen);

    pNode = pParent = m_pTree;
    
    while (1)
    {
        /*
         * pNode is a node which matches segment at;
         */
        if (!pNode)
        {
            break;
        }
        
        if (pLast == NULL && pNode->m_pAMPList)
        {
            pLastMatch = GetNextAMP(pNode->m_pAMPList, NULL, bBelongsTo);
        }
        else
        {
            if (pParent->m_pAMPList)
            {
                pParentFirstMP = GetNextAMP(pParent->m_pAMPList, NULL, bBelongsTo);
            }

            pNextMatch = GetNextAMP(pNode->m_pAMPList, pLast, bBelongsTo);

            if (bBelongsTo)
            {
                if (pNextMatch || pNode == m_pTree)
                {
                    pLastMatch = pNextMatch;  
                    break;
                }
                else
                {
                    //return parent's first node
                    pLastMatch = pParentFirstMP;
                    break;
                }
            }
        }
        
        at++;
        if (!mpp.GetSegment(at, &pBuf, &nLen))
        {
            // Last segment of MP or a Root.
            // Find the next AMP to be used.
            // pLastMatch = GetNextAMP(pNode->m_pAMPList, pLast);
            break;
        }
        if (!pNode->m_pNodeList)
        {
            break;
        }
        else
        {
            // pLastMatch = NULL;
            MountPointNode* pTemp = NULL;
            for (i = pNode->m_pNodeList->Begin();
                i != pNode->m_pNodeList->End(); ++i)
            {
                pTemp = (MountPointNode*)(*i);
                if (!pathcompare(pTemp->m_pNodeName, pBuf, nLen) &&
                    pTemp->m_pNodeName[nLen] == 0)
                {
                    break;
                }
                pTemp = NULL;
            }
            pParent = pNode;
            pNode = pTemp;
        }
    }
    
    return pLastMatch;
}

void
MountPointHandler::PrintTree()
{
    print_tree_recurse(m_pTree, 0);
}

void
MountPointHandler::print_amp(MountPointNode* pMPNode, int level)
{
    IHXBuffer* base_path = NULL;
    UINT32 search_order = 0;
    PluginHandler::FileSystem::PluginInfo* pPluginInfo = 0;

    CHXSimpleList::Iterator j;
    IHXValues* pOptions = NULL;

    if (!pMPNode || !pMPNode->m_pAMPList || pMPNode->m_pAMPList->IsEmpty())
    {
        return;
    }

    for (j = pMPNode->m_pAMPList->Begin(); j != pMPNode->m_pAMPList->End(); ++j)
    {
        pPluginInfo = (PluginHandler::FileSystem::PluginInfo*)(*j);
        pPluginInfo->GetOptions(pOptions);
        pOptions->GetPropertyBuffer("BasePath", base_path);
        if (base_path)
        {
            pOptions->GetPropertyULONG32("MountPointSearchOrder", search_order);
            for (int i = 0; i <= level; i++)
            {
                printf("    ");
            }
            printf ("%d %s\n", search_order, base_path->GetBuffer());
            HX_RELEASE(base_path);
        }
        HX_RELEASE(pOptions);
    }
}

void
MountPointHandler::print_tree_recurse(MountPointNode* pNode, int level)
{
    if (!pNode)
    {
	return;
    }
    for (int i = 0; i < level; i++)
    {
	printf("    ");
    }
    printf("%s\n", pNode->m_pNodeName);

    if (m_TreeID == MP_TREE_FSMOUNT)
    {
        print_amp(pNode, level);
    }

    CHXSimpleList::Iterator j;
    if (!pNode->m_pNodeList)
    {
	return;
    }
    for (j = pNode->m_pNodeList->Begin(); j != pNode->m_pNodeList->End(); ++j)
    {
	MountPointNode* pTemp = (MountPointNode*)(*j);
	print_tree_recurse(pTemp, level+1);
    }
}
void
MountPointHandler::SetTreeID(TreeID id)
{
    m_TreeID = id;
}
