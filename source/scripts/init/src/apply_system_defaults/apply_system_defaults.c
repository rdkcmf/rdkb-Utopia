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
===================================================================
    This programs will compare syscfg database and sysevent database
    against a default database. If any tuple in syscfg or sysevent is
    not already set, then this program will set it according to the
    default value
===================================================================
*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <ctype.h>
#include <arpa/inet.h>
#include <syscfg/syscfg.h>
#include "sysevent/sysevent.h"
#include "platform_hal.h"
#include "cJSON.h"
#include <unistd.h>
#define PARTNERS_INFO_FILE  			"/nvram/partners_defaults.json"
#define PARTNERID_FILE  				"/nvram/.partner_ID"
#define PARTNER_DEFAULT_APPLY_FILE  	"/nvram/.apply_partner_defaults"
#define PARTNER_DEFAULT_MIGRATE_PSM  	"/tmp/.apply_partner_defaults_psm"
#define PARTNER_ID_LEN 64
static char default_file[1024];

// The sysevent server is local 
#define SE_WELL_KNOWN_IP    "127.0.0.1"
static short server_port;
static char  server_ip[19];
static int   syscfg_dirty;
#define DEFAULT_FILE "/etc/utopia/system_defaults"
#define SE_NAME "system_default_set"

static int global_fd = 0;

//Flag to indicate a db conversion is necessary
static int convert = 0;

// we can use one global id for sysevent because we are single threaded
token_t global_id;

#if defined (_CBR_PRODUCT_REQ_) || defined (_XB6_PRODUCT_REQ_)
        #define LOG_FILE "/rdklogs/logs/Consolelog.txt.0"
#else
	#define LOG_FILE "/rdklogs/logs/ArmConsolelog.txt.0"
#endif

#define APPLY_PRINT(fmt ...)   {\
   FILE *logfp = fopen ( LOG_FILE , "a+");\
   if (logfp)\
   {\
        fprintf(logfp,fmt);\
        fclose(logfp);\
   }\
}\

/* Function Prototypes */
int IsValuePresentinSyscfgDB( char *param );

/*
 * Procedure     : trim
 * Purpose       : trims a string
 * Parameters    :
 *    in         : A string to trim
 * Return Value  : The trimmed string
 * Note          : This procedure will change the input sting in situ
 */
static char *trim(char *in)
{
   // trim the front of the string
   if (NULL == in) {
      return(NULL);
   }
   char *start = in;
   while(isspace(*start)) {
      start++;
   }
   // trim the end of the string

   char *end = start+strlen(start);
   end--;
   while(isspace(*end)) {
      *end = '\0';
      end--;
   }
   return(start);
}

/*
 * Procedure     : parse_line
 * Purpose       : parses a line into a name and a value
 * Parameters    :
 *    in         : the line to parse
 *    name       : on return the name
 *    value      : on return the value
 * Return Value  : 0 if ok -1 if not
 * Note          : This function will change the contents of in
 */
static int parse_line(char *in, char **name, char **value)
{
   // look for the separator token
   if (NULL == in) {
      return(-1);
   } else {
      *name = *value = NULL;
   }

   char *tok = strchr(in, '=');
   if (NULL == tok) {
      return(-1);
   } else {
      *name=(in + 1);
      *value=(tok + 1); 
      *tok='\0';
   } 
   return(0);
}

/*
 * Procedure     : set_sysevent
 * Purpose       : sets a sysevent tuple if it is not already set
 * Parameters    :
 *    name       : the name of the tuple
 *    value      : the value to set the tuple to
 * Return Value  : 0 if ok, -1 if not
 */
static int set_sysevent(char *name, char *value, int flags) 
{
   int   rc = 0;
   char get_val[512];
   rc = sysevent_get(global_fd, global_id, name, get_val, sizeof(get_val));
   if ('\0' == get_val[0]) {
      if (0x00000000 != flags) {
         rc = sysevent_set_options(global_fd, global_id, name, flags);
      }

      // if the value is prefaced by '$' then we use the
      // current value of syscfg
      char *trimmed_val = trim(value);
      if ('$' == trimmed_val[0]) {
         get_val[0] = '\0';
         rc = syscfg_get(NULL, trimmed_val+1, get_val, sizeof(get_val));
         rc = sysevent_set(global_fd, global_id, name, get_val, 0);
//         printf("[utopia] [init] apply_system_defaults set <@%s, %s, 0x%x>\n", name, get_val, flags);
      } else {
         rc = sysevent_set(global_fd, global_id, name, value, 0);
         APPLY_PRINT("[utopia] [init] apply_system_defaults set <@%s, %s, 0x%x>\n", name, value, flags);
         printf("[utopia] [init] apply_system_defaults set <@%s, %s, 0x%x>\n", name, value, flags);
      }
   } else {
      rc = 0;
   }
   return(rc);
}

/*
 * Procedure     : set_syscfg
 * Purpose       : sets a syscfg tuple if it is not already set
 * Parameters    :
 *    name       : the name of the tuple
 *    value      : the value to set the tuple to
 * Return Value  : 0 if ok, -1 if not
 */
