/* ***** BEGIN LICENSE BLOCK *****
 * Source last modified: $Id: asmrulep.cpp,v 1.24 2006/05/22 19:14:09 dcollins Exp $
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

// #include "hlxclib/stdio.h"      /* printf */
#include "hxtypes.h"    /* Basic Types */
#include "hlxclib/stdlib.h"
#include "hxstrutl.h"
#include "hxcom.h"      /* IUnknown */
#include "ihxpckts.h"
#include "asmrulep.h"	/* ASM Public Include File */
#include "asmrulpp.h"	/* ASM Private Include File */
#include "chxpckts.h"
#include "pckunpck.h"
#include "hxassert.h"

#define RULE_VAL_INFINITY -1

#include "hxheap.h"
/*
#ifdef _DEBUG
#undef HX_THIS_FILE		
static const char HX_THIS_FILE[] = __FILE__;
#endif
*/
static const char* const zpOpName[] =
{
    ">",
    "<",
    ">=",
    "<=",
    "==",
    "!=",
    "AND",
    "OR"
};


ASMRuleExpression::ASMRuleExpression(const char* pExpression)
{
    int temp;

    char* pTemp = new char[temp = (strlen(pExpression) + 1)];
    memcpy(pTemp, pExpression, temp); /* Flawfinder: ignore */

    m_ulNumThresholds = 1; // always have one zero rule
    m_pHead = Parse(pTemp, m_ulNumThresholds);

    delete[] pTemp;
}


ASMRuleExpression::~ASMRuleExpression()
{
    RDelete(m_pHead);
}


/* Note:  This parse is destructive to pExpression */
Node*
ASMRuleExpression::Parse(char* pExpression, UINT32& ulNumThreshold)
{
    char*	pTemp = pExpression;
    int		PLevel = 0;
    HXBOOL	bStripAgain = 1;
    int		OperSize;

    //printf ("Parse Expression: %s\n", pExpression);

    // Strip outside unneccesary parens
    while ((*pExpression == '(') && (bStripAgain))
    {
	for (pTemp = pExpression, PLevel = 0; *pTemp; pTemp++)
	{
	    if (*pTemp == '(')
		PLevel++;

	    if (*pTemp == ')')
	    {
		PLevel--;
		if ((!(*(pTemp + 1))) && (!PLevel))
		{
		    pExpression++;
		    pExpression[strlen(pExpression) - 1] = 0;
		    bStripAgain = 1;
		    break;
		}
		if (!PLevel)
		{
		    bStripAgain = 0;
		    break;
		}
	    }
	}

        // Invalid expression if not able to find subsequent right parens
	if (PLevel)
	{
	    // Invalidate expression
	    pExpression[0] = 0;
	    break;
	}
    }

    for (pTemp = pExpression, PLevel = 0; *pTemp; pTemp++)
    {
	if (*pTemp == '(')
	    PLevel++;
	if (*pTemp == ')')
	    PLevel--;

	if (!PLevel)
	{
	    OperSize = 1;
	    if (((*pTemp == '>') || (*pTemp == '<')) ||
	        ((*pTemp == '=') || (*pTemp == '!')) ||
	        ((*pTemp == '&') || (*pTemp == '|')))
	    {
		OperatorNode* pNode = new OperatorNode;

		pNode->m_Type = HX_RE_OPERATOR;
		switch (*pTemp)
		{
		case '>':
		    if ((*(pTemp + 1)) == '=')
		    {
			pNode->m_Data = HX_RE_GREATEREQUAL;
			OperSize = 2;
		    }
		    else
			pNode->m_Data = HX_RE_GREATER;
		    break;
		case '<':
		    if ((*(pTemp + 1)) == '=')
		    {
			pNode->m_Data = HX_RE_LESSEQUAL;
			OperSize = 2;
		    }
		    else
			pNode->m_Data = HX_RE_LESS;
		    break;
		case '=':
		    if ((*(pTemp + 1)) == '=')
		    {
			pNode->m_Data = HX_RE_EQUAL;
			OperSize = 2;
		    }
		    break;
		case '!':
		    if ((*(pTemp + 1)) == '=')
		    {
			pNode->m_Data = HX_RE_NOTEQUAL;
			OperSize = 2;
		    }
		    break;
		case '&':
		    if ((*(pTemp + 1)) == '&')
		    {
			pNode->m_Data = HX_RE_AND;
			OperSize = 2;
		    }
		    break;
		case '|':
		    if ((*(pTemp + 1)) == '|')
		    {
			pNode->m_Data = HX_RE_OR;
			OperSize = 2;
		    }
		    break;
		default:
		    break;
		};

		*pTemp = 0;
		pNode->m_pLeft  = Parse(pExpression, ulNumThreshold);
		pNode->m_pRight = Parse(pTemp + OperSize, ulNumThreshold);

		return pNode;
	    }
	}
    }

    for (pTemp = pExpression, PLevel = 0; *pTemp; pTemp++)
    {
	if (*pTemp == '(')
	    PLevel++;
	if (*pTemp == ')')
	    PLevel--;

	if (!PLevel)
	{
	    if (*pTemp == '$')
	    {
		VariableNode* pNode = new VariableNode;

		pNode->m_Type = HX_RE_VARIABLE;
		pNode->m_Data = new char[strlen(pTemp)];
		memcpy(pNode->m_Data, pTemp + 1, strlen(pTemp)); /* Flawfinder: ignore */

		pNode->m_pLeft  = 0;
		pNode->m_pRight = 0;

		ulNumThreshold++; // each open variable means one more threshold.

		return pNode;
	    }
	}
    }

    for (pTemp = pExpression, PLevel = 0; *pTemp; pTemp++)
    {
	if (*pTemp == '(')
	    PLevel++;
	if (*pTemp == ')')
	    PLevel--;

	if (!PLevel)
	{
	    if  ((*pTemp == '0') || (*pTemp == '1') || (*pTemp == '2') ||
		 (*pTemp == '3') || (*pTemp == '4') || (*pTemp == '5') ||
		 (*pTemp == '6') || (*pTemp == '7') || (*pTemp == '8') ||
		 (*pTemp == '9'))
	    {
		if (strchr(pTemp, '.'))
		{
		    FloatNode* pNode = new FloatNode;

		    pNode->m_Type = HX_RE_FLOAT;
		    pNode->m_Data = (float)atof(pTemp);

		    pNode->m_pLeft  = 0;
		    pNode->m_pRight = 0;

		    return pNode;
		}
		else
		{
		    IntegerNode* pNode = new IntegerNode;

		    pNode->m_Type = HX_RE_INTEGER;
		    pNode->m_Data = atoi(pTemp);

		    pNode->m_pLeft  = 0;
		    pNode->m_pRight = 0;

		    return pNode;
		}
	    }
	}
    }

    //printf ("Panic: Bad rule\n");
    return 0;
}


