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

#include "he_auth.h"
#include "he_cli.h"
#include "he_auth_util.h"

#include "hdk_srv.h"
#include "hdk_cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REGISTRAR_HTTP_USERNAME "admin"
#define REGISTRAR_HTTP_PASSWORD "admin"


/* Helper method for HNAP results */
#define SetHNAPResult(pStruct, prefix, method, result) \
    prefix##_Set_##method##Result(pStruct, prefix##_Element_##method##Result, prefix##_Enum_##method##Result_##result)


#if defined(__HE_AUTH_METHOD_CISCO_HE_EVENT_DISPATCH__) || \
    defined(__HE_AUTH_METHOD_CISCO_HE_REGISTRAR_GETDEVICE__) || \
    defined(__HE_AUTH_METHOD_CISCO_HE_REGISTRAR_REGISTER__)

static HDK_XML_Member* s_Devices_FindDevice(HDK_XML_Struct* pStruct, HDK_XML_UUID* pDeviceID)
{
    HDK_XML_Member* pmDevice = 0;

    /* Look for the device instance with matching DID */
    for (pmDevice = pStruct->pHead; pmDevice; pmDevice = pmDevice->pNext)
    {
        HDK_XML_Struct* psDevice = HDK_XML_GetMember_Struct(pmDevice);
        HDK_XML_UUID* pDID = HDK_XML_Get_UUID(psDevice, HE_AUTH_Element_Cisco_he_registrar_DeviceID);

        if (pDID &&
            HDK_XML_IsEqual_UUID(pDID, pDeviceID))
        {
            /* Found it! */
            break;
        }
    }

    return pmDevice;
}

#endif /* defined(__HE_AUTH_METHOD_CISCO_HE_EVENT_DISPATCH__) || \
          defined(__HE_AUTH_METHOD_CISCO_HE_REGISTRAR_GETDEVICE__) || \
          defined(__HE_AUTH_METHOD_CISCO_HE_REGISTRAR_REGISTER__) */

#if defined(__HE_AUTH_METHOD_CISCO_HE_EVENT_SUBSCRIBE__) || \
    defined(__HE_AUTH_METHOD_CISCO_HE_EVENT_UNSUBSCRIBE__) || \
    defined(__HE_AUTH_METHOD_CISCO_HE_EVENT_DISPATCH__)

static HDK_XML_Struct* s_SubscriptionKey_Create(HDK_XML_Struct* pDestStruct, HDK_XML_Struct* pStruct)
{
    HDK_XML_Struct* pSubscription;
    HDK_XML_UUID defaultUUID;

    /* Initialize the defaultUUID */
    memset(defaultUUID.bytes, 0, 16);

    return (pDestStruct && pStruct &&
            (pSubscription = HDK_XML_Set_Struct(pDestStruct, HE_AUTH_Element_Cisco_he_event_Subscription)) != 0 &&

            /* Build up the subscription struct */
            HDK_XML_Set_String(pSubscription, HE_AUTH_Element_Cisco_he_event_EventURI,
                               HDK_XML_GetEx_String(pStruct, HE_AUTH_Element_Cisco_he_event_EventURI, "*")) != 0 &&
            HDK_XML_Set_UUID(pSubscription, HE_AUTH_Element_Cisco_he_event_DeviceID,
                             HDK_XML_GetEx_UUID(pStruct, HE_AUTH_Element_Cisco_he_event_DeviceID, &defaultUUID)) != 0 &&
            HDK_XML_Set_UUID(pSubscription, HE_AUTH_Element_Cisco_he_event_NetworkObjectID,
                             HDK_XML_GetEx_UUID(pStruct, HE_AUTH_Element_Cisco_he_event_NetworkObjectID, &defaultUUID)) != 0)
        ? pSubscription : 0;
}

static HDK_XML_Struct* s_Subscription_Create(HDK_XML_Struct* pDestStruct, HDK_XML_Struct* pStruct)
{
    HDK_XML_Struct* pSubscription;

    return (pDestStruct && pStruct &&
            (pSubscription = s_SubscriptionKey_Create(pDestStruct, pStruct)) != 0 &&

            /* Add the NORef element */
            HDK_XML_Set_Member(pSubscription, HE_AUTH_Element_Cisco_he_event_Notify,
                               HDK_XML_Get_Member(pStruct, HE_AUTH_Element_Cisco_he_event_Notify, HDK_XML_BuiltinType_Struct)))
        ? pSubscription : 0;
}