static int set_syscfg(char *name, char *value) 
{
   int rc = 0, force = 0;
   char get_val[512];
   char *ns = NULL;
   char *ns_delimiter;
   

   if (NULL == value || 0 == strlen(value)) {
      return(0);
   }
   
   //Note to force write if the value does not match and is marked, and increment past the mark.
   if (name[0] == '$') {
       if (convert)
           force = 1;
       name ++;
   }

   ns_delimiter = strstr(name, "::");
   if (ns_delimiter)
   {
      *ns_delimiter = '\0';
      ns = name;
      name = ns_delimiter+2;
   }
   else
   {
      ns = NULL;
   }

   get_val[0] = '\0';
   rc = syscfg_get(ns, name, get_val, sizeof(get_val));
   if (0 != rc || 0 == strlen(get_val) || (force && strcmp(get_val, value)) ) {
      rc = syscfg_set(ns, name, value);
      printf("[utopia] [init] apply_system_defaults set <$%s::%s, %s> set(rc=%d) get_val %s force %d  \n", ns, name, value, rc,get_val,force);
      syscfg_dirty++;
   } else {
      rc = 0;
      printf("[utopia] [init] syscfg_get <$%s::%s> \n", name,get_val);
   }
   return(rc);
}

static int handle_version(char* name, char* value) {
    int ret = 0;
    int rc;
    char get_val[512];
    
    if (!strcmp(name, "$Version")) {
        ret = 1;
        name++;
        rc = syscfg_get(NULL, name, get_val, sizeof(get_val));
        if (rc != 0 || 0 == strlen(get_val) || strcmp(value, get_val))
            convert = 1;
    }

    return ret;
}

static int check_version(void) {
   int handled = 0;
   FILE *fp = fopen(default_file, "r");
   if (NULL == fp) {
      printf("[utopia] no system default file (%s) found\n", default_file);
      return(-1);
   }

   /*
    * The default file must contain one default per line in the format
    * name=value (whitespace is allowed)
    * If the default is for a syscfg tuple, then name must be preceeded with a $
    * If the default is for a sysevent tuple, then name must be preceeded with a @
    * If the first character in the line is # then the line will be ignored
    */
   
   char buf[1024];
   char *line; 
   char *name;
   char *value;
   while (NULL != fgets(buf, sizeof(buf)-1, fp)) {
      line = trim(buf);
      if ('#' == line[0]) {
         // this is a comment
      } else if (0x00 == line[0]) {
         // this is an empty line
      } else if ('$' == line[0]) {
         if (0 !=  parse_line(line, &name, &value)) {
            printf("[utopia] [error] set_defaults failed to set syscfg (%s)\n", line);
         } else { 
            if ( handle_version(trim(name), trim(value)) ) {
                handled = 1;
                break;
            }
         }
      } else if ('@' == line[0]) {
         // this is a sysevent line
      } else {
         // this is a malformed line
         printf("[utopia] set_defaults found a malformed line (%s)\n", line);
      }
   }

   fclose(fp); 
   return(0);
}

/*
 * Procedure     : set_syscfg_defaults
 * Purpose       : Go through a file, parse it into <name, value> tuples,
 *                 and set syscfg namespace (iff not already set),
 * Parameters    :
 * Return Value  : 0 if ok, -1 if not
 */
static int set_syscfg_defaults (void)
{
   FILE *fp = fopen(default_file, "r");
   if (NULL == fp) {
      printf("[utopia] no system default file (%s) found\n", default_file);
      return(-1);
   }

   /*
    * The default file must contain one default per line in the format
    * name=value (whitespace is allowed)
    * If the default is for a syscfg tuple, then name must be preceeded with a $
    * If the default is for a sysevent tuple, then name must be preceeded with a @
    * If the first character in the line is # then the line will be ignored
    */
   
   char buf[1024];
   char *line; 
   char *name;
   char *value;
   while (NULL != fgets(buf, sizeof(buf)-1, fp)) {
      line = trim(buf);
      if ('#' == line[0]) {
         // this is a comment
      } else if (0x00 == line[0]) {
         // this is an empty line
      } else if ('$' == line[0]) {
         if (0 !=  parse_line(line, &name, &value)) {
            printf("[utopia] [error] set_defaults failed to set syscfg (%s)\n", line);
         } else { 
            set_syscfg(trim(name), trim(value));
         }
      } else if ('@' == line[0]) {
         // this is a sysevent line
      } else {
         // this is a malformed line
         printf("[utopia] set_defaults found a malformed line (%s)\n", line);
      }
   }
   fclose(fp); 
   return(0);
}

/*
 * Procedure     : set_sysevent_defaults
 * Purpose       : Go through a file, parse it into <name, value> tuples,
 *                 and set sysevent namespace
 * Parameters    :
 * Return Value  : 0 if ok, -1 if not
 */
static int set_sysevent_defaults (void)
{
   FILE *fp = fopen(default_file, "r");
   if (NULL == fp) {
      printf("[utopia] no system default file (%s) found\n", default_file);
      return(-1);
   }

   /*
    * The default file must contain one default per line in the format
    * name=value (whitespace is allowed)
    * If the default is for a syscfg tuple, then name must be preceeded with a $
    * If the default is for a sysevent tuple, then name must be preceeded with a @
    * If the first character in the line is # then the line will be ignored
    */
   
   char buf[1024];
   char *line; 
   char *name;
   char *value;
   while (NULL != fgets(buf, sizeof(buf)-1, fp)) {
      line = trim(buf);
      if ('#' == line[0]) {
         // this is a comment
      } else if (0x00 == line[0]) {
         // this is an empty line
      } else if ('$' == line[0]) {
         // this is a syscfg line
      } else if ('@' == line[0]) {
         if (0 !=  parse_line(line, &name, &value)) {
            printf("[utopia] set_defaults failed to set sysevent (%s)\n", line);
         } else { 
            char *val = trim(value);
            char *flagstr;
            int flags = 0x00000000;

            int i;
            int len = strlen(val);
            for (i=0; i<len; i++) {
               if (isspace(val[i])) {
                  flagstr = (&(val[i])+1);
                  val[i] = '\0';
                  flags = strtol(flagstr, NULL, 16);
                  break;
               }
            } 
            set_sysevent(trim(name), val, flags);
         }
      } else {
         // this is a malformed line
         printf("[utopia] set_defaults found a malformed line (%s)\n", line);
      }
   }
   fclose(fp); 
   return(0);
}