void
ASMRuleExpression::Dump()
{
    //printf ("Dumping ASMRuleExpression:\n");
    RDump(m_pHead);
    //printf ("\n");
}


void
ASMRuleExpression::RDump(Node* pNode)
{
    if (!pNode)
	return;

#if 0
    switch(pNode->m_Type)
    {
    case HX_RE_VARIABLE:
	printf ("   Variable: %s\n", ((VariableNode *)pNode)->m_Data);
	break;
    case HX_RE_INTEGER:
	printf ("   Integer: %d\n", ((IntegerNode *)pNode)->m_Data);
	break;
    case HX_RE_FLOAT:
	printf ("   Float: %f\n", ((FloatNode *)pNode)->m_Data);
	break;
    case HX_RE_OPERATOR:
	printf ("   Operator: %s\n",
	    zpOpName[((OperatorNode *)pNode)->m_Data]);
	break;
    }
#endif

    RDump(pNode->m_pLeft);
    RDump(pNode->m_pRight);
}


void
ASMRuleExpression::RDelete(Node* pNode)
{
    if (!pNode)
	return;

    if (pNode->m_Type == HX_RE_VARIABLE)
    {
	HX_VECTOR_DELETE(((VariableNode *)pNode)->m_Data);
    };

    RDelete(pNode->m_pLeft);
    RDelete(pNode->m_pRight);
    delete pNode;
}


/*
 * This is a recursive expression evaluator that will determine whether
 * or not we are subscribed to a particular rule, given the current conditions
 */ 
float
ASMRuleExpression::REvaluate(Node* pNode, IHXValues* pVars)
{
    if (!pNode)
        return (float)0;

    switch(pNode->m_Type)
    {
    case HX_RE_VARIABLE:
	{
	    IHXBuffer* pValue=NULL;
            float nValue = (float)0;

	    pVars->GetPropertyCString(((VariableNode *)pNode)->m_Data, pValue);

	    if (pValue)
	    {
                nValue = (float)atof((const char *)pValue->GetBuffer());
		pValue->Release();
	    }

	    return nValue;
	}

    case HX_RE_INTEGER:
        return (float)((IntegerNode *)pNode)->m_Data;
	break;
    case HX_RE_FLOAT:
	return ((FloatNode *)pNode)->m_Data;
	break;
    case HX_RE_OPERATOR:
	{
	    float Left  = REvaluate(pNode->m_pLeft,  pVars); 
	    float Right = REvaluate(pNode->m_pRight, pVars); 

	    switch (((OperatorNode *)pNode)->m_Data)
	    {
	    case HX_RE_GREATEREQUAL:
                return (float)(Left >= Right);
		break;
	    case HX_RE_GREATER:
                return (float)(Left > Right);
		break;
            case HX_RE_LESSEQUAL:
		if( Right == RULE_VAL_INFINITY )
		{
		    return (float)TRUE;
		}
                return (float)(Left <= Right);
		break;
	    case HX_RE_LESS:
		if( Right == RULE_VAL_INFINITY )
		{
		    return (float)TRUE;
		}
                return (float)(Left < Right);
		break;
	    case HX_RE_EQUAL:
                return (float)(Left == Right);
		break;
	    case HX_RE_NOTEQUAL:
                return (float)(Left != Right);
		break;
	    case HX_RE_AND:
                return (float)(Left && Right);
		break;
	    case HX_RE_OR:
                return (float)(Left || Right);
		break;
            default:
                HX_ASSERT(0);
                return (float)0;
                break;
	    }
	}
	break;
    default:
        HX_ASSERT(0);
        return (float)0;
        break;
    }
}


HXBOOL
ASMRuleExpression::Evaluate(IHXValues* pVars)
{
    HXBOOL res;
    //printf ("Evaluate ASMRuleExpression:\n");
    res = (HXBOOL)REvaluate(m_pHead, pVars);
    //printf ("%d\n", res);
    return res;
}




/*
 * This is a recursive function which will evaluate a tree with one free
 * variable (pPrevar).  The returned array will contain all possible values
 * that are border cases.
 */