static int s_SubscriptionKey_Match(HDK_XML_Struct* pSubscription, HDK_XML_Struct* pSubscriptionKey)
{
    char* pszEventURI = HDK_XML_Get_String(pSubscriptionKey, HE_AUTH_Element_Cisco_he_event_EventURI);
    const HDK_XML_UUID* pDeviceID;
    const HDK_XML_UUID* pNetworkObjectID;
    HDK_XML_UUID defaultUUID;

    /* Initialize the defaultUUID */
    memset(defaultUUID.bytes, 0, 16);

    pDeviceID = HDK_XML_GetEx_UUID(pSubscriptionKey, HE_AUTH_Element_Cisco_he_event_DeviceID, &defaultUUID);
    pNetworkObjectID = HDK_XML_GetEx_UUID(pSubscriptionKey, HE_AUTH_Element_Cisco_he_event_NetworkObjectID, &defaultUUID);

    return (pSubscription && pSubscriptionKey &&
            (pszEventURI == 0 ||
             !strcmp(pszEventURI, HDK_XML_GetEx_String(pSubscription, HE_AUTH_Element_Cisco_he_event_EventURI, ""))) &&
            (memcmp(pDeviceID->bytes, defaultUUID.bytes, 16) == 0 ||
             HDK_XML_IsEqual_UUID(pDeviceID, HDK_XML_GetEx_UUID(pSubscription, HE_AUTH_Element_Cisco_he_event_DeviceID, &defaultUUID))) &&
            (memcmp(pNetworkObjectID->bytes, defaultUUID.bytes, 16) == 0 ||
             HDK_XML_IsEqual_UUID(pNetworkObjectID, HDK_XML_GetEx_UUID(pSubscription, HE_AUTH_Element_Cisco_he_event_NetworkObjectID, &defaultUUID))))
        ? 1 : 0;
}

#endif /* defined(__HE_AUTH_METHOD_CISCO_HE_EVENT_SUBSCRIBE__) || \
          defined(__HE_AUTH_METHOD_CISCO_HE_EVENT_UNSUBSCRIBE__) || \
          defined(__HE_AUTH_METHOD_CISCO_HE_EVENT_DISPATCH__) */

#if defined(__HE_AUTH_METHOD_CISCO_HE_EVENT_SUBSCRIBE__) || \
    defined(__HE_AUTH_METHOD_CISCO_HE_EVENT_UNSUBSCRIBE__)

static int s_NORef_IsEqual(HDK_XML_Struct* pStructA, HDK_XML_Struct* pStructB)
{
    HDK_XML_UUID defaultUUID;

    /* Initialize the defaultUUID */
    memset(defaultUUID.bytes, 0, 16);

    return (pStructA && pStructB &&
            HDK_XML_IsEqual_UUID(
                HDK_XML_GetEx_UUID(pStructA, HE_AUTH_Element_Cisco_he_registrar_DeviceID, &defaultUUID),
                HDK_XML_GetEx_UUID(pStructB, HE_AUTH_Element_Cisco_he_registrar_DeviceID, &defaultUUID)) &&
            HDK_XML_IsEqual_UUID(
                HDK_XML_GetEx_UUID(pStructA, HE_AUTH_Element_Cisco_he_registrar_NetworkObjectID, &defaultUUID),
                HDK_XML_GetEx_UUID(pStructB, HE_AUTH_Element_Cisco_he_registrar_NetworkObjectID, &defaultUUID)))
        ? 1 : 0;
}

static int s_SubscriptionKey_IsEqual(HDK_XML_Struct* pStructA, HDK_XML_Struct* pStructB)
{
    HDK_XML_UUID defaultUUID;

    /* Initialize the defaultUUID */
    memset(defaultUUID.bytes, 0, 16);

    return (pStructA && pStructB &&
            !strcmp(HDK_XML_GetEx_String(pStructA, HE_AUTH_Element_Cisco_he_event_EventURI, ""),
                    HDK_XML_GetEx_String(pStructB, HE_AUTH_Element_Cisco_he_event_EventURI, "")) &&
            HDK_XML_IsEqual_UUID(
                HDK_XML_GetEx_UUID(pStructA, HE_AUTH_Element_Cisco_he_event_DeviceID, &defaultUUID),
                HDK_XML_GetEx_UUID(pStructB, HE_AUTH_Element_Cisco_he_event_DeviceID, &defaultUUID)) &&
            HDK_XML_IsEqual_UUID(
                HDK_XML_GetEx_UUID(pStructA, HE_AUTH_Element_Cisco_he_event_NetworkObjectID, &defaultUUID),
                HDK_XML_GetEx_UUID(pStructB, HE_AUTH_Element_Cisco_he_event_NetworkObjectID, &defaultUUID)))
        ? 1 : 0;
}

static int s_Subscription_IsEqual(HDK_XML_Struct* pStructA, HDK_XML_Struct* pStructB)
{
    return (pStructA && pStructB &&
            s_SubscriptionKey_IsEqual(pStructA, pStructB) &&
            s_NORef_IsEqual(
                HDK_XML_Get_Struct(pStructA, HE_AUTH_Element_Cisco_he_event_Notify),
                HDK_XML_Get_Struct(pStructB, HE_AUTH_Element_Cisco_he_event_Notify)))
        ? 1 : 0;
}

#endif /* defined(__HE_AUTH_METHOD_CISCO_HE_EVENT_SUBSCRIBE__) || \
          defined(__HE_AUTH_METHOD_CISCO_HE_EVENT_UNSUBSCRIBE__) */

