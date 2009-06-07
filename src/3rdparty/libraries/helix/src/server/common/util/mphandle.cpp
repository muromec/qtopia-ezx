/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: mphandle.cpp,v 1.2 2003/01/23 23:42:51 damonlan Exp $ 
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
, m_pExtrData(0)
{
}

MountPointHandler::MountPointNode::~MountPointNode()
{
    delete[] m_pNodeName;
    m_pExtrData = 0;
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
: m_pTree(0)
{
}

MountPointHandler::~MountPointHandler()
{
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
	    pRetData = pNode->m_pExtrData;
	    pNode->m_pExtrData = pExtrData;
	    break;
	}
	needtoadd = 0;
	if (!pNode->m_pNodeList)
	{
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
		    break;
		}
		pTemp = NULL;
	    }
	    if (!pTemp)
	    {
		needtoadd = 1;
	    }
	    else
	    {
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
MountPointHandler::RemoveMount(const char* pMountPoint)
{
    HX_ASSERT(0);
    //XXX PM implement some day
    return NULL;
}

void*
MountPointHandler::GetMount(const char* pMountPoint, void* pLast)
{
    void* pLastMatch = NULL;
    MountPointPath mpp;
    MountPointNode* pNode;
    CHXSimpleList::Iterator i;
    int at = 0;
    const char* pBuf;
    int nLen;
    
    mpp.GiveBuffer(pMountPoint);
    mpp.GetSegment(at, &pBuf, &nLen);

    pNode = m_pTree;
    while (1)
    {
	/*
	 * pNode is a node which matches segment at;
	 */

	if (!pNode)
	{
	    break;
	}
	if (pLast != 0 && pNode->m_pExtrData == pLast)
	{
	    break;
	}
	else
	{
	    pLastMatch = pNode->m_pExtrData;
	}
	at++;
	if (!mpp.GetSegment(at, &pBuf, &nLen))
	{
	    break;
	}
	if (!pNode->m_pNodeList)
	{
	    break;
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
		    break;
		}
		pTemp = NULL;
	    }
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