float
ASMRuleExpression::RPreEvaluate(Node* pNode, IHXValues* pVars,
				const char* pPreVar, float*& pThreshold,
				UINT32& ulNumThreshold,
				HXBOOL& bInvolvesTheOpenVariable)
{
    bInvolvesTheOpenVariable = FALSE;
    float retval = 0;

    float* pThresholdL = NULL;
    float* pThresholdR = NULL;

#define RETURN(x) retval = x; goto exitpoint;

    if (!pNode)
        return (float)0;

    switch(pNode->m_Type)
    {
    case HX_RE_VARIABLE:
	{
	    IHXBuffer* pValue=NULL;
            float nValue = (float)0;

	    pVars->GetPropertyCString(((VariableNode *)pNode)->m_Data, pValue);

	    if (pValue)
	    {
                nValue = (float)atof((const char *)pValue->GetBuffer());
		pValue->Release();
	    }

	    if (strcasecmp(((VariableNode *)pNode)->m_Data, pPreVar) == 0)
	    {
		bInvolvesTheOpenVariable = TRUE;
	    }

	    return nValue;
	}

    case HX_RE_INTEGER:
        return (float)((IntegerNode *)pNode)->m_Data;
	break;
    case HX_RE_FLOAT:
	return ((FloatNode *)pNode)->m_Data;
	break;
    case HX_RE_OPERATOR:
	{
	    HXBOOL bInvolveL, bInvolveR;

	    UINT32 ulNumThresholdL = 0;
	    pThresholdL = pThreshold;
	    float Left  = RPreEvaluate(pNode->m_pLeft,  pVars, pPreVar,
			    pThresholdL, ulNumThresholdL, bInvolveL);

	    UINT32 ulNumThresholdR = 0;
	    pThresholdR = pThreshold + ulNumThresholdL;
	    float Right = RPreEvaluate(pNode->m_pRight, pVars, pPreVar,
			    pThresholdR, ulNumThresholdR, bInvolveR);

	    /* Handle aggregation of Threshold arrays */
	    switch (((OperatorNode *)pNode)->m_Data)
	    {
	    case HX_RE_GREATEREQUAL:
	    case HX_RE_GREATER:
	    case HX_RE_LESSEQUAL:
	    case HX_RE_LESS:
	    case HX_RE_EQUAL:
	    case HX_RE_NOTEQUAL:
		if (bInvolveR)
		{
		    bInvolvesTheOpenVariable = TRUE;
		    *pThreshold = Left;
		    pThreshold++;
		    ulNumThreshold++;
		    RETURN ((float)1);
		}
		if (bInvolveL)
		{
		    bInvolvesTheOpenVariable = TRUE;
		    *pThreshold = Right;
		    pThreshold++;
		    ulNumThreshold++;
		    RETURN ((float)1);
		}
		break;
	    case HX_RE_AND:
		if ((Left && bInvolveR) || (Right && bInvolveL))
		{
		    bInvolvesTheOpenVariable = TRUE;
		    UINT32 ulSize = ulNumThresholdL + ulNumThresholdR;
		    pThreshold += ulSize;
		    ulNumThreshold += ulSize;

		    RETURN ((float)1);
		}
		else
		{
		    RETURN ((float)0);
		}

	    case HX_RE_OR:
		if (Left || bInvolveL || Right || bInvolveR)
		{
		    bInvolvesTheOpenVariable = TRUE;
		    UINT32 ulSize = ulNumThresholdL + ulNumThresholdR;
		    pThreshold += ulSize;
		    ulNumThreshold += ulSize;

		    RETURN ((float)1);
		}
		else
		{
		    RETURN ((float)0);
		}
	    }

	    switch (((OperatorNode *)pNode)->m_Data)
	    {
	    case HX_RE_GREATEREQUAL:
                RETURN ((float)(Left >= Right));

	    case HX_RE_GREATER:
                RETURN ((float)(Left > Right));

            case HX_RE_LESSEQUAL:
                RETURN ((float)(Left <= Right));

	    case HX_RE_LESS:
                RETURN ((float)(Left < Right));

	    case HX_RE_EQUAL:
                RETURN ((float)(Left == Right));

	    case HX_RE_NOTEQUAL:
                RETURN ((float)(Left != Right));

	    case HX_RE_AND:
                RETURN ((float)(Left && Right));

	    case HX_RE_OR:
                RETURN ((float)(Left || Right));

            default:
                HX_ASSERT(0);
                RETURN ((float)0);
	    }
	}

    default:
        HX_ASSERT(0);
        RETURN ((float)0);

    }

exitpoint:

    return retval;

#undef RETURN
}


void
ASMRuleExpression::PreEvaluate(float*& pThreshold, UINT32& ulNumThreshold,
	    IHXValues* pVariables, const char* pPreVar)
{
    HXBOOL bJunk = 0;

    RPreEvaluate(m_pHead, pVariables, pPreVar, 
		 pThreshold, ulNumThreshold, bJunk);
}

/*
 * This is a recursive expression evaluator that will determine whether
 * or not the given variable occurs anywhere in the expression.
 */ 
HXBOOL
ASMRuleExpression::RFindVariable(Node* pNode, const char* pVariable)
{
    if (!pNode)
        return FALSE;

    switch(pNode->m_Type)
    {
    case HX_RE_VARIABLE:
	{
	    if (strcasecmp(((VariableNode *)pNode)->m_Data, pVariable) == 0)
	    {
		return TRUE;
	    }
	}
	break;
    case HX_RE_OPERATOR:
	{
	    return (RFindVariable(pNode->m_pLeft, pVariable) ||
		    RFindVariable(pNode->m_pRight, pVariable));
	}

    case HX_RE_INTEGER:
    case HX_RE_FLOAT:
    default:
        break;
    }

    return FALSE;
}

HXBOOL
ASMRuleExpression::FindVariable(const char* pVariable)
{
    return RFindVariable(m_pHead, pVariable);
}

class Rule
{
public:
			Rule();
    void		SetExpression(const char* pExpression);
    void		Dump();
private:
    ASMRuleExpression*	m_pASMRuleExpression;
    // Add Properties Definitions Here
};

ASMRule::ASMRule()
	:m_pRuleExpression(NULL)
	,m_pRuleProps(NULL)
{
}

ASMRule::~ASMRule()
{
    HX_DELETE(m_pRuleExpression);
    HX_RELEASE(m_pRuleProps);
}

void
ASMRule::SetContext(IUnknown* pContext)
{
    if (pContext)
    {
	CreateValuesCCF(m_pRuleProps, pContext);
    }
#ifndef HELIX_FEATURE_CLIENT
    else
    {
	m_pRuleProps = new CHXHeader;
	HX_ASSERT(m_pRuleProps);
        m_pRuleProps->AddRef();
    }
#endif
}

void
ASMRule::SetExpression(const char* pExpression)
{
    HX_DELETE(m_pRuleExpression);
    m_pRuleExpression = new ASMRuleExpression(pExpression);
}

void
ASMRule::Dump()
{
    m_pRuleExpression->Dump();
}

ASMRuleBook::ASMRuleBook(const char* pRuleBook)
: m_LastError(HXR_OK)
, m_pValidRulesArray(NULL)
, m_pDeletedRulesArray(NULL)
, m_pRuleBook(NULL)
, m_pContext(NULL)
{
#ifndef HELIX_FEATURE_CLIENT
    Init(pRuleBook);
#else
    // Client should use the other constructor which takes IUnknown*
    HX_ASSERT(FALSE);
#endif
}