#if defined(__HE_AUTH_METHOD_CISCO_HE_EVENT_DISPATCH__)

static HDK_XML_Member* s_Devices_GetControlURL(HDK_XML_Struct* pStruct, HDK_XML_UUID* pDeviceID, HDK_XML_UUID* pNetworkObjectID)
{
    HDK_XML_Member* pmControlURL = 0;
    HDK_XML_Member* pmDevice = 0;
    HDK_XML_Struct* psNetworkObjects;

    if ((pmDevice = s_Devices_FindDevice(pStruct, pDeviceID)) != 0 &&
        (psNetworkObjects = HDK_XML_Get_Struct(HDK_XML_GetMember_Struct(pmDevice), HE_AUTH_Element_Cisco_he_registrar_NetworkObjects)) != 0)
    {
        HDK_XML_Member* pmNetworkObject = 0;

        /* Now loop over the network objects */
        for (pmNetworkObject = psNetworkObjects->pHead; pmNetworkObject; pmNetworkObject = pmNetworkObject->pNext)
        {
            HDK_XML_Struct* psNetworkObject = HDK_XML_GetMember_Struct(pmNetworkObject);
            HDK_XML_UUID* pNOID = HDK_XML_Get_UUID(psNetworkObject, HE_AUTH_Element_Cisco_he_registrar_NetworkObjectID);

            /* If the network object id matches, then return the control url member */
            if (pNOID &&
                HDK_XML_IsEqual_UUID(pNOID, pNetworkObjectID))
            {
                pmControlURL = HDK_XML_Get_Member(psNetworkObject, HE_AUTH_Element_Cisco_he_registrar_ControlURL,
                                                  HDK_XML_BuiltinType_String);
                break;
            }
        }
    }
    return pmControlURL;
}

static int s_Notify_IsUnique(HDK_XML_Struct* pNotified, HDK_XML_Struct* pNORef)
{
    HDK_XML_Member* pmNORef = 0;
    HDK_XML_Struct* psNORefs;

    if ((psNORefs = HDK_XML_Get_Struct(pNotified, HE_AUTH_Element_Cisco_he_event_Notify)) != 0)
    {
        for (pmNORef = psNORefs->pHead; pmNORef; pmNORef = pmNORef->pNext)
        {
            if (s_NORef_IsEqual(HDK_XML_GetMember_Struct(pmNORef), pNORef))
            {
                /* NORef has been notified */
                break;
            }
        }

    }
    else
    {
        /* No entries yet, so create a new struct */
        psNORefs = HDK_XML_Set_Struct(pNotified, HE_AUTH_Element_Cisco_he_event_Notify);
    }

    if (pmNORef == 0)
    {
        /* Add the NORef */
        if (psNORefs != 0)
        {
            HDK_XML_AppendEx_Struct(psNORefs, HE_AUTH_Element_Cisco_he_event_Notify, pNORef);
        }
        return 1;
    }
    return 0;
}

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

static HDK_XML_Struct* s_EventNotify_CreateRequest(HDK_XML_Struct* pDestStruct, HDK_XML_Struct* pStruct)
{
    HDK_XML_Struct* psNotifyRequest;
    HDK_XML_Struct* psSource;

    if ((psNotifyRequest = HDK_XML_Set_Struct(pDestStruct, HE_CLI_Element_Cisco_he_event_Notify)) != 0 &&
        (psSource = HDK_XML_Set_Struct(psNotifyRequest, HE_CLI_Element_Cisco_he_event_Source)) != 0)
    {
        HDK_XML_Struct* psTemp = HDK_XML_Get_Struct(pStruct, HE_AUTH_Element_Cisco_he_event_Source);

        /* Build up the request struct */
        HDK_XML_Set_String(psNotifyRequest, HE_CLI_Element_Cisco_he_event_EventURI,
                           HDK_XML_Get_String(pStruct, HE_AUTH_Element_Cisco_he_event_EventURI));

        HDK_XML_Set_UUID(psSource, HE_CLI_Element_Cisco_he_registrar_DeviceID,
                         HDK_XML_Get_UUID(psTemp, HE_AUTH_Element_Cisco_he_registrar_DeviceID));

        HDK_XML_Set_UUID(psSource, HE_CLI_Element_Cisco_he_registrar_NetworkObjectID,
                         HDK_XML_Get_UUID(psTemp, HE_AUTH_Element_Cisco_he_registrar_NetworkObjectID));

        if (HDK_XML_Get_String(pStruct, HE_AUTH_Element_Cisco_he_event_EventData))
        {
            HDK_XML_Set_String(psNotifyRequest, HE_CLI_Element_Cisco_he_event_EventData,
                               HDK_XML_Get_String(pStruct, HE_AUTH_Element_Cisco_he_event_EventData));
        }
    }
    return psNotifyRequest;
}

