/* ***** BEGIN LICENSE BLOCK *****  
 * Source last modified: $Id: mphandle.h,v 1.4 2007/12/27 06:12:26 imakandar Exp $ 
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
class CHXSimpleList;


class MountPointPath
{
public:
    MountPointPath();
    ~MountPointPath();
    
    void GiveBuffer(const char* pBuf);
    int GetSegment(int n, const char** pBuf, int* pLen);
    
private:
    class Segment
    {
    public: 
	Segment();
	~Segment();
	
	const char* m_pStart;
	const char* m_pEnd;
    };
    const char* m_pRoot;
    const char* m_pBuffer;
    int m_NumSegments;
    Segment* m_pSegments;
};

static const char *arrIgnoreAMP[] = {
	"pn-ramgen",
    "pn-asxgen",
    "pn-sdpgen"
};
#define SIZE_IGNORE_AMP	(sizeof(arrIgnoreAMP)/sizeof(const char *))

enum TreeID 
{
    MP_TREE_FSMOUNT, 
    MP_TREE_OTHER
};

class MountPointHandler
{
public:
    MountPointHandler();
    ~MountPointHandler();
    
    void* AddMount(const char* pMountPoint, void* pExtrData);
    void* RemoveMount(const char* pMountPoint);
    void* GetMount(const char* pMountPoint, void* pLast);
    void PrintTree();
    void SetTreeID(TreeID id);
    
private:
    class MountPointNode
    {
    public:
	MountPointNode();
	~MountPointNode();
	
	void SetName(const char* pName, int nLen);
	
    private:
	CHXSimpleList* m_pNodeList;
	char* m_pNodeName;
	CHXSimpleList*  m_pAMPList; // AMP List.
	friend class MountPointHandler;
    };
    void print_tree_recurse(MountPointNode* pNode, int level);
	BOOL IgnoreAMP(void* pExtrData);
	void CheckAndAddAMP(void* pExtrData, MountPointNode* pMPNode);
    void *GetNextAMP(CHXSimpleList* pAMPList, void *pLast, BOOL& bBelongsTo);
    void print_amp(MountPointNode* pMPNode, int level);

    MountPointNode* m_pTree;
    TreeID m_TreeID;

    // Mount point config and description mapping.
    class MPDescNode
    {
    private:
        char* m_pMPDescription;
        void* pExtrData;

    public:
        MPDescNode() {}
        ~MPDescNode() {}
        friend class MountPointHandler;
    };

    void AddMPDescription(void *pExtrData);
    void* RemoveMPDescription(const char* pMPDescription);
    void* GetMPDescription(const char* pMPDescription);

    CHXSimpleList* m_pMPDescList;
};

inline int
IsSlash(char c)
{
    return c == '/' || c == '\\';
}