ASMRuleBook::ASMRuleBook(IUnknown* pContext, const char* pRuleBook)
: m_LastError(HXR_OK)
, m_pValidRulesArray(NULL)
, m_pDeletedRulesArray(NULL)
, m_pRuleBook(NULL)
, m_pContext(NULL)
{
    m_pContext = pContext;
    HX_ADDREF(m_pContext);
    
    Init(pRuleBook);
}

ASMRuleBook::~ASMRuleBook()
{
    delete [] m_pRules;
    if( m_pValidRulesArray )
    {
       HX_VECTOR_DELETE(m_pValidRulesArray);
    }
    if( m_pDeletedRulesArray )
    {
       HX_VECTOR_DELETE(m_pDeletedRulesArray);
    }

    HX_VECTOR_DELETE(m_pRuleBook);

    HX_RELEASE(m_pContext);
}


HXBOOL
ASMRuleBook::HasExpression()
{
    for (int i = 0; i < m_unNumRules; i++)
    {
        if (m_pRules[i].m_pRuleExpression)
	    return TRUE;
    }

    return FALSE;
}


HX_RESULT
ASMRuleBook::GetProperties(UINT16 unRuleNumber, IHXValues*& pRuleProps)
{
    pRuleProps = m_pRules[unRuleNumber].m_pRuleProps;
    if( pRuleProps )
    {
        pRuleProps->AddRef();
    }

    return HXR_OK;
}


/* Uses REvaluate to return the current subscription given pVariables */
HX_RESULT
ASMRuleBook::GetSubscription(HXBOOL* pSubInfo, IHXValues* pVariables)
{
    UINT32 i = 0;

    for (i = 0; i < m_unNumRules; i++)
    {
        if( m_pDeletedRulesArray && m_pDeletedRulesArray[i] == TRUE )
	{  
            pSubInfo[i] = 0;
	}
	else if (m_pRules[i].m_pRuleExpression)
        {
            pSubInfo[i] = m_pRules[i].m_pRuleExpression->Evaluate(pVariables);
        }
        else
        {
            pSubInfo[i] = 1;
        }
    }

    return HXR_OK;
}

/* 
 * Uses RFindVariable to return information on which rules contain a 
 * given variable in their expressions 
 */
HX_RESULT
ASMRuleBook::FindVariable(HXBOOL* pFound, const char* pVariable)
{
    UINT32 i = 0;

    for (i = 0; i < m_unNumRules; i++)
    {
	if (m_pRules[i].m_pRuleExpression)
	{
	    pFound[i] = m_pRules[i].m_pRuleExpression->FindVariable(pVariable);
	}
	else
	{
	    pFound[i] = FALSE;
	}
    }

    return HXR_OK;
}

/* 
 * Uses GetProperties to return information on which rules contain a 
 * given property 
 */
HX_RESULT
ASMRuleBook::FindProperty(HXBOOL* pFound, const char* pProperty)
{
    HX_RESULT	hResult	    = HXR_OK;
    UINT32	i	    = 0;
    IHXValues* pProperties = NULL;
    IHXBuffer* pBuffer	    = NULL;

    for (i = 0; i < m_unNumRules; i++)
    {
	hResult = GetProperties((UINT16)i, pProperties);
	if (HXR_OK == hResult)
	{
	    hResult = pProperties->GetPropertyCString(pProperty, pBuffer);
	    if (HXR_OK == hResult)
	    {
		pFound[i] = TRUE;
		HX_RELEASE(pBuffer);
	    }
	    HX_RELEASE(pProperties);
	}
    }

    return HXR_OK;
}

static int FloatCompare(const void* p1, const void* p2)
{
    if (*(float*)p1 < *(float*)p2)
	return -1;
    if (*(float*)p1 > *(float*)p2)
	return 1;
    return 0;
}

/*
 * Uses RPreEvaluate to return the array of threshold values, 
 * given pVariables and pPreVar (the free variable)
 * Caveat: Your variables must say "0" for pPreVar in pVariables.
 */
HX_RESULT
ASMRuleBook::GetPreEvaluate(float* pThreshold, UINT32& ulNumThreshold,
			    IHXValues* pVariables, const char* pPreVar)
{
    UINT16 i;
    float* pUnsortedFinal   = new float[m_ulNumThresholds+1];
    float* pTemp = pUnsortedFinal;

    UINT32 ulUFNum = 0;
    ulNumThreshold = 0;

    /* Get everything into a unsorted array with duplicates */ 
    for (i = 0; i < m_unNumRules; i++)
    {
        if (m_pRules[i].m_pRuleExpression)
	{
            m_pRules[i].m_pRuleExpression->PreEvaluate(pTemp, ulUFNum,
		pVariables, pPreVar);
	}
    }

    /*
     * XXXSMP Totally disgusting and doesn't belong here.  So much for
     * a clean, extensible function.
     */
    IHXValues* pValues = 0;
    IHXBuffer* pBuffer = 0;

    pUnsortedFinal[ulUFNum] = (float)0;
    for (i = 0; i < m_unNumRules; i++)
    {
	HXBOOL bOn = TRUE;

	if (m_pRules[i].m_pRuleExpression)
	{
	    bOn = m_pRules[i].m_pRuleExpression->Evaluate(pVariables);
	}

	if (bOn)
	{
	    GetProperties((UINT16) i, pValues);

	    if (HXR_OK == pValues->GetPropertyCString("AverageBandwidth",
		pBuffer))
	    {
		pUnsortedFinal[ulUFNum] += atoi((char*)pBuffer->GetBuffer());
		pBuffer->Release();
	    }

	    if (HXR_OK == pValues->GetPropertyCString("DropByN",
		pBuffer))
	    {
		pUnsortedFinal[ulUFNum] += 1;
		pBuffer->Release();
	    }
            HX_RELEASE(pValues);
	}
    }

    ulUFNum++;

    pUnsortedFinal[ulUFNum] = (float)0;

    qsort(pUnsortedFinal, ulUFNum+1, sizeof(float), FloatCompare);

    /* Sort and squash duplicates into the output array */
    pTemp = pUnsortedFinal;
    float fLast = *pUnsortedFinal;
    *pThreshold = *pUnsortedFinal;
    pTemp++;
    pThreshold++;
    ulNumThreshold++;
    for (i = 0; i < ulUFNum; i++)
    {
	if (*pTemp > fLast)
	{
	    fLast = *pTemp;
	    *pThreshold = fLast;
	    pThreshold++;
	    ulNumThreshold++;
	}
	pTemp++;
    }

    delete [] pUnsortedFinal;

    return HXR_OK;
}


