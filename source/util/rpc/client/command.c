#include <stdio.h>
#include <stdbool.h>
#include "rpc_client.h"
#include "rpc_specification.h"
#include "pthread.h"
//char rpcServerIp[16] = "192.168.254.254";

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

}
int
main (int argc, char *argv[],char **args)
{
	char *host;
	char *armIp = "192.168.254.253";
	char *atomIp = "192.168.254.254";
    int iRet;
	if (argc < 3) {
	    printf("usage example: %s %s ls\n",argv[0],armIp);
		exit (1);
	}
	host = argv[1];
	if(strcmp(host,armIp)!=0 && strcmp(host,atomIp)!=0)
	{
	    printf("Server_host ip is wrong. ARM ip is 192.168.254.253 and ATOM ip is 192.168.254.254\n");
	    return 0;
	}
	iRet = initRPC(host);
	if(iRet == 1) {
		iRet = ExecuteCommand(argv[2]);
			if(iRet == 0) {
				printf("RPC FAILED !!!\n");
			}
	}
	
    exit (0);
}