static int s_EventNotify_Fire(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pStruct, HDK_XML_Struct* pNotifyNORef)
{
    int fSuccess = 0;
    HDK_XML_Struct sInput;
    HDK_XML_Struct sOutput;
    HDK_XML_Struct sTemp;
    HDK_XML_Struct* psNotify;
    HDK_XML_Struct* psDeviceInstances;
    HDK_XML_Member* pmControlURL = 0;
    HDK_XML_UUID* pNotifyDID;
    HDK_XML_UUID* pNotifyNOID;

    /* Initialize temp structs */
    HDK_XML_Struct_Init(&sTemp);
    HDK_XML_Struct_Init(&sInput);
    HDK_XML_Struct_Init(&sOutput);

    HDK_SRV_ADIGet(pMethodCtx, HE_AUTH_ADI_Cisco_he_registrar_RegisteredDevices,
                   &sTemp, HE_AUTH_Element_Cisco_he_registrar_RegisteredDevices);

    if ((psDeviceInstances = HDK_XML_Get_Struct(&sTemp, HE_AUTH_Element_Cisco_he_registrar_RegisteredDevices)) != 0 &&
        (pNotifyDID = HDK_XML_Get_UUID(pNotifyNORef, HE_AUTH_Element_Cisco_he_registrar_DeviceID)) != 0 &&
        (pNotifyNOID = HDK_XML_Get_UUID(pNotifyNORef, HE_AUTH_Element_Cisco_he_registrar_NetworkObjectID)) != 0 &&
        /* Look up the control URL */
        (pmControlURL = s_Devices_GetControlURL(psDeviceInstances, pNotifyDID, pNotifyNOID)) != 0 &&
        /* Create the request struct */
        (psNotify = s_EventNotify_CreateRequest(&sInput, pStruct)) != 0)
    {
        char* pszControlURL = HDK_XML_GetMember_String(pmControlURL);
        char* pszNotifyDID = s_UUID_ToString(pNotifyDID);
        char* pszNotifyNOID = s_UUID_ToString(pNotifyNOID);

        if (pszNotifyDID && pszNotifyNOID)
        {
#ifndef HE_AUTH_UNITTEST
            /* Trim the '/HNAP1' off the end of the control URL */
            pszControlURL[strlen(pszControlURL) - 6] = '\0';

            HDK_CLI_Init();
            fSuccess = (HDK_CLI_Request
                        (
                            pszNotifyDID,
                            pszNotifyNOID,
                            pszControlURL,
                            REGISTRAR_HTTP_USERNAME,
                            REGISTRAR_HTTP_PASSWORD,
                            30,
                            HE_CLI_Module(),
                            HE_CLI_MethodEnum_Cisco_he_event_Notify,
                            psNotify,
                            &sOutput
                         ) == HDK_CLI_Error_OK);
            HDK_CLI_Cleanup();
#else
            HDK_XML_Struct* pSource = HDK_XML_Get_Struct(psNotify, HE_CLI_Element_Cisco_he_event_Source);
            char* pszSourceDID = s_UUID_ToString(HDK_XML_Get_UUID(pSource, HE_CLI_Element_Cisco_he_registrar_DeviceID));
            char* pszSourceNOID = s_UUID_ToString(HDK_XML_Get_UUID(pSource, HE_CLI_Element_Cisco_he_registrar_NetworkObjectID));
            char* pszEventData = HDK_XML_Get_String(psNotify, HE_CLI_Element_Cisco_he_event_EventData);
            char* pszEventURI = HDK_XML_Get_String(psNotify, HE_CLI_Element_Cisco_he_event_EventURI);

            /* Print out the Notify request */
            printf("Calling Notify to: \n");
            printf("\tNORef: {%s,%s}\n", pszNotifyDID, pszNotifyNOID);
            printf("\tControlURL: %s\n", pszControlURL);
            printf("\nNotify\n{\n");
            printf("\tEventURI: %s\n", pszEventURI);
            printf("\tSource: {%s,%s}\n", pszSourceDID, pszSourceNOID);
            if (pszEventData)
            {
                printf("\tEventData: %s\n", pszEventData);
            }
            printf("}\n");

            if (pszSourceDID)
            {
                free(pszSourceDID);
            }
            if (pszSourceNOID)
            {
                free(pszSourceNOID);
            }
            fSuccess = 1;
#endif
        }

        if (pszNotifyDID)
        {
            free(pszNotifyDID);
        }
        if (pszNotifyNOID)
        {
            free(pszNotifyNOID);
        }
    }

    /* Free the temporary structs */
    HDK_XML_Struct_Free(&sTemp);
    HDK_XML_Struct_Free(&sInput);
    HDK_XML_Struct_Free(&sOutput);

    return fSuccess;
}

#endif /* defined(__HE_AUTH_METHOD_CISCO_HE_EVENT_DISPATCH__) */


/*
 * Method http://cisco.com/he/event/Dispatch
 */

#ifdef __HE_AUTH_METHOD_CISCO_HE_EVENT_DISPATCH__