#ifdef TESTING
int
main()
{
    const char* pRuleBook =
	{
	/* Rule 0 */
	"								    \
	    #(24000 <= $Bandwidth) && ($Cpu > 50),			    \
	    TolerablePacketLoss=1.5,					    \
	    AverageBandwidth=4000,					    \
	    AverageBandwidthStd=0,					    \
	    Priority=7;							    \
	"
	/* Rule 1 */
	"								    \
	    #(20000 <= $Bandwidth),					    \
	    TolerablePacketLoss=1.5,					    \
	    AverageBandwidth=4000,					    \
	    AverageBandwidthStd=0,					    \
	    Priority=7;							    \
	"
	/* Rule 2 */
	"								    \
	    # $Bandwidth >= 16000,					    \
	    TolerablePacketLoss=1.5,					    \
	    AverageBandwidth=16000,					    \
	    AverageBandwidthStd=0,					    \
	    Priority=7;							    \
	"
	/* Rule 3 */
	"								    \
	    #(16000 > $Bandwidth) && (($Bandwidth >= 8000)),		    \
	    TolerablePacketLoss=2,					    \
	    AverageBandwidth=8000,					    \
	    AverageBandwidthStd=0,					    \
	    Foo=\"This is a test\",					    \
	    Bar=\"This wu'z a test\",					    \
	    Priority=7;							    \
	"
	/* Rule 4 */
	"								    \
	    # ($Bandwidth > 8000),					    \
	    AverageBandwidth=4000,					    \
	    Priority=7;							    \
	"
	};

    ASMRuleBook	    r(pRuleBook);

    for (int i = 0; i < 25000; i += 1000)
    {
	IHXValues* pVal = new CHXHeader;
	IHXBuffer* pBuffer = new CHXBuffer;
	char s[1024]; /* Flawfinder: ignore */
	//printf ("%d\n", i);
	sprintf (s, "%d", i); /* Flawfinder: ignore */

	pBuffer->Set((unsigned char *)s, 5);
	pVal->SetPropertyCString("Bandwidth", pBuffer);
	pVal->AddRef();

	pBuffer = new CHXBuffer;
	sprintf (s, "%d", 60); /* Flawfinder: ignore */
	pBuffer->Set((unsigned char *)s, 5);
	pVal->SetPropertyCString("Cpu", pBuffer);
	pVal->AddRef();

	if (i == 0)
	{
	    printf ("Threshold Points:  ");
	    float pThreshold[1024];
	    memset (pThreshold, 0xffffffff, sizeof(float) * 1024);
	    UINT32 ulNum = 0;
	    r.GetPreEvaluate(pThreshold, ulNum, pVal, "Bandwidth");
	    for (int j = 0; j < ulNum; j++)
	    {
		printf ("%.2f ", pThreshold[j]);
	    }
	    printf ("\n");
	}

	HXBOOL pSubInfo[1024];
	r.GetSubscription(pSubInfo, pVal);
	printf ("    bw %5d: %d %d %d %d\n", i,
	    pSubInfo[0], pSubInfo[1], pSubInfo[2], pSubInfo[3], pSubInfo[4]);

	pVal->Release();
    }

    return 0;
}
#endif

void
ASMRuleBook::Init(const char* pRuleBook)
{
    m_ulNumThresholds = 1; // always have a default
    int i;
    const char* pRule;
    UINT32 ulRuleBookLen = strlen(pRuleBook);

    // Count Rules
    i = 0;
    HXBOOL bSingleQuote 	    = 0;
    HXBOOL bDoubleQuote	    = 0;
    for (pRule = pRuleBook; *pRule; pRule++)
    {
	if ((*pRule == '\'') && (!bDoubleQuote))
	    bSingleQuote = !bSingleQuote;
	if ((*pRule == '"') && (!bSingleQuote))
	    bDoubleQuote = !bDoubleQuote;

	// Count the number of semi-colons (number of rules)
	if ((bSingleQuote == 0) && (bDoubleQuote == 0) && (*pRule == ';'))
	    i++;
    }

    m_unNumRules = i;
    m_pRules = new ASMRule[i];
    if(!m_pRules)
    {
       m_LastError = HXR_OUTOFMEMORY;
        return;
    }
    
    for (i = 0; i < m_unNumRules; i++)
    {
	m_pRules[i].SetContext(m_pContext);
    }

    //printf ("%d Rules\n", i);

    // Iterate through each rule
    m_pRuleBook = new char[ ulRuleBookLen + 1 ];
    if(!m_pRuleBook)
    {
        m_LastError = HXR_OUTOFMEMORY;
        HX_DELETE(m_pRules);
        return;
    }
    else
    {
        memcpy( (void*) m_pRuleBook, pRuleBook, ulRuleBookLen + 1 );
    }

    m_LastError = Reset();
}

HX_RESULT
ASMRuleBook::InitRulesArray()
{
   if( !m_pValidRulesArray )
   {
      m_pValidRulesArray = new HXBOOL[ m_unNumRules ];
      if(!m_pValidRulesArray)
      {
          return HXR_OUTOFMEMORY;
      }

      for( int ii=0; ii<m_unNumRules; ii++ )
      {
         m_pValidRulesArray[ii] = TRUE;
      }
   }
   if( !m_pDeletedRulesArray )
   {
      m_pDeletedRulesArray = new HXBOOL[ m_unNumRules ];
      if(!m_pDeletedRulesArray)
      {
          HX_VECTOR_DELETE(m_pValidRulesArray);
          return HXR_OUTOFMEMORY;
      }
      for( int ii=0; ii<m_unNumRules; ii++ )
      {
         m_pDeletedRulesArray[ii] = FALSE;
      }
   }
   return HXR_OK;
}