/*
 * Procedure     : set_defaults
 * Purpose       : Go through a file twice, first for syscfg variables 
 *                 (because sysevent might use syscfg values for initialization),
 *                 and then again for sysevent variables
 * Parameters    :
 * Return Value  : 0 if ok, -1 if not
 */
static int set_defaults(void)
{
   check_version();
   set_syscfg_defaults();
   set_sysevent_defaults();
   return(0);
}

static void printhelp(char *name) {
      printf ("Usage %s --file default_file --port syseventd_port --ip syseventd_ip --help command params\n", name);
}

/*
 * Procedure     : parse_command_line
 * Purpose       : if any command line args then apply them
 * Parameters    :
 * Return Value  : 0 if ok, -1 if not
 */
static int parse_command_line(int argc, char **argv)
{
   int c;
   while (1) {
      int option_index = 0;
      static struct option long_options[] = {
         {"file", 1, 0, 'f'},
         {"port", 1, 0, 'p'},
         {"ip", 1, 0, 'd'},
         {"help", 0, 0, 'h'},
         {0, 0, 0, 0}
      };

      // optstring has a leading : to stop debug output
      // f takes an argument
      // p takes an argument
      // i takes an argument
      // h takes no argument
      c = getopt_long (argc, argv, ":f:p:i:h", long_options, &option_index);
      if (c == -1) {
         break;
      }

      switch (c) {
         case 'f':
            snprintf(default_file, sizeof(default_file), "%s", optarg);
            break;
        
         case 'p':
            server_port = htons(atoi(optarg));
            break;

         case 'i':
            snprintf(server_ip, sizeof(server_ip), "%s", optarg);
            break;

         case 'h':
         case '?':
            printhelp(argv[0]);
            exit(0);
            break;

         default:
            printhelp(argv[0]);
            break;
      }
   }
   return(0);
}

char * json_file_parse( char *path )
{
	cJSON 		*json 		= NULL;
	FILE	 	*fileRead 	= NULL;
	char		*data 		= NULL;
	int 		 len 		= 0;

	//File read
	fileRead = fopen( path, "r" );

	//Null Check
	if( fileRead == NULL ) 
	{
	 	APPLY_PRINT( "%s-%d : Error in opening JSON file\n" , __FUNCTION__, __LINE__ );
		return NULL;
	}

	//Calculate length for memory allocation	 
	fseek( fileRead, 0, SEEK_END );
	len = ftell( fileRead );
	fseek( fileRead, 0, SEEK_SET );

	APPLY_PRINT("%s-%d : Total File Length :%d \n", __FUNCTION__, __LINE__, len );

	if( len > 0 )
 	{
		 data = ( char* )malloc( len + 1 );

		 //Check memory availability
		 if ( data != NULL ) 
		 {
			fread( data, 1, len, fileRead );
		 } 
		 else 
		 {
			 APPLY_PRINT("%s-%d : Memory allocation failed Length :%d\n", __FUNCTION__, __LINE__, len );
		 }
 	}
	 
	if( fileRead )
	fclose( fileRead );
	
	return data;
}

static int writeToJson(char *data)
{
    FILE 	*fp;
    fp = fopen(PARTNERS_INFO_FILE, "w");
    if (fp == NULL) 
    {
        return -1;
    }
    
    fwrite(data, strlen(data), 1, fp);
    fclose(fp);
    return 0;
}

/* IsValuePresentinSyscfgDB() */
int IsValuePresentinSyscfgDB( char *param )
{
	char buf[ 512 ];
	int  ret;

	//check whether passed param with value is already existing or not
	memset( buf, sizeof( buf ), 0 );
	ret = syscfg_get( NULL, param, buf, sizeof(buf));

	if( ( ret != 0 ) || ( buf[ 0 ] == '\0' ) )
	{
		return 0;
	}

	return 1;
}

int set_syscfg_partner_values(char *pValue,char *param)
{
	if ((syscfg_set(NULL, param, pValue) != 0)) 
	{
        	APPLY_PRINT("setPartnerId : syscfg_set failed\n");
		return ;
	}
	else 
	{
       	 	if (syscfg_commit() != 0) 
		{
			APPLY_PRINT("setPartnerId : syscfg_commit failed\n");
			return ;
		}
		return 0;
	}
}

int GetDevicePropertiesEntry( char *pOutput, int size, char *sDevicePropContent )
{
    FILE 	*fp1 		 = NULL;
    char 	 buf[ 1024 ] = { 0 },
	  		*urlPtr 	 = NULL;
    int 	 ret		 = -1;

    // Read the device.properties file 
    fp1 = fopen( "/etc/device.properties", "r" );
	
    if ( fp1 == NULL ) 
	{
        APPLY_PRINT("Error opening properties file! \n");
        return -1;
    }

    while ( fgets( buf, sizeof( buf ), fp1 ) != NULL ) 
    {
        // Look for Device Properties Passed Content
        if ( strstr( buf, sDevicePropContent ) != NULL ) 
	{
		 buf[strcspn( buf, "\r\n" )] = 0; // Strip off any carriage returns
		 // grab content from string(entry)
		urlPtr = strstr( buf, "=" );
		urlPtr++;
		strncpy( pOutput, urlPtr, size );
		ret=0;
		break;
        }
    }

    fclose( fp1 );
    return ret;
}

