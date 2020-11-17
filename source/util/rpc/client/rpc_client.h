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

/**
 * @file   rpc_client.h
 * @author Comcast Inc
 * @date   13 May 2013
 * @brief  rpc client initialising api declaration 
 */

#ifndef RPCCLIENT_H
#define RPCCLIENT_H

#include <stdio.h>
#include <rpc/rpc.h>
//static CLIENT *clnt;

/**
 * @brief Function to initialise rpc client
 *
 * @param  mainArgv - ipaddress of machine where daemon is running 
 * @return None
 *
 */
int initRPC(char* mainArgv);

/**
 * @brief Function to get rpc client instance
 *
 * @param  None
 * @return CLIENT pointer -rpc client instance
 *
 */
CLIENT* getClientInstance();
bool isRPCConnectionLoss(char* errString);
#endif
