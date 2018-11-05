/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2015 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

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

#include "he_cli.h"

#include "hdk_srv.h"

#include <stdio.h>
#include <stdlib.h>


/* Helper method for HNAP results */
#define SetHNAPResult(pStruct, prefix, method, result) \
    prefix##_Set_##method##Result(pStruct, prefix##_Element_##method##Result, prefix##_Enum_##method##Result_##result)

#ifdef HE_CLI_UNITTEST

static char* s_UUID_ToString(HDK_XML_UUID* pUUID)
{
    char* pszReturn = 0;
    HDK_XML_OutputStream_BufferContext streamCtx;
    unsigned int cbStream;

    streamCtx.pBuf = 0;
    streamCtx.cbBuf = 0;
    streamCtx.ixCur = 0;

    if (HDK_XML_Serialize_UUID(&cbStream, HDK_XML_OutputStream_GrowBuffer, &streamCtx, pUUID))
    {
        /* The grow buffer function always allocates an extra byte for '\0'. */
        if (streamCtx.pBuf)
        {
            streamCtx.pBuf[cbStream] = '\0';
        }
        pszReturn = streamCtx.pBuf;
    }
    return pszReturn;
}

#endif /* HE_CLI_UNITTEST */


/*
 * Method http://cisco.com/he/event/Notify
 */

#ifdef __HE_CLI_METHOD_CISCO_HE_EVENT_NOTIFY__

void HE_CLI_Method_Cisco_he_event_Notify(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
#ifdef HE_CLI_UNITTEST
    const char* pszEventData = HDK_XML_Get_String(pInput, HE_CLI_Element_Cisco_he_event_EventData);
    HDK_XML_Struct* psNORef = HDK_XML_Get_Struct(pInput, HE_CLI_Element_Cisco_he_event_Source);
    char* pszDID = s_UUID_ToString(HDK_XML_Get_UUID(psNORef, HE_CLI_Element_Cisco_he_registrar_DeviceID));
    char* pszNOID = s_UUID_ToString(HDK_XML_Get_UUID(psNORef, HE_CLI_Element_Cisco_he_registrar_NetworkObjectID));

    printf("Recieved Notify:\n\n");
    printf("EventURI:  ");
    printf("%s\n", HDK_XML_Get_String(pInput, HE_CLI_Element_Cisco_he_event_EventURI));
    printf("NORef:     {%s,%s}\n", pszDID, pszNOID);
    if (pszEventData)
    {
        printf("EventData: ");
        printf("%s\n", HDK_XML_Get_String(pInput, HE_CLI_Element_Cisco_he_event_EventData));
    }
    printf("\n");

    if (pszDID)
    {
        free(pszDID);
    }
    if (pszNOID)
    {
        free(pszNOID);
    }
#endif

    /* Unused parameters */
    (void)pMethodCtx;
    (void)pInput;
    (void)pOutput;
}

#endif /* __HE_CLI_METHOD_CISCO_HE_EVENT_NOTIFY__ */
