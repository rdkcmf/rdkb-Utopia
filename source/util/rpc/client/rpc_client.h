/*
 * ============================================================================
 * COMCAST C O N F I D E N T I A L AND PROPRIETARY
 * ============================================================================
 * This file and its contents are the intellectual property of Comcast.  It may
 * not be used, copied, distributed or otherwise  disclosed in whole or in part
 * without the express written permission of Comcast.
 * ============================================================================
 * Copyright (c) 2012 Comcast. All rights reserved.
 * ============================================================================
 */

/**
 * @file   ocgw_rpc_client.c
 * @author Comcast Inc
 * @date   13 May 2013
 * @brief  rpc client initialising api declaration 
 */

#ifndef RPCCLIENT_H
#define RPCCLIENT_H

#include <stdio.h>
#include <rpc/rpc.h>
static CLIENT *clnt;

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
#endif
