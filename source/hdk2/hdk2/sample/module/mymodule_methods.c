/**********************************************************************
   Copyright [2014] [Cisco Systems, Inc.]
 
   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at
 
       http://www.apache.org/licenses/LICENSE-2.0
 
   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
**********************************************************************/

/*
 * Copyright (c) 2008-2010 Cisco Systems, Inc. All rights reserved.
 *
 * Cisco Systems, Inc. retains all right, title and interest (including all
 * intellectual property rights) in and to this computer program, which is
 * protected by applicable intellectual property laws.  Unless you have obtained
 * a separate written license from Cisco Systems, Inc., you are not authorized
 * to utilize all or a part of this computer program for any purpose (including
 * reproduction, distribution, modification, and compilation into object code),
 * and you must immediately destroy or return to Cisco Systems, Inc. all copies
 * of this computer program.  If you are licensed by Cisco Systems, Inc., your
 * rights to utilize this computer program are limited by the terms of that
 * license.  To obtain a license, please contact Cisco Systems, Inc.
 *
 * This computer program contains trade secrets owned by Cisco Systems, Inc.
 * and, unless unauthorized by Cisco Systems, Inc. in writing, you agree to
 * maintain the confidentiality of this computer program and related information
 * and to not disclose this computer program and related information to any
 * other person or entity.
 *
 * THIS COMPUTER PROGRAM IS PROVIDED AS IS WITHOUT ANY WARRANTIES, AND CISCO
 * SYSTEMS, INC. EXPRESSLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * INCLUDING THE WARRANTIES OF MERCHANTIBILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, TITLE, AND NONINFRINGEMENT.
 */

#include "mymodule.h"

#include "hdk_srv.h"


/* Helper method for HNAP results */
#define SetHNAPResult(pStruct, prefix, method, result) \
    prefix##_Set_##method##Result(pStruct, prefix##_Element_##method##Result, prefix##_Enum_##method##Result_##result)


/*
 * Method http://cisco.com/mymodule/GetSum
 */

#ifdef __MYMODULE_METHOD_CISCO_MYMODULE_GETSUM__

void MYMODULE_Method_Cisco_mymodule_GetSum(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    HDK_XML_Struct sTemp;
    HDK_XML_Int* pA;
    HDK_XML_Int* pB;

    /* Unused parameters */
    (void)pInput;

    /* Initialize the temporary structure */
    HDK_XML_Struct_Init(&sTemp);

    /* Get the values */
    pA = HDK_XML_GetMember_Int(
        HDK_SRV_ADIGet(pMethodCtx, MYMODULE_ADI_Cisco_mymodule_a, &sTemp, MYMODULE_Element_Cisco_mymodule_a));
    pB = HDK_XML_GetMember_Int(
        HDK_SRV_ADIGet(pMethodCtx, MYMODULE_ADI_Cisco_mymodule_b, &sTemp, MYMODULE_Element_Cisco_mymodule_b));
    if (!pA || !pB)
    {
        SetHNAPResult(pOutput, MYMODULE, Cisco_mymodule_GetSum, ERROR);
    }
    else
    {
        /* Set the result sum */
        HDK_XML_Set_Int(pOutput, MYMODULE_Element_Cisco_mymodule_c, *pA + *pB);
    }

    /* Free the temporary structure */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* __MYMODULE_METHOD_CISCO_MYMODULE_GETSUM__ */


/*
 * Method http://cisco.com/mymodule/SetValues
 */

#ifdef __MYMODULE_METHOD_CISCO_MYMODULE_SETVALUES__

void MYMODULE_Method_Cisco_mymodule_SetValues(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Validate the values */
    if ((*HDK_XML_Get_Int(pInput, MYMODULE_Element_Cisco_mymodule_a) & 1) ||
        (*HDK_XML_Get_Int(pInput, MYMODULE_Element_Cisco_mymodule_a) & 1))
    {
        /* Odd value custom error */
        SetHNAPResult(pOutput, MYMODULE, Cisco_mymodule_SetValues, ERROR_ODD_VALUE);
        return;
    }

    /* Set the values */
    if (!HDK_SRV_ADISet(pMethodCtx, MYMODULE_ADI_Cisco_mymodule_a, pInput, MYMODULE_Element_Cisco_mymodule_a) ||
        !HDK_SRV_ADISet(pMethodCtx, MYMODULE_ADI_Cisco_mymodule_b, pInput, MYMODULE_Element_Cisco_mymodule_b))
    {
        SetHNAPResult(pOutput, MYMODULE, Cisco_mymodule_SetValues, ERROR);
    }
}

#endif /* __MYMODULE_METHOD_CISCO_MYMODULE_SETVALUES__ */