void HE_AUTH_Method_Cisco_he_event_Dispatch(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    HDK_XML_Struct* psSubscriptionKey;
    HDK_XML_Struct* psSubscriptions;
    HDK_XML_Struct sTemp;

    /* Initialize subscription struct */
    HDK_XML_Struct_Init(&sTemp);

    HDK_SRV_ADIGet(pMethodCtx, HE_AUTH_ADI_Cisco_he_event_Subscriptions,
                   &sTemp, HE_AUTH_Element_Cisco_he_event_Subscriptions);

    /* Create the subscription key for comparison */
    if ((psSubscriptionKey = s_SubscriptionKey_Create(&sTemp, pInput)) != 0 &&
        (psSubscriptions = HDK_XML_Get_Struct(&sTemp, HE_AUTH_Element_Cisco_he_event_Subscriptions)) != 0)
    {
        HDK_XML_Member* pmSubscription = 0;

        /* Loop over the subscription ADI searching for matching subscriptions */
        for (pmSubscription = psSubscriptions->pHead; pmSubscription; pmSubscription = pmSubscription->pNext)
        {
            HDK_XML_Struct* psSubscription = HDK_XML_GetMember_Struct(pmSubscription);

            if (s_SubscriptionKey_Match(psSubscription, psSubscriptionKey))
            {
                HDK_XML_Struct* psNORef;

                /* Found a matching subscription, make sure we haven't already notified this NORef */
                if ((psNORef = HDK_XML_Get_Struct(psSubscription, HE_AUTH_Element_Cisco_he_event_Notify)) != 0 &&
                    s_Notify_IsUnique(&sTemp, psNORef))
                {
                    /* Fire off the notification */
                    if (s_EventNotify_Fire(pMethodCtx, pInput, psNORef) == 0)
                    {
                        SetHNAPResult(pOutput, HE_AUTH, Cisco_he_event_Dispatch, ERROR);
                        break;
                    }
                }
            }
        }
    }

    /* Free the temporary struct */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* __HE_AUTH_METHOD_CISCO_HE_EVENT_DISPATCH__ */


/*
 * Method http://cisco.com/he/event/Subscribe
 */

#ifdef __HE_AUTH_METHOD_CISCO_HE_EVENT_SUBSCRIBE__

void HE_AUTH_Method_Cisco_he_event_Subscribe(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    HDK_XML_Struct* psSubscription;
    HDK_XML_Struct* psSubscriptions;
    HDK_XML_Struct sTemp;

    /* Initialize subscription struct */
    HDK_XML_Struct_Init(&sTemp);

    if ((psSubscription = s_Subscription_Create(&sTemp, pInput)) != 0)
    {
        HDK_XML_Member* pmSubscription = 0;

        HDK_SRV_ADIGet(pMethodCtx, HE_AUTH_ADI_Cisco_he_event_Subscriptions,
                       &sTemp, HE_AUTH_Element_Cisco_he_event_Subscriptions);

        if ((psSubscriptions = HDK_XML_Get_Struct(&sTemp, HE_AUTH_Element_Cisco_he_event_Subscriptions)) != 0)
        {
            /* Search the ADI for existing subscription */
            for (pmSubscription = psSubscriptions->pHead; pmSubscription; pmSubscription = pmSubscription->pNext)
            {
                if (s_Subscription_IsEqual(HDK_XML_GetMember_Struct(pmSubscription), psSubscription))
                {
                    break;
                }
            }
        }
        else
        {
            /* No subscriptions exist in the ADI, set a new one in the ADI */
            psSubscriptions = HDK_XML_Set_Struct(&sTemp, HE_AUTH_Element_Cisco_he_event_Subscriptions);
        }

        if (psSubscriptions == 0 ||
            /* Only add the new subscription and set the ADI if it doesn't exist */
            (pmSubscription == 0 &&
             (HDK_XML_AppendEx_Struct(psSubscriptions, HE_AUTH_Element_Cisco_he_event_Subscription, psSubscription) == 0 ||
              HDK_SRV_ADISet(pMethodCtx, HE_AUTH_ADI_Cisco_he_event_Subscriptions,
                             &sTemp, HE_AUTH_Element_Cisco_he_event_Subscriptions) == 0)))
        {
            SetHNAPResult(pOutput, HE_AUTH, Cisco_he_event_Subscribe, ERROR);
        }
    }
    else
    {
        SetHNAPResult(pOutput, HE_AUTH, Cisco_he_event_Subscribe, ERROR);
    }

    /* Free the temporary struct */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* __HE_AUTH_METHOD_CISCO_HE_EVENT_SUBSCRIBE__ */


/*
 * Method http://cisco.com/he/event/Unsubscribe
 */

#ifdef __HE_AUTH_METHOD_CISCO_HE_EVENT_UNSUBSCRIBE__

void HE_AUTH_Method_Cisco_he_event_Unsubscribe(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    HDK_XML_Struct* psSubscription;
    HDK_XML_Struct* psSubscriptions;
    HDK_XML_Struct sTemp;

    /* Initialize subscription struct */
    HDK_XML_Struct_Init(&sTemp);

    if ((psSubscription = s_Subscription_Create(&sTemp, pInput)) != 0)
    {
        HDK_SRV_ADIGet(pMethodCtx, HE_AUTH_ADI_Cisco_he_event_Subscriptions,
                       &sTemp, HE_AUTH_Element_Cisco_he_event_Subscriptions);

        if ((psSubscriptions = HDK_XML_Get_Struct(&sTemp, HE_AUTH_Element_Cisco_he_event_Subscriptions)) != 0)
        {
            HDK_XML_Member* pmSubscription = 0;
            HDK_XML_Member* pmSubscriptionPrev = 0;

            /* Look in the ADI for existing subscription */
            for (pmSubscription = psSubscriptions->pHead; pmSubscription; pmSubscription = pmSubscription->pNext)
            {
                if (s_Subscription_IsEqual(HDK_XML_GetMember_Struct(pmSubscription), psSubscription))
                {
                    /* Found it! */
                    if (pmSubscriptionPrev == 0)
                    {
                        psSubscriptions->pHead = pmSubscription->pNext;
                    }
                    else
                    {
                        pmSubscriptionPrev->pNext = pmSubscription->pNext;
                    }

                    /* Since this is a heap member, we also need to free the member */
                    HDK_XML_Struct_Free(HDK_XML_GetMember_Struct(pmSubscription));
                    free(pmSubscription);
                    break;
                }
                pmSubscriptionPrev = pmSubscription;
            }

            /* Only do an ADI set if we actually removed a subscription */
            if (pmSubscription != 0)
            {
                if (HDK_SRV_ADISet(pMethodCtx, HE_AUTH_ADI_Cisco_he_event_Subscriptions,
                                   &sTemp, HE_AUTH_Element_Cisco_he_event_Subscriptions) == 0)
                {
                    SetHNAPResult(pOutput, HE_AUTH, Cisco_he_event_Unsubscribe, ERROR);
                }
            }
        }
    }

    /* Free the temporary struct */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* __HE_AUTH_METHOD_CISCO_HE_EVENT_UNSUBSCRIBE__ */


/*
 * Method http://cisco.com/he/registrar/GetDevice
 */

#ifdef __HE_AUTH_METHOD_CISCO_HE_REGISTRAR_GETDEVICE__

void HE_AUTH_Method_Cisco_he_registrar_GetDevice(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    HDK_XML_Member* pmDevice = 0;
    HDK_XML_Struct* psDevices;
    HDK_XML_UUID* pDeviceID = 0;

    HDK_XML_Struct sTemp;

    /* Initialize temp struct */
    HDK_XML_Struct_Init(&sTemp);

    HDK_SRV_ADIGet(pMethodCtx, HE_AUTH_ADI_Cisco_he_registrar_RegisteredDevices,
                   &sTemp, HE_AUTH_Element_Cisco_he_registrar_RegisteredDevices);

    if ((psDevices = HDK_XML_Get_Struct(&sTemp, HE_AUTH_Element_Cisco_he_registrar_RegisteredDevices)) != 0 &&
        (pDeviceID = HDK_XML_Get_UUID(pInput, HE_AUTH_Element_Cisco_he_registrar_DeviceID)) != 0 &&
        (pmDevice = s_Devices_FindDevice(psDevices, pDeviceID)) != 0)
    {
        /* Found it! */
        HDK_XML_SetEx_Struct(pOutput, HE_AUTH_Element_Cisco_he_registrar_Device, HDK_XML_GetMember_Struct(pmDevice));
    }

    /* If we didn't find it, set an error */
    if (pmDevice == 0)
    {
        SetHNAPResult(pOutput, HE_AUTH, Cisco_he_registrar_GetDevice, ERROR_UNKNOWN_DEVICE_ID);
    }

    /* Free temp struct */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* __HE_AUTH_METHOD_CISCO_HE_REGISTRAR_GETDEVICE__ */


/*
 * Method http://cisco.com/he/registrar/Query
 */

#ifdef __HE_AUTH_METHOD_CISCO_HE_REGISTRAR_QUERY__

void HE_AUTH_Method_Cisco_he_registrar_Query(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    char* pszServiceURI;
    HDK_XML_Struct* psQueryResults;
    HDK_XML_Struct* psDevices;
    HDK_XML_UUID* pDeviceID;
    HDK_XML_UUID* pNetworkObjectID;

    HDK_XML_Struct sTemp;

    /* Initialize temp struct */
    HDK_XML_Struct_Init(&sTemp);

    pDeviceID = HDK_XML_Get_UUID(pInput, HE_AUTH_Element_Cisco_he_registrar_DeviceID);
    pNetworkObjectID = HDK_XML_Get_UUID(pInput, HE_AUTH_Element_Cisco_he_registrar_NetworkObjectID);
    pszServiceURI = HDK_XML_Get_String(pInput, HE_AUTH_Element_Cisco_he_registrar_ServiceURI);

    HDK_SRV_ADIGet(pMethodCtx, HE_AUTH_ADI_Cisco_he_registrar_RegisteredDevices,
                   &sTemp, HE_AUTH_Element_Cisco_he_registrar_RegisteredDevices);

    if ((psQueryResults = HDK_XML_Set_Struct(pOutput, HE_AUTH_Element_Cisco_he_registrar_QueryResults)) != 0 &&
        (psDevices = HDK_XML_Get_Struct(&sTemp, HE_AUTH_Element_Cisco_he_registrar_RegisteredDevices)) != 0)
    {
        /* Loop over all of the registered devices */
        HDK_XML_Member* pmDevice = 0;

        for (pmDevice = psDevices->pHead; pmDevice; pmDevice = pmDevice->pNext)
        {
            HDK_XML_Struct* psNetworkObjects;
            HDK_XML_Struct* psDevice = HDK_XML_GetMember_Struct(pmDevice);
            HDK_XML_UUID* pDID = HDK_XML_Get_UUID(psDevice, HE_AUTH_Element_Cisco_he_registrar_DeviceID);

            /* If there is a device id, then filter on that */
            if (((pDeviceID && pDID && HDK_XML_IsEqual_UUID(pDeviceID, pDID)) || pDeviceID == 0) &&
                (psNetworkObjects = HDK_XML_Get_Struct(psDevice, HE_AUTH_Element_Cisco_he_registrar_NetworkObjects)) != 0)
            {
                /* Now loop over the network objects */
                HDK_XML_Member* pmNetworkObject = 0;

                for (pmNetworkObject = psNetworkObjects->pHead; pmNetworkObject; pmNetworkObject = pmNetworkObject->pNext)
                {
                    HDK_XML_Struct* psServiceURIs;
                    HDK_XML_Struct* psNetworkObject = HDK_XML_GetMember_Struct(pmNetworkObject);
                    HDK_XML_UUID* pNOID = HDK_XML_Get_UUID(psNetworkObject, HE_AUTH_Element_Cisco_he_registrar_NetworkObjectID);
                    int fMatches = 0;

                    /* If there is a network object id, then filter on that */
                    if (((pNetworkObjectID && pNOID && HDK_XML_IsEqual_UUID(pNetworkObjectID, pNOID)) || pNetworkObjectID == 0) &&
                        (psServiceURIs = HDK_XML_Get_Struct(psNetworkObject, HE_AUTH_Element_Cisco_he_registrar_ServiceURIs)) != 0)
                    {
                        /* Now loop over the service URI */
                        HDK_XML_Member* pmServiceURI = 0;

                        for (pmServiceURI = psServiceURIs->pHead; pmServiceURI; pmServiceURI = pmServiceURI->pNext)
                        {
                            char* pszSURI = HDK_XML_GetMember_String(pmServiceURI);

                            /* If there is a service uri, then filter on that */
                            if ((pszServiceURI && pszSURI && strcmp(pszSURI, pszServiceURI) == 0) || pszServiceURI == 0)
                            {
                                fMatches = 1;
                                break;
                            }
                        }
                    }

                    /* If this network object matches, then add it to the network instance output struct */
                    if (fMatches)
                    {
                        HDK_XML_Struct* psNORef;
                        HDK_XML_Struct* psQueryMatch;

                        if ((psQueryMatch = HDK_XML_Append_Struct(psQueryResults, HE_AUTH_Element_Cisco_he_registrar_QueryMatch)) != 0 &&
                            (psNORef = HDK_XML_Set_Struct(psQueryMatch, HE_AUTH_Element_Cisco_he_registrar_NetworkObjectRef)) != 0)
                        {
                            HDK_XML_Set_UUID(psNORef, HE_AUTH_Element_Cisco_he_registrar_DeviceID, pDID);
                            HDK_XML_Set_UUID(psNORef, HE_AUTH_Element_Cisco_he_registrar_NetworkObjectID, pNOID);
                            HDK_XML_Set_String(psQueryMatch, HE_AUTH_Element_Cisco_he_registrar_ControlURL,
                                               HDK_XML_Get_String(psNetworkObject, HE_AUTH_Element_Cisco_he_registrar_ControlURL));
                        }
                    }
                }
            }
        }
    }

    /* Free temp struct */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* __HE_AUTH_METHOD_CISCO_HE_REGISTRAR_QUERY__ */


/*
 * Method http://cisco.com/he/registrar/Register
 */

#ifdef __HE_AUTH_METHOD_CISCO_HE_REGISTRAR_REGISTER__

void HE_AUTH_Method_Cisco_he_registrar_Register(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    HDK_XML_Struct* psNewDevice;
    HDK_XML_Struct* psDevices;
    HDK_XML_UUID* pNewDeviceID;

    HDK_XML_Struct sTemp;

    /* Initialize temp struct */
    HDK_XML_Struct_Init(&sTemp);

    psNewDevice = HDK_XML_Get_Struct(pInput, HE_AUTH_Element_Cisco_he_registrar_Device);
    pNewDeviceID = HDK_XML_Get_UUID(psNewDevice, HE_AUTH_Element_Cisco_he_registrar_DeviceID);

    HDK_SRV_ADIGet(pMethodCtx, HE_AUTH_ADI_Cisco_he_registrar_RegisteredDevices,
                   &sTemp, HE_AUTH_Element_Cisco_he_registrar_RegisteredDevices);

    if ((psDevices = HDK_XML_Get_Struct(&sTemp, HE_AUTH_Element_Cisco_he_registrar_RegisteredDevices)) != 0)
    {
        HDK_XML_Member* pmDevice = 0;

        if ((pmDevice = s_Devices_FindDevice(psDevices, pNewDeviceID)) != 0)
        {
            /* Found an existing regisration, update it */
            HDK_XML_SetEx_Member(pmDevice, (HDK_XML_Member*)psNewDevice);
        }
        else
        {
            /* New registration, append to registered devices */
            HDK_XML_AppendEx_Struct(psDevices, HE_AUTH_Element_Cisco_he_registrar_Device, psNewDevice);
        }
    }
    else
    {
        /* No registered devices, create a new one */
        if ((psDevices = HDK_XML_Set_Struct(&sTemp, HE_AUTH_Element_Cisco_he_registrar_RegisteredDevices)) != 0)
        {
            HDK_XML_SetEx_Struct(psDevices, HE_AUTH_Element_Cisco_he_registrar_Device, psNewDevice);
        }
    }

    /* If the ADI set fails, return error */
    if (HDK_SRV_ADISet(pMethodCtx, HE_AUTH_ADI_Cisco_he_registrar_RegisteredDevices,
                       &sTemp, HE_AUTH_Element_Cisco_he_registrar_RegisteredDevices) == 0)
    {
        SetHNAPResult(pOutput, HE_AUTH, Cisco_he_registrar_Register, ERROR);
    }

    /* Free temp struct */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* __HE_AUTH_METHOD_CISCO_HE_REGISTRAR_REGISTER__ */


/*
 * Method http://cisco.com/he/registrar/Unregister
 */

#ifdef __HE_AUTH_METHOD_CISCO_HE_REGISTRAR_UNREGISTER__

void HE_AUTH_Method_Cisco_he_registrar_Unregister(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{

    HDK_XML_Member* pmDevice = 0;
    HDK_XML_Struct* psDevices;
    HDK_XML_UUID* pDeviceID;

    HDK_XML_Struct sTemp;

    /* Initialize temp struct */
    HDK_XML_Struct_Init(&sTemp);

    HDK_SRV_ADIGet(pMethodCtx, HE_AUTH_ADI_Cisco_he_registrar_RegisteredDevices,
                   &sTemp, HE_AUTH_Element_Cisco_he_registrar_RegisteredDevices);

    if ((pDeviceID = HDK_XML_Get_UUID(pInput, HE_AUTH_Element_Cisco_he_registrar_DeviceID)) != 0 &&
        (psDevices = HDK_XML_Get_Struct(&sTemp, HE_AUTH_Element_Cisco_he_registrar_RegisteredDevices)) != 0)
    {
        HDK_XML_Member* pmDevicePrev = 0;

        /* Look for the device instance with matching DID */
        for (pmDevice = psDevices->pHead; pmDevice; pmDevice = pmDevice->pNext)
        {
            HDK_XML_Struct* psDevice = HDK_XML_GetMember_Struct(pmDevice);
            HDK_XML_UUID* pDID = HDK_XML_Get_UUID(psDevice, HE_AUTH_Element_Cisco_he_registrar_DeviceID);

            if (pDID &&
                HDK_XML_IsEqual_UUID(pDID, pDeviceID))
            {
                /* Found it! */
                if (pmDevicePrev == 0)
                {
                    psDevices->pHead = pmDevice->pNext;
                }
                else
                {
                    pmDevicePrev->pNext = pmDevice->pNext;
                }

                /* Since this is a heap member, we also need to free the member */
                HDK_XML_Struct_Free(HDK_XML_GetMember_Struct(pmDevice));
                free(pmDevice);
                break;
            }
            pmDevicePrev = pmDevice;
        }
    }

    /* If we couldn't find the device, return appropriate error */
    if (pmDevice == 0)
    {
        SetHNAPResult(pOutput, HE_AUTH, Cisco_he_registrar_Unregister, ERROR_UNKNOWN_DEVICE_ID);
    }
    /* If the ADI set fails, return an error */
    else if (HDK_SRV_ADISet(pMethodCtx, HE_AUTH_ADI_Cisco_he_registrar_RegisteredDevices,
                            &sTemp, HE_AUTH_Element_Cisco_he_registrar_RegisteredDevices) == 0)
    {
        SetHNAPResult(pOutput, HE_AUTH, Cisco_he_registrar_Unregister, ERROR);
    }

    /* Free temp struct */
    HDK_XML_Struct_Free(&sTemp);
}

#endif /* __HE_AUTH_METHOD_CISCO_HE_REGISTRAR_UNREGISTER__ */