HX_RESULT
ASMRuleBook::Enable( UINT16 nRule )
{
   // Allocate and initialized m_pValidRulesArray if necessary.
   if( HXR_OUTOFMEMORY == InitRulesArray() )
   {
       return HXR_OUTOFMEMORY;
   }
   else
   {
       m_pValidRulesArray[nRule] = TRUE;
   }

   return HXR_OK;
}

HX_RESULT
ASMRuleBook::Disable( UINT16 nRule )
{
   // Allocate and initialized m_pValidRulesArray if necessary.
   if( HXR_OUTOFMEMORY == InitRulesArray() )
   {
       return HXR_OUTOFMEMORY;
   }
   else
   {
       m_pValidRulesArray[nRule] = FALSE;
   }

   return HXR_OK;
}

HX_RESULT
ASMRuleBook::ReCompute()
{
   // We should call a Reset function here
   if( HXR_OUTOFMEMORY == Reset() )
   {
       return HXR_OUTOFMEMORY;
   }

   for( int ii=0; ii<m_unNumRules; ii++ )
   {
      if( m_pValidRulesArray[ ii ] == FALSE )
      {
	 DeleteRule( ii );
      }
   }
   return HXR_OK;
}

// Remove a rule from the rulebook and 'collapse' the surrounding rules
// to cover the bandwidth range that up to now was covered by this rule.
HX_RESULT
ASMRuleBook::DeleteRule( int uRuleToDel )
{
   int nLeftEdge = 0;
   int nRightEdge = 0;
   ULONG32 uNumActiveRules = 0;
   int ii;

   // The rule should not be greater the max, of course.
   if( uRuleToDel > m_unNumRules )
   {
      return HXR_FAIL;
   }

   // Find the "right edge" rule.
   for( ii=0; ii<m_unNumRules; ii++ )
   {
      if( !m_pDeletedRulesArray[ii] && m_pRules[ii].m_pRuleExpression &&
		  m_pRules[ii].m_pRuleExpression->IsRightEdge() )
      {
         nRightEdge = ii;
	 break;
      }
   }

   // Find the "left edge" rule.
   for( ii=m_unNumRules-1; ii>=0; ii-- )
   {
      if( !m_pDeletedRulesArray[ii] && m_pRules[ii].m_pRuleExpression &&
		  m_pRules[ii].m_pRuleExpression->IsLeftEdge() )
      {
         nLeftEdge = ii;
      }
   }

   // Determine the number of active rules.
   for( ii=0; ii<m_unNumRules; ii++ )
   {
      if( !m_pDeletedRulesArray[ii] )
      {
         uNumActiveRules++;
      }
   }

   // We handle the 3 possible cases separately: 1) Left Edge, 2) Right Edge, 
   // and 3) Non-Edge.
   if( uRuleToDel == nLeftEdge )
   {
      // Left edge case
      // Modify the next rule(s) to the right so that it's > value is 0.
      // But only if this is the only rule covering uRuleToDel's bw range.
      if( CheckCurrentRangeEmpty( uRuleToDel ) == TRUE )
      {
         for( ii=uRuleToDel+1; ii<m_unNumRules; ii++ )
         {
            if( !m_pDeletedRulesArray[ ii ] && m_pRules[ ii ].m_pRuleExpression && (
                ( m_pRules[ ii ].m_pRuleExpression->GetLeft() == m_pRules[ uRuleToDel ].m_pRuleExpression->GetRight() ) ) )
            {
               m_pRules[ ii ].m_pRuleExpression->SetLeft( 0 );
            }
         }
      }
   }
   else if( uRuleToDel >= nRightEdge )
   {
      // Right edge case
      // Set the right value (less than x) to infinity
      // But only if this is the only rule covering uRuleToDel's bw range.
      if( CheckCurrentRangeEmpty( uRuleToDel ) == TRUE )
      {
         for( ii=uRuleToDel-1; ii>=0; ii-- )
         {
            if( !m_pDeletedRulesArray[ ii ] && m_pRules[ ii ].m_pRuleExpression && (
                ( m_pRules[ ii ].m_pRuleExpression->GetRight() == m_pRules[ uRuleToDel ].m_pRuleExpression->GetRight() ) ) )
            {
               m_pRules[ ii ].m_pRuleExpression->SetRight( RULE_VAL_INFINITY );
            }
         }
      }
   }
   else if( uRuleToDel > nLeftEdge )
   {
      // Non-edge case
      // Set all "left-adjacent" rules "right-hand" value to nRight.
      // But only if this is the only rule covering uRuleToDel's bw range.
      if( CheckCurrentRangeEmpty( uRuleToDel ) == FALSE )
      {
         // Determine the value of our right-hand value.
         int nRight = (int) m_pRules[ uRuleToDel ].m_pRuleExpression->GetRight();

         for( ii=uRuleToDel-1; ii>=0; ii-- )
         {
            if( !m_pDeletedRulesArray[ ii ]  && m_pRules[ ii ].m_pRuleExpression && (
	        ( m_pRules[ ii ].m_pRuleExpression->GetRight() == m_pRules[ uRuleToDel ].m_pRuleExpression->GetLeft() ) ) )
            {
               m_pRules[ ii ].m_pRuleExpression->SetRight( nRight );
            }
         }
      }
   }

   m_pDeletedRulesArray[uRuleToDel] = TRUE;
   return HXR_OK;
}