static int getFactoryPartnerId
	(
		char*                       pValue
	)
{
#if defined (_XB6_PRODUCT_REQ_)
	if(0 == platform_hal_getFactoryPartnerId(pValue))
	{
		APPLY_PRINT("%s - %s\n",__FUNCTION__,pValue);
		return 0;		 
	}
	else
	{
		int count = 0 ;
		while ( count < 3 )
		{
			APPLY_PRINT(" Retrying for getting partnerID from HAL, Retry Count:%d\n", count + 1);
			if(0 == platform_hal_getFactoryPartnerId(pValue))
			{
				APPLY_PRINT("%s - %s\n",__FUNCTION__,pValue);
				return 0;
			}
			sleep(3);
			count++;
		}
	}
#endif

	APPLY_PRINT("%s - Failed Get factoryPartnerId \n", __FUNCTION__);
	return -1;
}
int validatePartnerId ( char *PartnerID )
{
	char *ptr_nvram_json = NULL;

	ptr_nvram_json = json_file_parse ( "/etc/partners_defaults.json" ) ;

	if ( ptr_nvram_json )
	{
		cJSON 	*subitem_nvram 	= 	NULL, 	* root_nvram_json = NULL ;
		char 	*param_nvram 	= 	NULL;
		int 		row_count = 0 , 	nvram_val = 0, 	flag = 0;

		root_nvram_json = cJSON_Parse(ptr_nvram_json);
		row_count = cJSON_GetArraySize(root_nvram_json);

		for (nvram_val = 0; nvram_val < row_count ; nvram_val++)
		{
		        if ( (cJSON_GetArrayItem(root_nvram_json, nvram_val) != NULL) )
		        {
		                subitem_nvram = cJSON_GetArrayItem(root_nvram_json,nvram_val);
				param_nvram = cJSON_GetArrayItem(root_nvram_json,nvram_val)->string;
				if ( 0 == strcmp ( param_nvram, PartnerID) )
				{
					break;
				}
				else
				{
					flag++;
				}
			}
		}
		if ( flag == row_count )
		{
			APPLY_PRINT("Invalid partner ID got, So assigning as default partnerID as unknown\n");
			sprintf(PartnerID,"%s","unknown");
		}
		cJSON_Delete(root_nvram_json);
	}
	free ( ptr_nvram_json ) ;

}

static int get_PartnerID( char *PartnerID)
{
	char buf[PARTNER_ID_LEN];
	memset(buf, 0, sizeof(buf));

	/* 
	  *  Check whether /nvram/.partner_ID file is available or not. 
	  *  If available then read it and apply defaults based on new partnerID
	  *  If not available then read it from HAL and create the /nvram/.partner_ID file
	  *     then apply defaults based on current partnerID	  
	  */
	if ( access( PARTNERID_FILE , F_OK ) != 0 )	 
	{

		APPLY_PRINT("%s - %s is not there\n", __FUNCTION__, PARTNERID_FILE );
		if( ( 0 == getFactoryPartnerId( PartnerID ) ) && ( PartnerID [ 0 ] != '\0' ) )
		{
			APPLY_PRINT("%s - PartnerID from HAL: %s\n",__FUNCTION__,PartnerID );
			validatePartnerId ( PartnerID );
		}
		else
		{
			if ( 0 == GetDevicePropertiesEntry( buf, sizeof( buf ),"PARTNER_ID" ) )
			{
				if( buf != NULL )
				{
				    strncpy(PartnerID,buf,strlen(buf));
				    APPLY_PRINT("%s - PartnerID from device.properties: %s\n",__FUNCTION__,PartnerID );
				}
			}
			else		
			{
				sprintf( PartnerID, "%s", "comcast" );
				APPLY_PRINT("%s - Failed Get factoryPartnerId so set it PartnerID as: %s\n", __FUNCTION__, PartnerID );
			}
		}
	}
	else
	{
		FILE	   *FilePtr 			= NULL;
		char		fileContent[ 256 ]	= { 0 };
		
		FilePtr = fopen( PARTNERID_FILE, "r" );
		
		if ( FilePtr ) 
		{
			char *pos;
		
			fgets( fileContent, 256, FilePtr );
			fclose( FilePtr );
			FilePtr = NULL;
			
			// Remove line \n charecter from string  
			if ( ( pos = strchr( fileContent, '\n' ) ) != NULL )
			 *pos = '\0';

			sprintf( PartnerID, "%s", fileContent );

			APPLY_PRINT("%s - PartnerID from File: %s\n",__FUNCTION__,PartnerID );
			validatePartnerId ( PartnerID );
		}
		system("rm -rf /nvram/.partner_ID");
	}
	set_syscfg_partner_values(PartnerID,"PartnerID");

	//To print Facgtory PartnerID on every boot-up
	memset(buf, 0, sizeof(buf));
	if( 0 == getFactoryPartnerId( buf ) )
	{
		APPLY_PRINT("[GET-PARTNERID] Factory_PartnerID:%s\n", buf );
	}
   	else
    {
       APPLY_PRINT("[GET-PARTNERID] Factory_PartnerID:NULL\n" );
   	}

	APPLY_PRINT("[GET-PARTNERID] Current_PartnerID:%s\n", PartnerID );
	
}

