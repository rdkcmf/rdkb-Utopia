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
 * Copyright (c) 2008-2009 Cisco Systems, Inc. All rights reserved.
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
#include "hdk_methods.h"

#include "hdk_device.h"
#include "hnap12.h"
#include "utctx_api.h"

#include <stdio.h>
#include <stdlib.h>


/*
 * Method http://purenetworks.com/HNAP1/FirmwareUpload
 */

#ifdef __HNAP12_METHOD_PN_FIRMWAREUPLOAD__

#define UTOPIA_FW_UPLOAD_FILE       "/tmp/fw.trx"
#define UTOPIA_FW_UPGRADE_LOG       "/tmp/hdk_fw_upgrade.log"
#define UTOPIA_FW_UPGRADE_CMD       "/usr/sbin/image_writer"

void HNAP12_Method_PN_FirmwareUpload(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    char* pBuf;
    FILE* pfh = 0;
    int fResult = 0;
    unsigned int ccb;

    /* Acquire write lock, which will allow all current reads or current write to finish */
    if (UtopiaRWLock_WriteLock(&pUTCtx(pMethodCtx)->rwLock) != 0 &&
        (pBuf = HDK_XML_Get_Blob(pInput, HNAP12_Element_PN_Base64Image, &ccb)) != 0)
    {
        if ((pfh = fopen(UTOPIA_FW_UPLOAD_FILE, "wb")) != 0)
        {
            if (fwrite(pBuf, sizeof(char), ccb, pfh) == ccb)
            {
                fResult = !system(UTOPIA_FW_UPGRADE_CMD " " UTOPIA_FW_UPLOAD_FILE " > " UTOPIA_FW_UPGRADE_LOG " 2>&1");
            }

            fclose(pfh);
        }
    }

    /* If we succeeded, then reboot otherwise set the ERROR result */
    if (fResult != 0)
    {
        /* Set the reboot event */
        Utopia_SetEvent(pUTCtx(pMethodCtx), Utopia_Event_Reboot);
    }
    else
    {
        HNAP12_Set_PN_FirmwareUploadResult(pOutput, HNAP12_Element_PN_FirmwareUploadResult, HNAP12_Enum_PN_FirmwareUploadResult_ERROR);
    }
}

#endif /* __HNAP12_METHOD_PN_FIRMWAREUPLOAD__ */