void
ASMRuleExpression::SetLeft( int nLeft )
{
    // This function assumes the following forms
    // ($Bandwidth > 123)
    // ($Bandwidth >= 123)
    // ($Bandwidth > 123) && ($Bandwidth < 1234)
    // ($Bandwidth >= 123) && ($Bandwidth < 1234)
    //
    // It assumes the left-most expression is > or >=

    if (m_pHead && (m_pHead->m_Type == HX_RE_OPERATOR))
    {
        OperatorNode* pOp = (OperatorNode*)m_pHead;

        if (pOp->m_pLeft &&
            (pOp->m_pLeft->m_Type == HX_RE_OPERATOR))
        {
            // ($Bandwidth >= 123) && ($Bandwidth < 1234)
            // We want to update the left side
            pOp = (OperatorNode*)pOp->m_pLeft;
        }

        if (NormalizeBwNode(pOp) &&
            ((pOp->m_Data == HX_RE_GREATER) ||
             (pOp->m_Data == HX_RE_GREATEREQUAL)))
        {
            IntegerNode* pInt = (IntegerNode*)pOp->m_pRight;
            pInt->m_Data = nLeft;
        }
    }
}

void
ASMRuleExpression::SetRight( int nRight )
{
    // This function assumes the following forms
    // ($Bandwidth < 123)
    // ($Bandwidth <= 123)
    // ($Bandwidth > 123) && ($Bandwidth < 1234)
    // ($Bandwidth > 123) && ($Bandwidth <= 1234)
    //
    // It assumes the right-most expression is < or <=

    if (m_pHead && (m_pHead->m_Type == HX_RE_OPERATOR))
    {
        OperatorNode* pOp = (OperatorNode*)m_pHead;

        if (pOp->m_pRight &&
            (pOp->m_pRight->m_Type == HX_RE_OPERATOR))
        {
            // ($Bandwidth > 123) && ($Bandwidth < 1234)
            // We want to update the right side
            pOp = (OperatorNode*)pOp->m_pRight;
        }

        if (NormalizeBwNode(pOp) &&
            ((pOp->m_Data == HX_RE_LESS) ||
             (pOp->m_Data == HX_RE_LESSEQUAL)))
        {
            IntegerNode* pInt = (IntegerNode*)pOp->m_pRight;
            pInt->m_Data = nRight;
        }
    }
}

float
ASMRuleExpression::GetLeft()
{
    // This function returns the
    // lower bandwidth threshold. It assumes
    // that the lower threshold expression
    // is always on the left
    OperatorNode* pOp = NULL;

    if(m_pHead && m_pHead->m_pLeft &&
       (m_pHead->m_Type == HX_RE_OPERATOR))
    {
        // Test for ($Bandwidth > 123) && ($Bandwidth <= 123)
        if (m_pHead->m_pLeft->m_Type == HX_RE_OPERATOR)
        {
            // We want the value from the left node
            pOp = (OperatorNode*)m_pHead->m_pLeft;
        }
        else
        {
            pOp = (OperatorNode*)m_pHead;
        }
    }
   
    return (float)GetBwValue(pOp);
}

float
ASMRuleExpression::GetRight()
{
    // This function returns the
    // upper bandwidth threshold. It assumes
    // that the upper threshold expression
    // is always on the right

    OperatorNode* pOp = NULL;

    if(m_pHead && m_pHead->m_pRight &&
       (m_pHead->m_Type == HX_RE_OPERATOR))
    {
        // Test for ($Bandwidth > 123) && ($Bandwidth <= 123)
        if (m_pHead->m_pRight->m_Type == HX_RE_OPERATOR)
        {
            // We want the value from the right node
            pOp = (OperatorNode*)m_pHead->m_pRight;
        }
        else
        {
            pOp = (OperatorNode*)m_pHead;
        }
    }
   
    return (float)GetBwValue(pOp);
}

int
ASMRuleExpression::GetOperatorAsInt()
{
   return (int) ((OperatorNode*) m_pHead)->m_Data;
}

HXBOOL
ASMRuleExpression::IsLeftEdge()
{
    // This is a left edge if it
    // has a bandwidth value on
    // either the left or the right
    return ( GetLeft() || GetRight() );
}


// This function checks to see if the current rule's operator is GREATER or
// GREATEROREQUAL, both of which would suggest that we are the "right edge"
// rule.
HXBOOL
ASMRuleExpression::IsRightEdge()
{
    HXBOOL bRet = FALSE;
    if( m_pHead && (m_pHead->m_Type == HX_RE_OPERATOR))
    {
        OperatorNode* pOp = (OperatorNode*)m_pHead;

        if (NormalizeBwNode(pOp) &&
            ((pOp->m_Data == HX_RE_GREATER) ||
             (pOp->m_Data == HX_RE_GREATEREQUAL)))
        {
            bRet = TRUE;
        }
    }

    return bRet;
}