char compare_partner_json_param(char *partner_nvram_obj,char *partner_etc_obj,char *PartnerID)
{

	cJSON 	*subitem_nvram = NULL, 	*subitem_etc = NULL;
	char 	*key = NULL, 	*value = NULL, *param_etc = NULL, *param_nvram = NULL;
        int 	rowCnt_etc = 0, 	rowCnt_nvram = 0, 	etc_val = 0, nvram_val = 0, 	count = 0, 	count_etc = 0, 	count_nvram = 0;

	cJSON * root_nvram_json = cJSON_Parse(partner_nvram_obj);
	cJSON * root_etc_json = cJSON_Parse(partner_etc_obj);	

	rowCnt_etc = cJSON_GetArraySize(root_etc_json);
	rowCnt_nvram = cJSON_GetArraySize(root_nvram_json);
	
	for (etc_val = 0; etc_val < rowCnt_etc ; etc_val++)
	{
		subitem_etc = cJSON_GetArrayItem(root_etc_json,etc_val);
		
		if ( (cJSON_GetArrayItem(root_etc_json,etc_val) != NULL) )
		{
			int etc_paramCount = cJSON_GetArraySize(subitem_etc);
			param_etc = cJSON_GetArrayItem(root_etc_json,etc_val)->string;
			for( nvram_val = 0 ; nvram_val < rowCnt_nvram ; nvram_val++)
			{	
				subitem_nvram = cJSON_GetArrayItem(root_nvram_json,nvram_val);
				int nvram_paramCount = cJSON_GetArraySize(subitem_nvram);
				
				if (  (cJSON_GetArrayItem(root_nvram_json,nvram_val) != NULL) )
				{
					param_nvram = cJSON_GetArrayItem(root_nvram_json,nvram_val)->string;
					if ( strcmp(param_etc, param_nvram) == 0 )
					{
					for( count_etc = 0 ; count_etc < etc_paramCount ; count_etc++)
					{

						if (cJSON_GetArrayItem(subitem_etc, count_etc) != NULL)
						{
							if (cJSON_GetArrayItem(subitem_etc, count_etc)->string != NULL)
							{
								count = 0;
								for( count_nvram = 0 ;count_nvram< nvram_paramCount ; count_nvram++)
								{
					
									if ( cJSON_GetArrayItem(subitem_nvram,count_nvram) != NULL)
									{
										if (cJSON_GetArrayItem(subitem_nvram, count_nvram)->string != NULL)
										{
											if( strcmp( (cJSON_GetArrayItem(subitem_etc, count_etc)->string) , (cJSON_GetArrayItem(subitem_nvram, count_nvram)->string)) != 0 )
											{
												count++;
											}
											else
											{
												break;
											}
										}
										else
										{
											printf ("Nvram JSON file has empty Parameter list\n");

										}
									}

								}

								if (count == nvram_paramCount )
								{
									printf("Adding new param to JSON file\n");
									key = cJSON_GetArrayItem(subitem_etc, count_etc)->string;
									value = cJSON_GetArrayItem(subitem_etc, count_etc)->valuestring;

									// Add newly introduced entry from /etc/ directory to /nvram directory
									cJSON_AddItemToObject(subitem_nvram, key, cJSON_CreateString(value));
									
					                  if ( ( strcmp( param_nvram,PartnerID ) == 0 ) )
					                  {
						                    if ( 0 == strcmp ( key, "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.SyndicationFlowControl.InitialForwardedMark") )
						                    {
												if ( 0 == IsValuePresentinSyscfgDB( "DSCP_InitialForwardedMark" ) )
												{
													set_syscfg_partner_values( value,"DSCP_InitialForwardedMark" );
												}
						                    }
                                                                    if ( 0 == strcmp ( key, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.WANsideSSH.Enable") )
                                                                    {
                                                                                                if ( 0 == IsValuePresentinSyscfgDB( "WANsideSSH_Enable" ) )
                                                                                                {
                                                                                                        set_syscfg_partner_values( value,"WANsideSSH_Enable" );
                                                                                                }
                                                                    }


						                    if ( 0 == strcmp ( key, "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.SyndicationFlowControl.InitialOutputMark") )
						                    {
												if ( 0 == IsValuePresentinSyscfgDB( "DSCP_InitialOutputMark" ) )
												{
													set_syscfg_partner_values( value,"DSCP_InitialOutputMark" );
												}
						                    }
					                  }
								}
							}
						}
		
					}
					}
				
				}
				
			}

		}

	}

	for (etc_val = 0; etc_val < rowCnt_etc ;etc_val++)
	{
		int flag = 0;
		for (nvram_val = 0; nvram_val < rowCnt_nvram;nvram_val++)
		{
			if ( strcmp ( cJSON_GetArrayItem(root_nvram_json,nvram_val)->string , cJSON_GetArrayItem(root_etc_json,etc_val)->string ) == 0)
			{
				break;
			}
			else
			{
				flag++;
				printf("flag is %d\n",flag);
			}
		}
		if ( flag == rowCnt_nvram)
		{
			printf("Adding New PartnerId details to nvram JSON file\n");	
			cJSON_AddItemToArray(root_nvram_json,  cJSON_GetArrayItem(root_etc_json,etc_val));
			break;
		}
				
	}

	char *out = cJSON_Print(root_nvram_json);
	writeToJson(out);
	out = NULL;

	cJSON_Delete(root_nvram_json);
}

int apply_partnerId_default_values(char *data, char *PartnerID)
{
	cJSON 	*partnerObj 	= NULL;
	cJSON 	*json 			= NULL;
	char 	*userName 		= NULL, 
		    *defaultAdminIP = NULL,	 
		    *passWord 		= NULL,	 
		    *subnetRange 	= NULL,
	*minAddress = NULL,
	*maxAddress = NULL,
        *allow_ethernet_wan = NULL,
        *initialForwardedMark = NULL,
        *initialOutputMark = NULL;
	int	    isNeedToApplyPartnersDefault = 1;
	int	    isNeedToApplyPartnersPSMDefault = 0;	

	/*
	  * Case 1:
	  * Check whether PartnerID is comcast of not. 
	  * if "comcast" then we don't want to apply any defaults 
	  * if not "comcast" then we should apply partners defaults
	  *
	  * Case 2:
	  * Check whether PartnerID is comcast of not. 
	  * if "/nvram/.apply_partner_defaults" file available or not
	  * if available then go ahead and apply default values corresponding partners
	  * if not available then it would have applied before boot-up
	  *
	  * Case 3:
	  * Check whether /tmp/.apply_partner_defaults_psm file has touched or not
	  * if touched then we need to do migration for PSM members like RegionCode and CertLocation only
	  *
	  */
	if ( access( PARTNER_DEFAULT_APPLY_FILE , F_OK ) != 0 )  
	{
		isNeedToApplyPartnersDefault = 0;
	}
	else
  	{
   		APPLY_PRINT("%s - Deletion of %s file handled in PSM init :%s\n", __FUNCTION__, PARTNER_DEFAULT_APPLY_FILE );
		//Delete at PSM init
		//system( "rm -rf /nvram/.apply_partner_defaults" );
  	}

	if ( access( PARTNER_DEFAULT_MIGRATE_PSM , F_OK ) == 0 )  
	{
		isNeedToApplyPartnersPSMDefault = 1;

   		APPLY_PRINT("%s - %s file available so need to do partner's PSM member migration :%s\n", __FUNCTION__, PARTNER_DEFAULT_MIGRATE_PSM );
   		APPLY_PRINT("%s - Deletion of %s file handled in PSM init :%s\n", __FUNCTION__, PARTNER_DEFAULT_MIGRATE_PSM );
	}

	if( ( 1 == isNeedToApplyPartnersDefault ) || \
		( 1 == isNeedToApplyPartnersPSMDefault ) 
	  )
	{
          	APPLY_PRINT("%s - Applying  %s default configuration\n", __FUNCTION__, PartnerID );
		json = cJSON_Parse( data );
		if( !json ) 
		{
			APPLY_PRINT(  "%s-%d : json file parser error : [%s]\n", __FUNCTION__,__LINE__, cJSON_GetErrorPtr() );
			return ;
		} 
		else
		{
			int isThisComcastPartner = 0;
			
			//Check whether this is comcast partner or not
			if( 0 == strcmp( "comcast", PartnerID ) )
			{
				isThisComcastPartner = 1;
			}
				
			partnerObj = cJSON_GetObjectItem( json, PartnerID );
			if( partnerObj != NULL) 
			{
				// Don't overwrite this value into syscfg.db for comcast partner
				if( ( 0 == isThisComcastPartner ) && \
					( 1 == isNeedToApplyPartnersDefault )
				  )
				{
					if ( cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.RDKB_UIBranding.LocalUI.DefaultLoginUsername") != NULL )
					{
						userName = cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.RDKB_UIBranding.LocalUI.DefaultLoginUsername")->valuestring; 
					
						if (userName != NULL) 
						{
							set_syscfg_partner_values(userName,"user_name_3");
							userName = NULL;
						}	
						else
						{
							APPLY_PRINT("%s - DefaultLoginUsername Value is NULL\n", __FUNCTION__ );
						}	
					}
					
					if ( cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.RDKB_UIBranding.LocalUI.DefaultLoginPassword") != NULL )
					{
						passWord = cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.RDKB_UIBranding.LocalUI.DefaultLoginPassword")->valuestring; 
					
						if (passWord != NULL) 
						{
							set_syscfg_partner_values(passWord,"user_password_3");
							passWord = NULL;
						}	
						else
						{
							APPLY_PRINT("%s - DefaultLoginUsername Value is NULL\n", __FUNCTION__ );
						}	
					}
					
					if ( cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.RDKB_UIBranding.DefaultAdminIP") != NULL )
					{
						defaultAdminIP = cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.RDKB_UIBranding.DefaultAdminIP")->valuestring; 
					
						if (defaultAdminIP != NULL) 
						{
							set_syscfg_partner_values(defaultAdminIP,"lan_ipaddr");
							defaultAdminIP = NULL;
						}	
						else
						{
							APPLY_PRINT("%s - DefaultAdminIP Value is NULL\n", __FUNCTION__ );
						}	
					}
					
					if ( cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.RDKB_UIBranding.DefaultLocalIPv4SubnetRange") != NULL )
					{
						subnetRange = cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.RDKB_UIBranding.DefaultLocalIPv4SubnetRange")->valuestring; 
					
						if (subnetRange != NULL) 
						{
							set_syscfg_partner_values(subnetRange,"lan_netmask");
							subnetRange = NULL;
						}	
						else
						{
							APPLY_PRINT("%s - DefaultLocalIPv4SubnetRange Value is NULL\n", __FUNCTION__ );
						}	
					}
					if ( cJSON_GetObjectItem( partnerObj, "Device.DHCPv4.Server.Pool.1.MinAddress") != NULL )
					{

						minAddress = cJSON_GetObjectItem( partnerObj, "Device.DHCPv4.Server.Pool.1.MinAddress")->valuestring;

						if (minAddress != NULL)
						{
							set_syscfg_partner_values(minAddress,"dhcp_start");
							minAddress = NULL;
						}
						else
						{
							APPLY_PRINT("%s - Default DHCP minAddress Value is NULL\n", __FUNCTION__ );
						}
					}
                                        if ( cJSON_GetObjectItem( partnerObj, "Device.DHCPv4.Server.Pool.1.MaxAddress") != NULL )
                                        {

                                                maxAddress = cJSON_GetObjectItem( partnerObj, "Device.DHCPv4.Server.Pool.1.MaxAddress")->valuestring;

                                                if (maxAddress != NULL)
                                                {
                                                        set_syscfg_partner_values(maxAddress,"dhcp_end");
                                                        maxAddress = NULL;
                                                }
                                                else
                                                {
                                                        APPLY_PRINT("%s - Default DHCP maxAddress Value is NULL\n", __FUNCTION__ );
                                                }
                                        }
                                          if ( cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.RDKB_UIBranding.AllowEthernetWAN") != NULL )
                                          {
                                            allow_ethernet_wan = cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.RDKB_UIBranding.AllowEthernetWAN")->valuestring; 

                                            if (allow_ethernet_wan != NULL)
                                            {
                                              set_syscfg_partner_values(allow_ethernet_wan,"AllowEthernetWAN");
                                              allow_ethernet_wan = NULL;
                                            }
                                            else
                                            {
                                              APPLY_PRINT("%s - AllowEthernetWAN Value is NULL\n", __FUNCTION__ );
                                            }
                                          }
				}

				if( ( 1 == isNeedToApplyPartnersDefault ) || \
						( 1 == isNeedToApplyPartnersPSMDefault ) 
					  )
				{
					if ( cJSON_GetObjectItem( partnerObj, "Device.WiFi.X_RDKCENTRAL-COM_Syndication.WiFiRegion.Code") != NULL )
					{
						char *pcWiFiRegionCode = NULL;
						
						pcWiFiRegionCode = cJSON_GetObjectItem( partnerObj, "Device.WiFi.X_RDKCENTRAL-COM_Syndication.WiFiRegion.Code")->valuestring; 
			
						if (pcWiFiRegionCode != NULL) 
						{
							set_syscfg_partner_values(pcWiFiRegionCode,"WiFiRegionCode");
							pcWiFiRegionCode = NULL;
						}	
						else
						{
							APPLY_PRINT("%s - DefaultWiFiRegionCode Value is NULL\n", __FUNCTION__ );
						}	
					}

					if ( cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.TR69CertLocation") != NULL )
					{
							char *tr69CertLocation = NULL;
					
							tr69CertLocation = cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.TR69CertLocation")->valuestring;
					
							if (tr69CertLocation != NULL)
							{
									set_syscfg_partner_values(tr69CertLocation,"TR69CertLocation");
									tr69CertLocation = NULL;
							}
							else
							{
									APPLY_PRINT("%s - Default TR69CertLocation Value is NULL\n", __FUNCTION__ );
							}
					}
				}

				if( 1 == isNeedToApplyPartnersDefault )
				{
					if ( cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.SyndicationFlowControl.InitialForwardedMark") != NULL )
					{
					  initialForwardedMark = cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.SyndicationFlowControl.InitialForwardedMark")->valuestring; 
					  if (initialForwardedMark[0] != NULL)
					  {
						set_syscfg_partner_values(initialForwardedMark,"DSCP_InitialForwardedMark");
						initialForwardedMark = NULL;
					  }
					}
					else
					{
					  APPLY_PRINT("%s - Default Value of InitialForwardedMark is NULL\n", __FUNCTION__ );
					}

					if ( cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.SyndicationFlowControl.InitialOutputMark") != NULL )
					{
					  initialOutputMark = cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.SyndicationFlowControl.InitialOutputMark")->valuestring; 
					  if (initialOutputMark[0] != NULL)
					  {
						set_syscfg_partner_values(initialOutputMark,"DSCP_InitialOutputMark");
						initialOutputMark = NULL;
					  }
					}
					else
					{
					  APPLY_PRINT("%s - Default Value of InitialOutputMark is NULL\n", __FUNCTION__ );
					}

					if ( cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.WANsideSSH.Enable") != NULL )
					{
							char *WANsideSSHEnable = NULL;

							WANsideSSHEnable = cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.WANsideSSH.Enable")->valuestring;

							if (WANsideSSHEnable != NULL)
							{
									set_syscfg_partner_values(WANsideSSHEnable,"WANsideSSH_Enable");
									WANsideSSHEnable = NULL;
							}
							else
							{
									APPLY_PRINT("%s - Default WANsideSSHEnable Value is NULL\n", __FUNCTION__ );
							}
					}
					else
					{
						APPLY_PRINT("%s - Default WANsideSSHEnable object is NULL\n", __FUNCTION__ );
					}

				}
			}
			else
			{
				APPLY_PRINT("%s - partnerObj Object is NULL\n", __FUNCTION__ );
			}
		}
	}
}

/*
 * main()
 */
int main( int argc, char **argv )
{
   char *ptr_etc_json = NULL, *ptr_nvram_json = NULL, *db_val = NULL;
   char  cmd[512] = {0};
   char  PartnerID[ PARTNER_ID_LEN ]  = { 0 };
   int   isNeedToApplyPartnersDefault = 1;
   int   rc;

   //Fill basic contents
   server_port = SE_SERVER_WELL_KNOWN_PORT;

   snprintf( server_ip, sizeof( server_ip ), "%s", SE_WELL_KNOWN_IP );
   snprintf( default_file, sizeof( default_file ), "%s", DEFAULT_FILE );

   syscfg_dirty = 0;

   parse_command_line(argc, argv);

   global_fd = sysevent_open(server_ip, server_port, SE_VERSION, SE_NAME, &global_id);
   APPLY_PRINT("[Utopia] global_fd is %d\n",global_fd);
   
   if ( 0 == global_fd ) 
   {
      APPLY_PRINT("[Utopia] %s unable to register with sysevent daemon.\n", argv[0]);
      printf("[Utopia] %s unable to register with sysevent daemon.\n", argv[0]);
   }

   rc = syscfg_init();
   if ( rc ) 
   {
      APPLY_PRINT("[Utopia] %s unable to initialize with syscfg context. Reason (%d)\n", argv[0], rc);
      sysevent_close(global_fd, global_id);
      return(-1);
   }

   if ( global_fd <= 0 )
   {		
		APPLY_PRINT("[Utopia] Retrying sysevent open\n");

		global_fd=0;
	   	global_fd = sysevent_open(server_ip, server_port, SE_VERSION, SE_NAME, &global_id);
		APPLY_PRINT("[Utopia] Global fd after retry is %d\n",global_fd);	

		if ( global_fd <= 0)
		APPLY_PRINT("[Utopia] Retrying sysevent open also failed %d\n",global_fd);
	
   }

   set_defaults();
   
   if (syscfg_dirty) 
   {
      printf("[utopia] [init] committing default syscfg values\n");
      syscfg_commit();
   }

   sysevent_close(global_fd, global_id);

#if defined(_SYNDICATION_BUILDS_)
   system( "sh /lib/rdk/apply_partner_customization.sh" );
#endif

  if ( access( PARTNER_DEFAULT_APPLY_FILE , F_OK ) != 0 )  
  {
	  isNeedToApplyPartnersDefault = 0;
      APPLY_PRINT("%s - Device in Reboot mode :%s\n", __FUNCTION__, PARTNER_DEFAULT_APPLY_FILE );
  }
  else
  {
	  APPLY_PRINT("%s - Device in FR mode :%s\n", __FUNCTION__, PARTNER_DEFAULT_APPLY_FILE );
  }
  
  if( 1 == isNeedToApplyPartnersDefault )  
  {
  	get_PartnerID ( PartnerID );
  }
  else
  {
    char buf[ 64 ] = { 0 };

#if defined(INTEL_PUMA7)
	//Below validation is needed to make sure the factory_partnerid and syscfg_partnerid are in sync.
	//This is mainly to address those units where customer_index/factory_partnerid was modified in the field through ARRISXB6-8400.
	system( "sh /lib/rdk/validate_syscfg_partnerid.sh" );
#endif
	
	//Get the partner ID
  	syscfg_get( NULL, "PartnerID", buf, sizeof( buf ));

	//Copy is it is not NULL. 
    if( buf[ 0 ] != '\0' )
    {
        strncpy( PartnerID, buf , strlen( buf ) );
    }
	else
	{
		//Partner ID is null so need to set default partner ID as "comcast"
		memset( PartnerID, 0, sizeof( PartnerID ) );
		sprintf( PartnerID, "%s", "comcast" );
		set_syscfg_partner_values( PartnerID, "PartnerID" );
		APPLY_PRINT("%s - PartnerID is NULL so set default partner :%s\n", __FUNCTION__, PartnerID );		
	}
  }

   APPLY_PRINT("%s - PartnerID :%s\n", __FUNCTION__, PartnerID );

   if ( access(PARTNERS_INFO_FILE, F_OK ) != 0 )	
   {
		snprintf(cmd, sizeof(cmd), "cp %s %s", "/etc/partners_defaults.json", PARTNERS_INFO_FILE);
		APPLY_PRINT("%s\n",cmd);
		system(cmd);

		//Need to touch /tmp/.apply_partner_defaults_psm for PSM migration handling
		system("touch "PARTNER_DEFAULT_MIGRATE_PSM); // FIX: RDKB-20566 to handle migration 		
   }
   else
   {
	   ptr_etc_json = json_file_parse( "/etc/partners_defaults.json" );

	   if ( ptr_etc_json ) 
	   {
		  //Check whether file is having content or not	 
		  ptr_nvram_json = json_file_parse( PARTNERS_INFO_FILE );

		  //Fallback case when file is empty
		  if( NULL == ptr_nvram_json )
	  	  {
			//Copy /etc/partners_defaults.json file to nvram
			APPLY_PRINT("%s - %s file is empty\n", __FUNCTION__, PARTNERS_INFO_FILE );
			snprintf( cmd, sizeof( cmd ), "rm -rf %s; cp %s %s", PARTNERS_INFO_FILE, "/etc/partners_defaults.json", PARTNERS_INFO_FILE );
			APPLY_PRINT( "%s\n",cmd );
			system( cmd );
	  	  }
		  
		  //Check again whether file is having content or not	
		  ptr_nvram_json = json_file_parse( PARTNERS_INFO_FILE );
	   	  if ( NULL != ptr_nvram_json )
	   	  {
	   		compare_partner_json_param( ptr_nvram_json, ptr_etc_json, PartnerID );

			if( NULL != ptr_nvram_json )
			free( ptr_nvram_json );		
	   	  }	   	

			if( NULL != ptr_etc_json )
			free( ptr_etc_json );
	   }
   }

   //Apply partner default values during FR/partner FR case
   db_val = json_file_parse( PARTNERS_INFO_FILE );

   if( db_val )
   {
		apply_partnerId_default_values( db_val ,PartnerID );

		if( NULL != db_val )
		free( db_val );
   }

   return(0);
}

