/*
 * This is sample code generated by rpcgen.
 * These are only templates and you can use them
 * as a guideline for developing your own functions.
 */
#include <stdio.h>
#include <rpc/rpc.h>
#include <stdbool.h>
#include "rpc_client.h"
#include "rpc_specification.h"
#include "pthread.h"

pthread_t tid[2];
bool isStarted = false;
bool isConnected = false;
static char rpcServerIp[16]={0} ;
static CLIENT *clnt;

bool getIsconnectedStatus()
{
	return isConnected;
}

bool isRPCConnectionLoss(char* errString)
{
	if(strstr(errString,"Connection reset by peer"))
	return true;
	else
	return false;
}

int connectRPC()
{   
	clnt = clnt_create(rpcServerIp,RPC_TOOL1,RPC_TOOL1_V1, "tcp");
		if( clnt != NULL)
		{
			printf("\n*** RPC CONNECTED ***\n");
			isConnected = true;
			return 1;
		} else {
			printf("\nRPC CONNECTION FAILED !!!!!\n");
			isConnected = false;
			clnt_pcreateerror (rpcServerIp);
			return 0;
		}
	return 1;
}

int initRPC(char* mainArgv)
{
	int ret;
	if (NULL == mainArgv)
	{
		fprintf (stderr, "Please provide the RPC server IP tp start...\n");
		exit (0);
	}

	/* We have IP being given to us */
	strcpy(rpcServerIp,mainArgv);
	ret = connectRPC();
	return ret;
}

void startRPCThread()
{
	int err;
	pthread_t tid;
	if(!isStarted)
	{
		clnt_destroy(clnt);
		isConnected  = false;
		clnt = NULL;
		err = pthread_create(&tid, NULL, (void *)&connectRPC, NULL);

		if (err != 0)
		    printf("\nstartRPCThread:can't create thread :[%s]", strerror(err));
		else
		    printf("\nstartRPCThread: Thread created successfully\n");
		isStarted = true;
	}
	return;

}

/**
 * @brief Function to get rpc client instance
 *
 * @param  None
 * @return CLIENT pointer -rpc client instance
 *
 */
CLIENT* getClientInstance()
{
	if (clnt == (CLIENT *)NULL) {
		printf("RPC tool client is null \n");
	}	
	return clnt;
}