HX_RESULT
ASMRuleBook::Reset()
{
    int i = 0;
    { //XXXSMPNOW
	HXBOOL bSeenExpression = 0;

	//printf ("******* Rule %d\n", i);

	const char* pRuleBook = m_pRuleBook;
	while (*pRuleBook)
	{
	    char pTemp[2048 + 1];
	    int  n		 = 0;
	    HXBOOL bSingleQuote 	 = 0;
	    HXBOOL bDoubleQuote	 = 0;

	    for (;((*pRuleBook) && (bDoubleQuote || ((*pRuleBook != ',') && (*pRuleBook != ';')))); pRuleBook++)
	    {
		if ((*pRuleBook == '\'') && (!bDoubleQuote))
		    bSingleQuote = !bSingleQuote;
		if ((*pRuleBook == '"') && (!bSingleQuote))
		    bDoubleQuote = !bDoubleQuote;

		// Kill whitespace outside of quotes
		if (bSingleQuote || bDoubleQuote || (
			(*pRuleBook != ' ') &&
			(*pRuleBook != '\n') &&
			(*pRuleBook != '\r') &&
			(*pRuleBook != '\t')))
		{
		    pTemp[n++] = *pRuleBook;
		    if (n >= 2048)
		    {
			//printf ("Panic: Rule Property %d too long\n", i);
			HX_ASSERT(0);
			break;
		    }
		}
	    }
	    pTemp[n] = 0;

	    if ((*pRuleBook == ',') || (*pRuleBook == ';'))
	    {
		// Rule is Valid!

	    	if (*pTemp == '#')
		{
		    // This part of the rule is an expression
		    if (!bSeenExpression)
		    {
			m_pRules[i].SetExpression(pTemp + 1);
			m_pRules[i].Dump();
			bSeenExpression = 1;
			m_ulNumThresholds += m_pRules[i].m_pRuleExpression->GetNumThresholds();
		    }
		    else
		    {
			//printf ("Panic: Two expressions in Rule %d\n", i);
			HX_ASSERT(0);
		    }
		}
		else
		{
                    char *pProp;
                    char *pValue;

                    pProp  = pTemp;
                    pValue = pTemp;

                    while ((*pValue != '=') && (pValue-pTemp < ((int) strlen(pTemp))))
                    pValue++;

                    if (*pValue == '=')
                    {
                        *pValue++ = '\0';
                    }
                    else
                    {
                        pValue = NULL;
                    }

                    if (pValue)
                    {
			IHXBuffer* pBuffer = NULL;
			if (m_pContext)
			{
			    CreateBufferCCF(pBuffer, m_pContext);
			    if(!pBuffer)
			    {
				return HXR_OUTOFMEMORY;
			    }
			}
#ifndef HELIX_FEATURE_CLIENT
			// Client should always provide m_pContext
			else
			{
			    pBuffer = new CHXBuffer;
			    if (!pBuffer)
			    {
				return HXR_OUTOFMEMORY;
			    }
			    pBuffer->AddRef();
			}
#endif

                        /* Strip Outside Quotes */
                        if (*pValue == '"')
                        {
                            pValue++;
                            int end = (strlen(pValue) ? strlen(pValue)-1 : 0);
                            HX_ASSERT(pValue[end] == '"');
                            pValue[end] = 0;
                        }

                        HX_RESULT theErr = 
                        pBuffer->Set((const unsigned char *)pValue,
                        strlen(pValue) + 1);

                        if( theErr == HXR_OUTOFMEMORY )
                        {
                            pBuffer->Release();
                            return theErr;
                        }

                        //printf ("Property '%s'--'%s'\n", pProp, pValue);

                        if( m_pRules[i].m_pRuleProps )
                        {
                            m_pRules[i].m_pRuleProps->
                            SetPropertyCString(pProp, pBuffer);
                        }

                        pBuffer->Release();
                    }
		}

		if (*pRuleBook == ';')
		{
		    i++;
		    bSeenExpression = 0;
                    if (i >= m_unNumRules)
                    {
                        break;
                    }
		    //printf ("******* Rule %d\n", i);
		}
		pRuleBook++;
	    }
	}
    }

    HX_VECTOR_DELETE(m_pDeletedRulesArray);
    return InitRulesArray();
}

HXBOOL
ASMRuleBook::CheckCurrentRangeEmpty( int uRuleToDel )
{
    for( int ii=0; ii<m_unNumRules; ii++ )
    {
        // But only if this is the only rule covering uRuleToDel's bw range.
        if( uRuleToDel != ii && !m_pDeletedRulesArray[ ii ] && m_pRules[ ii ].m_pRuleExpression &&
            ( m_pRules[ ii ].m_pRuleExpression->GetLeft() == m_pRules[ uRuleToDel ].m_pRuleExpression->GetLeft() ) &&
            ( m_pRules[ ii ].m_pRuleExpression->GetRight() == m_pRules[ uRuleToDel ].m_pRuleExpression->GetRight() ) &&
            ( m_pRules[ ii ].m_pRuleExpression->GetOperatorAsInt() == m_pRules[ uRuleToDel ].m_pRuleExpression->GetOperatorAsInt() ) )
        {
            return FALSE;
        }
    }

    return TRUE;
}

int 
ASMRuleExpression::GetBwValue(OperatorNode* pOp) const
{
    int ret = 0;

    if (NormalizeBwNode(pOp))
    {
        // The bandwidth value is always on the right
        // after the node has been normalized
        IntegerNode* pInt = (IntegerNode*)pOp->m_pRight;
        
        ret = pInt->m_Data;
    }

    return ret;
}

HXBOOL 
ASMRuleExpression::NormalizeBwNode(OperatorNode* pOp) const
{
    // This function determines if the expression is a
    // bandwidth threshold expression and if so it makes sure it 
    // always has the bandwidth variable on the left side
    // and the bandwidth value on the right side
    HXBOOL bRet = FALSE;

    if (pOp && pOp->m_pLeft && pOp->m_pRight)
    {
        VariableNode* pVar = NULL;
        IntegerNode* pInt = NULL;
        
        if ((pOp->m_pLeft->m_Type == HX_RE_INTEGER) &&
            (pOp->m_pRight->m_Type == HX_RE_VARIABLE))
        {
            // (123 > somevar)
            // (123 >= somevar)
            pInt = (IntegerNode*)pOp->m_pLeft;
            pVar = (VariableNode*)pOp->m_pRight;
        }
        else if ((pOp->m_pLeft->m_Type == HX_RE_VARIABLE) &&
                 (pOp->m_pRight->m_Type == HX_RE_INTEGER))
        {
            // (somevar < 123)
            // (somevar <= 123)
            pVar = (VariableNode*)pOp->m_pLeft;
            pInt = (IntegerNode*)pOp->m_pRight;
        }
        
        if (pVar && pInt &&
            !strcasecmp(pVar->m_Data, "Bandwidth"))
        {
            // This is a bandwidth node

            if (pVar != pOp->m_pLeft)
            {
                // We need to swap the left and
                // right nodes
                pOp->m_pLeft = pVar;
                pOp->m_pRight = pInt;

                // Flip operator around
                switch(pOp->m_Data) {
                case HX_RE_GREATER:
                    pOp->m_Data = HX_RE_LESS;
                    break;
                case HX_RE_LESS:
                    pOp->m_Data = HX_RE_GREATER;
                    break;
                case HX_RE_GREATEREQUAL:
                    pOp->m_Data = HX_RE_LESSEQUAL;
                    break;
                case HX_RE_LESSEQUAL:
                    pOp->m_Data = HX_RE_GREATEREQUAL;
                    break;
                case HX_RE_EQUAL:
                    break;
                case HX_RE_NOTEQUAL:
                    break;
                case HX_RE_AND:
                    break;
                case HX_RE_OR:
                    break;
                };
            }

            bRet = TRUE;
        }
    }

    return bRet;
}
