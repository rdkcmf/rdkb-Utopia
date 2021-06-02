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
#include <unistd.h>
#include "print_uptime.h"

// This function returns 
// 1 if BootTime information is printed in the log file.
// 0 if BootTime information is not printed in the log file. 
int main(int argc, char *argv[])
{
	if (argc < 2)
	{
		printf("Insufficient number of args return\n");
		return 0;
	}
	int opt;
	char *uptime = NULL;
	while((opt = getopt(argc, argv, "u:")) != -1)
	{
		switch(opt)
		{
			case 'u':
				uptime = optarg;
				break;
			case '?':
				printf("unknown option %c\n",opt);
				break;
		}
	}
	print_uptime(argv[optind], argv[optind+1], uptime);
	return 1;
}
