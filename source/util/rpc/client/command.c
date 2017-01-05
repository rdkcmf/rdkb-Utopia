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
	strcpy(commandBuf.buffer,cmnd);
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
	strcpy(commandBuf.buffer,cmnd);
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
    char l_cArmArpingIP[64] = {""};
    char l_cAtomArpingIP[64] = {""};
    if (NULL != l_fFp)
    {
        char props[255] = {""};
        while(fscanf(l_fFp,"%s", props) != EOF)
        {
            char *property = NULL;
            if(property = strstr(props, "ARM_ARPING_IP="))
            {
                property = property + strlen("ARM_ARPING_IP=");
                strncpy(l_cArmArpingIP, property, (strlen(props) - strlen("ARM_ARPING_IP=")));
            }
            if(property = strstr(props, "ATOM_ARPING_IP="))
            {
                property = property + strlen("ATOM_ARPING_IP=");
                strncpy(l_cAtomArpingIP, property, (strlen(props) - strlen("ATOM_ARPING_IP=")));
            }
        }
    }
    else
    {
        printf("Failed to open device.properties file:%s\n", DEVICE_PROPS_FILE);
    }
    fclose(l_fFp);

    if (0 == l_cArmArpingIP[0] || 0 == l_cAtomArpingIP[0])
    {
        printf("ARM / ATOM Interface IP is not present:%s %s\n", l_cArmArpingIP, l_cAtomArpingIP);
        return 1;
    }

    if (argc < 3) {
        printf("usage example: %s %s ls\n",argv[0],l_cArmArpingIP);
        return 1;
    }
    host = argv[1];
    if(strcmp(host,l_cArmArpingIP)!=0 && strcmp(host,l_cAtomArpingIP)!=0)
    {
        printf("Provided ip is wrong. ARM ip should be:%s and ATOM ip should be:%s\n", l_cArmArpingIP, l_cAtomArpingIP);
        return 1;
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
		return 1;
            }
            return 0;
        }
        iRet = ExecuteCommand(argv[2]);
        if(iRet == 0) {
            printf("RPC FAILED while executing the command:%s !!!\n", argv[2]);
	    return 1;
        }
        return 0;
    }
    else
    {
        printf("RPC FAILED while opening socket !!!\n");
        return 1;
    }  
    return 0;
}