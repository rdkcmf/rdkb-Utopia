/************************************************************************************
  If not stated otherwise in this file or this component's Licenses.txt file the
  following copyright and licenses apply:

  Copyright 2018 RDK Management

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

  http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
**************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include "rpc_client.h"
#include "rpc_specification.h"
#include "pthread.h"
//char rpcServerIp[16] = "192.168.254.254";
#define DEVICE_PROPS_FILE   "/etc/device.properties"


int ExecuteCommand(char *cmnd)
{
	CLIENT *clnt = NULL;
	struct rpc_CommandBuf commandBuf;
	struct rpc_CommandBuf *output = NULL;
	strncpy(commandBuf.buffer,cmnd,sizeof(commandBuf.buffer)-1);
	commandBuf.buffer[sizeof(commandBuf.buffer)-1] = '\0';
	char* errStr;
	/*bool isconnected = getIsconnectedStatus();
	if(!isconnected) {	
		//startRPCThread();
		return 0;
	}*/
	clnt = getClientInstance();  
	if(clnt != NULL) {
		output=executecommand_1(&commandBuf,clnt);
		if(output == NULL){
			errStr = clnt_sperror(clnt,"RPC");
			if(isRPCConnectionLoss(errStr))
			return 0;
		}
	}
 
	 if(output != NULL) {
	 	printf("\n%s\n",output->buffer);
	 } else {
	 	printf("ATOM CONSOLE OUTPUT IS NULL\n");
	 }

	return 1;

}

int ExeSysCmd(char *cmnd)
{
	CLIENT *clnt = NULL;
        struct rpc_CommandBuf commandBuf;
	/* CID 135428: Unbounded source buffer*/
	strncpy(commandBuf.buffer,cmnd,sizeof(commandBuf.buffer)-1);
	commandBuf.buffer[sizeof(commandBuf.buffer)-1] = '\0';
        char* errStr;	
	int *output = NULL;

	clnt = getClientInstance();
        if(clnt != NULL) {

                output = exec_1(&commandBuf,clnt);
                if(output == NULL){
                        errStr = clnt_sperror(clnt,"RPC");
                        if(isRPCConnectionLoss(errStr))
                        return 0;
                }
        }
	return 1;
}

int
main (int argc, char *argv[],char **args)
{
    char *host;
    int iRet;
   
    FILE *l_fFp = fopen(DEVICE_PROPS_FILE, "r");
    /*CID 68716 : Dereference after null check*/
    if (NULL == l_fFp)
    {
        printf("Failed to open device.properties file:%s\n", DEVICE_PROPS_FILE);
        exit(0);
    }
    char l_cArmArpingIP[64] = {""};
    char l_cAtomArpingIP[64] = {""};
    char props[255] = {""};
    while(fscanf(l_fFp,"%s", props) != EOF)
    {
         char *property = NULL;
         if((property = strstr(props, "ARM_ARPING_IP=")))
         {
             property = property + strlen("ARM_ARPING_IP=");
	     /* CID 58698: Out-of-bounds access (OVERRUN) */
             strncpy(l_cArmArpingIP, property, sizeof(l_cArmArpingIP)-1);
	     l_cArmArpingIP[sizeof(l_cArmArpingIP)-1] = '\0';
         }
         if((property = strstr(props, "ATOM_ARPING_IP=")))
         {
             property = property + strlen("ATOM_ARPING_IP=");
	     /* CID 71622 : Out-of-bounds access (OVERRUN) */
             strncpy(l_cAtomArpingIP, property, sizeof(l_cAtomArpingIP)-1);
	     l_cAtomArpingIP[sizeof(l_cAtomArpingIP)-1] = '\0';
         }
    }
    fclose(l_fFp);

    if (0 == l_cArmArpingIP[0] || 0 == l_cAtomArpingIP[0])
    {
        printf("ARM / ATOM Interface IP is not present:%s %s\n", l_cArmArpingIP, l_cAtomArpingIP);
        exit(0);
    }

    if (argc < 3) {
        printf("usage example: %s %s ls\n",argv[0],l_cArmArpingIP);
        exit(0);
    }
    host = argv[1];
    if(strcmp(host,l_cArmArpingIP)!=0 && strcmp(host,l_cAtomArpingIP)!=0)
    {
        printf("Provided ip is wrong. ARM ip should be:%s and ATOM ip should be:%s\n", l_cArmArpingIP, l_cAtomArpingIP);
        exit(0);
    }
    iRet = initRPC(host);
    if(iRet == 1) 
    {
        if(strcmp(argv[2],"sh") == 0)
        {
            if(argv[3] != NULL) 
                iRet = ExeSysCmd(argv[3]);
            else
                iRet = ExecuteCommand(argv[2]);
            
            if(iRet == 0) {
                printf("RPC FAILED while executing the command:%s !!!\n", argv[2]);
            }
            exit(0);
        }
        iRet = ExecuteCommand(argv[2]);
        if(iRet == 0) {
            printf("RPC FAILED while executing the command:%s !!!\n", argv[2]);
        }
        exit(0);
    }
    else
    {
        printf("RPC FAILED while opening socket !!!\n");
        exit(0);
    }  
    exit(1);
}
