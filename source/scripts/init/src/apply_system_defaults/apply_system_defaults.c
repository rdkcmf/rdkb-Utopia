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
#include "time.h"
#if defined (_XB6_PRODUCT_REQ_) || defined(_HUB4_PRODUCT_REQ_) || defined(_SR300_PRODUCT_REQ_)
#include "platform_hal.h"
#endif
#include "cJSON.h"
#include <unistd.h>
#include <stdbool.h>
#define PARTNERS_INFO_FILE  							"/nvram/partners_defaults.json"
#define PARTNERS_INFO_FILE_ETC                                                 "/etc/partners_defaults.json"
#define BOOTSTRAP_INFO_FILE                                                    "/nvram/bootstrap.json"
#define VERSION_TXT_FILE							"/version.txt"
#define PARTNERID_FILE  								"/nvram/.partner_ID"
#define PARTNER_DEFAULT_APPLY_FILE  					"/nvram/.apply_partner_defaults"
#define PARTNER_DEFAULT_MIGRATE_PSM  					"/tmp/.apply_partner_defaults_psm"
#define PARTNER_DEFAULT_MIGRATE_FOR_NEW_PSM_MEMBER  	"/tmp/.apply_partner_defaults_new_psm_member"

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

#define RETRY_COUNT 60
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
            printf("[utopia] [error] set_syscfg_defaults failed to set syscfg (%s)\n", line);
         } else { 
            set_syscfg(trim(name), trim(value));
         }
      } else if ('@' == line[0]) {
         // this is a sysevent line
      } else {
         // this is a malformed line
         printf("[utopia] set_syscfg_defaults found a malformed line (%s)\n", line);
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
            printf("[utopia] set_sysevent_defaults failed to set sysevent (%s)\n", line);
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
         printf("[utopia] set_sysevent_defaults found a malformed line (%s)\n", line);
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
	FILE	 	*fileRead 	= NULL;
	char		*data 		= NULL;
	int 		 len 		= 0;

	//File read
	fileRead = fopen( path, "r" );
  
	//Null Check
	if( fileRead == NULL ) 
	{
	 	APPLY_PRINT( "%s-%d : Error in opening %s JSON file\n" , __FUNCTION__, __LINE__,path );
		return NULL;
	}

	//Calculate length for memory allocation 
	fseek( fileRead, 0, SEEK_END );
	len = ftell( fileRead );
	fseek( fileRead, 0, SEEK_SET );

	APPLY_PRINT("%s-%d : Total File Length :%d \n", __FUNCTION__, __LINE__, len );

	if( len > 0 )
 	{
		 data = ( char* )malloc( sizeof(char) * (len + 1) );
		 //Check memory availability
		 if ( data != NULL ) 
		 {
			memset( data, 0, ( sizeof(char) * (len + 1) ));
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

static int writeToJson(char *data, char *file)
{
    FILE 	*fp;
    fp = fopen(file, "w");
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
	memset( buf, 0, sizeof( buf ));
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
        	APPLY_PRINT("set_syscfg_partner_values : syscfg_set failed\n");
		return 1;
	}
	else 
	{
       	 	if (syscfg_commit() != 0) 
		{
			APPLY_PRINT("set_syscfg_partner_values : syscfg_commit failed\n");
			return 1;
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
#if defined (_XB6_PRODUCT_REQ_) || defined(_HUB4_PRODUCT_REQ_) || defined(_SR300_PRODUCT_REQ_)
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
		//TCCBR-4426 getFactoryPartnerId is implemented for XB6/HUB4 Products as of now
		APPLY_PRINT("%s - Failed Get factoryPartnerId \n", __FUNCTION__);
	}
#endif
	return -1;
}

int validatePartnerId ( char *PartnerID )
{

   char* ptr_etc_jsons = NULL;
   cJSON * subitem_etc = NULL;
   ptr_etc_jsons = json_file_parse( PARTNERS_INFO_FILE_ETC );
   if(ptr_etc_jsons)
   {
      cJSON * root_etc_json = cJSON_Parse(ptr_etc_jsons);
      subitem_etc = cJSON_GetObjectItem(root_etc_json,PartnerID);
      if(subitem_etc)
      {
      	printf("##############Partner ID Found\n");
        return 1;
      }
      else
      {
      	printf("Partner ID NOT Found\n");
      	sprintf(PartnerID,"%s","unknown");
        return 0;
      }
   }
   return 0;
}

static int get_PartnerID( char *PartnerID)
{
	char buf[PARTNER_ID_LEN];
	memset(buf, 0, sizeof(buf));
	//int isValidPartner = 0;

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
                                APPLY_PRINT("%s:ERROR.....partnerId from factory also NULL setting it to unknown\n",__FUNCTION__);
				
#if defined (_XB6_PRODUCT_REQ_)
				sprintf( PartnerID, "%s", "unknown" );
#elif defined (_RDK_REF_PLATFORM_)
                                sprintf( PartnerID, "%s", "RDKM");
#elif defined (_SR300_PRODUCT_REQ_) /* Default fall back option for ADA devices SKYH4-4946 */
				sprintf( PartnerID, "%s", "sky-uk");
#elif defined (_HUB4_PRODUCT_REQ_) /* Default fall back option for HUB4 devices SKYH4-4946 */
			        sprintf( PartnerID, "%s", "sky-italia");
#else
				sprintf( PartnerID, "%s", "comcast" );
#endif
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
	
	return 0;	
}

void ValidateAndUpdatePartnerVersionParam(cJSON *root_etc_json,cJSON *root_nvram_json, bool *do_compare)
{
    cJSON *properties_etc = NULL;
    cJSON *properties_nvram = NULL;
    char *version_etc = NULL;
    char *version_nvram = NULL;
    cJSON *version_nvram_key = NULL;
  
    if (!do_compare || !root_etc_json || !root_nvram_json)
        return;

    /* Check if entire parameters need to be compared based on version number
    */
    properties_etc = cJSON_GetObjectItem(root_etc_json,"properties");
  
    if (!properties_etc)
        *do_compare = true;
  
    if (properties_etc)
    {
        properties_nvram = cJSON_GetObjectItem(root_nvram_json,"properties");
        version_etc = cJSON_GetObjectItem(properties_etc,"version")->valuestring;
        if (properties_nvram)
        {
            int etc_major = 0;
            int etc_minor = 0;
            int nvram_major = 0;
            int nvram_minor = 0;

            if (version_etc)
            {        
                sscanf(version_etc,"%d.%d",&etc_major,&etc_minor);
                printf ("\n READ version ######## etc: major %d minor %d \n",etc_major, etc_minor);
            }
            /* Check if version exists inside properties object */
            version_nvram_key = (cJSON *)cJSON_GetObjectItem(properties_nvram,"version");
            if(version_nvram_key)
            {
                version_nvram = cJSON_GetObjectItem(properties_nvram,"version")->valuestring;
            }
            else
            {
                 printf("version key is missing in properties\n");
                 *do_compare = true;
                 /* version is missing, so delete entire properties */
                 cJSON_DeleteItemFromObject(root_nvram_json,"properties");
            }
            /* If version exists in both nvram and etc, then compare the version number */
            if ( version_nvram && version_etc)
            {
                sscanf(version_nvram,"%d.%d",&nvram_major,&nvram_minor);
                printf ("\n READ version ######## nvram: major %d minor %d\n",nvram_major, nvram_minor);
                if (nvram_major < etc_major)
                {
                    *do_compare = true;
                }
                else if (nvram_major == etc_major)
                {
                    if (nvram_minor < etc_minor)
                    {
                        *do_compare = true;
                    }
                }

                if (*do_compare)
                {
                    printf ("\n VERSION MISMATCH ######## nvram %s etc %s \n", version_nvram, version_etc);
                }
            }                   
        }
        else
        {
            *do_compare = true;
        }
    }    

    if (version_etc)
    {
        /* If version doesn't exist in nvram file, then insert it at the 0th index 
           else just replace the object 
        */ 
        if (!version_nvram)
        {
            printf ("\n NVRAM VERSION not found  ######## etc %s \n", version_etc);
            cJSON_InsertItemInArray(root_nvram_json,0,cJSON_GetObjectItem(root_etc_json,"properties") );
        }
        else
        {
            printf("\n NVRAM VERSION FOUND\n");
            cJSON_ReplaceItemInArray(root_nvram_json,0,cJSON_GetObjectItem(root_etc_json,"properties") );
        }
        char *out = cJSON_Print(root_nvram_json);
        writeToJson(out, BOOTSTRAP_INFO_FILE);
        out = NULL;
    }
}

char * getBuildTime()
{
    static char buildTime[50] = {0};
    if (buildTime[0] != '\0')
        return buildTime;
    
    FILE *fptr;
    if ((fptr = fopen(VERSION_TXT_FILE, "r")) == NULL)
    {
        printf( "%s: Trying to open a non-existent file [%s] \n", __FUNCTION__, VERSION_TXT_FILE);
    }
    else
    {
        char * line = NULL;
        size_t len = 0;
        int read;

        while ((read = getline(&line, &len, fptr)) != -1)
        {
            if (strstr(line, "BUILD_TIME"))
            {
                char *substr = strtok(line, "\"");
                if (substr)
                {
                   substr = strtok(NULL,"\"");
                   if (substr)
                   {
                      strncpy(buildTime, substr, sizeof(buildTime));
                   }
                }
                break;
            }
        }
        if (line)
           free(line);

        fclose(fptr);
    }
    return buildTime;
}

char * getTime()
{
    time_t timer;
    static char buffer[50];
    struct tm* tm_info;
    time(&timer);
    tm_info = localtime(&timer);
    strftime(buffer, 50, "%Y-%m-%d %H:%M:%S ", tm_info);
    return buffer;
}

int addParamInPartnersFile(char* pKey, char* PartnerId, char* pValue)
{
	cJSON *partnerObj = NULL;
	cJSON *json = NULL;
	FILE *fileRead = NULL;
	char * cJsonOut = NULL;
	char* data = NULL;
	 int len ;
	 int configUpdateStatus = -1;
	 fileRead = fopen( PARTNERS_INFO_FILE, "r" );
	 if( fileRead == NULL ) 
	 {
		 APPLY_PRINT("%s-%d : Error in opening JSON file\n" , __FUNCTION__, __LINE__ );
		 return -1;
	 }
	 
	 fseek( fileRead, 0, SEEK_END );
	 len = ftell( fileRead );
	 fseek( fileRead, 0, SEEK_SET );
	 data = ( char* )malloc( sizeof(char) * (len + 1) );
	 if (data != NULL) 
	 {
		memset( data, 0, ( sizeof(char) * (len + 1) ));
	 	fread( data, 1, len, fileRead );
	 }
	 else 
	 {
		 APPLY_PRINT("%s-%d : Memory allocation failed \n", __FUNCTION__, __LINE__);
		 fclose( fileRead );
		 return -1;
	 }
	 
	 fclose( fileRead );
	 if ( data == NULL )
	 {
		APPLY_PRINT("%s-%d : fileRead failed \n", __FUNCTION__, __LINE__);
		return -1;
	 }
	 else if ( strlen(data) != 0)
	 {
		 json = cJSON_Parse( data );
		 if( !json ) 
		 {
			 APPLY_PRINT(  "%s : json file parser error : [%d]\n", __FUNCTION__,__LINE__);
			 free(data);
			 return -1;
		 } 
		 else
		 {
			 partnerObj = cJSON_GetObjectItem( json, PartnerId );
			 if ( NULL != partnerObj)
			 {
				 if (NULL == cJSON_GetObjectItem( partnerObj, pKey) )
				 {
					cJSON_AddItemToObject(partnerObj, pKey, cJSON_CreateString(pValue));
				 }
				 else
				 {
					 cJSON_ReplaceItemInObject(partnerObj, pKey, cJSON_CreateString(pValue));
				 }
				 cJsonOut = cJSON_Print(json);
				 configUpdateStatus = writeToJson(cJsonOut, PARTNERS_INFO_FILE);
				 if ( !configUpdateStatus)
				 {
					 APPLY_PRINT( "Added/Updated Value for %s partner\n",PartnerId);
					 APPLY_PRINT( "Param:%s - Value:%s\n",pKey,pValue);
				 }
				 else
			 	 {
					 APPLY_PRINT( "Failed to update value for %s partner\n",PartnerId);
					 APPLY_PRINT( "Param:%s\n",pKey);
		 			 cJSON_Delete(json);
					 return -1;
			 	 }
			 }
			 else
			 {
			 	APPLY_PRINT("%s - PARTNER ID OBJECT Value is NULL\n", __FUNCTION__ );
			 	cJSON_Delete(json);
			 	return -1;
			 }
			cJSON_Delete(json);
		 }
	  }
	  else
	  {
		APPLY_PRINT("PARTNERS_INFO_FILE %s is empty\n", PARTNERS_INFO_FILE);
		return -1;
	  }
	 return 0;
}

void addInSysCfgdDB(char * key, char * value)
{
   /* There are parameters which needs to be available in syscfg/PSM DBs
      Check if all of these parameters are SET into DBs
   */
   int IsPSMMigrationNeeded = 0;
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
   if ( 0 == strcmp ( key, "Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.StartupIPMode") )
   {
      if ( 0 == IsValuePresentinSyscfgDB( "StartupIPMode" ) )
      {
         set_syscfg_partner_values( value,"StartupIPMode" );
      }
   }
   if ( 0 == strcmp ( key, "Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.IPv4PrimaryDhcpServerOptions") )
   {
      if ( 0 == IsValuePresentinSyscfgDB( "IPv4PrimaryDhcpServerOptions" ) )
      {
         set_syscfg_partner_values( value,"IPv4PrimaryDhcpServerOptions" );
      }
   }
   if ( 0 == strcmp ( key, "Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.IPv4SecondaryDhcpServerOptions") )
   {
      if ( 0 == IsValuePresentinSyscfgDB( "IPv4SecondaryDhcpServerOptions" ) )
      {
         set_syscfg_partner_values( value,"IPv4SecondaryDhcpServerOptions" );
      }
   }
   if ( 0 == strcmp ( key, "Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.IPv6PrimaryDhcpServerOptions") )
   {
      if ( 0 == IsValuePresentinSyscfgDB( "IPv6PrimaryDhcpServerOptions" ) )
      {
         set_syscfg_partner_values( value,"IPv6PrimaryDhcpServerOptions" );
      }
   }
   if ( 0 == strcmp ( key, "Device.X_RDK_WebConfig.URL") )
   {
      if ( 0 == IsValuePresentinSyscfgDB( "WEBCONFIG_INIT_URL" ) )
      {
         set_syscfg_partner_values( value,"WEBCONFIG_INIT_URL" );
         IsPSMMigrationNeeded = 1;
      }
   }
   if ( 0 == strcmp ( key, "Device.X_RDK_WebConfig.SupplementaryServiceUrls.Telemetry") )
   {
      if ( 0 == IsValuePresentinSyscfgDB( "TELEMETRY_INIT_URL" ) )
      {
         set_syscfg_partner_values( value,"TELEMETRY_INIT_URL" );
         IsPSMMigrationNeeded = 1;
      }
   }
   if ( 0 == strcmp ( key, "Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.IPv6SecondaryDhcpServerOptions") )
   {
      if ( 0 == IsValuePresentinSyscfgDB( "IPv6SecondaryDhcpServerOptions" ) )
      {
         set_syscfg_partner_values( value,"IPv6SecondaryDhcpServerOptions" );
      }
   }
   if ( 0 == strcmp ( key, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.HomeSec.SSIDprefix") )
   {
      set_syscfg_partner_values( value,"XHS_SSIDprefix" );
      IsPSMMigrationNeeded = 1;
   }
   #ifdef MTA_TR104SUPPORT
   if ( 0 == strcmp ( key, "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.TR104.Enable") )
   {
       if ( 0 == IsValuePresentinSyscfgDB( "TR104Enable" ) )
       {
           set_syscfg_partner_values( value,"TR104Enable" );
       }
   }
   #else
       APPLY_PRINT("TR104 is not supported so making TR104 value as false\n");
       set_syscfg_partner_values( "false","TR104Enable" );
   #endif

   //Check whether migration needs to be handled or not
   if( 1 == IsPSMMigrationNeeded )
   {
      APPLY_PRINT("%s - Adding new member in %s file\n", __FUNCTION__, BOOTSTRAP_INFO_FILE);
      APPLY_PRINT("%s - PSM Migration needed for %s param so touching %s file\n", __FUNCTION__, key, PARTNER_DEFAULT_MIGRATE_FOR_NEW_PSM_MEMBER );

      //Need to touch /tmp/.apply_partner_defaults_new_psm_member for PSM migration handling
      system("touch "PARTNER_DEFAULT_MIGRATE_FOR_NEW_PSM_MEMBER);
   }
}

void updateSysCfgdDB(char * key, char * value)
{
   /* There are parameters which needs to be available in syscfg/PSM DBs
      Check if all of these parameters are SET into DBs
   */
   int IsPSMMigrationNeeded = 0;
   if ( 0 == strcmp ( key, "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.SyndicationFlowControl.InitialForwardedMark") )
   {
         set_syscfg_partner_values( value,"DSCP_InitialForwardedMark" );
   }
   if ( 0 == strcmp ( key, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.WANsideSSH.Enable") )
   {
         set_syscfg_partner_values( value,"WANsideSSH_Enable" );
   }
   if ( 0 == strcmp ( key, "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.SyndicationFlowControl.InitialOutputMark") )
   {
         set_syscfg_partner_values( value,"DSCP_InitialOutputMark" );
   }
   if ( 0 == strcmp ( key, "Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.StartupIPMode") )
   {
         set_syscfg_partner_values( value,"StartupIPMode" );
   }
   if ( 0 == strcmp ( key, "Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.IPv4PrimaryDhcpServerOptions") )
   {
         set_syscfg_partner_values( value,"IPv4PrimaryDhcpServerOptions" );
   }
   if ( 0 == strcmp ( key, "Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.IPv4SecondaryDhcpServerOptions") )
   {
         set_syscfg_partner_values( value,"IPv4SecondaryDhcpServerOptions" );
   }
   if ( 0 == strcmp ( key, "Device.X_RDK_WebConfig.URL") )
   {
         set_syscfg_partner_values( value,"WebConfig_url" );
         IsPSMMigrationNeeded = 1;
   }
   if ( 0 == strcmp ( key, "Device.X_RDK_WebConfig.SupplementaryServiceUrls.Telemetry") )
   {
         set_syscfg_partner_values( value,"telemetry_url" );
         IsPSMMigrationNeeded = 1;
   }
   if ( 0 == strcmp ( key, "Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.IPv6PrimaryDhcpServerOptions") )
   {
         set_syscfg_partner_values( value,"IPv6PrimaryDhcpServerOptions" );
   }
   if ( 0 == strcmp ( key, "Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.IPv6SecondaryDhcpServerOptions") )
   {
         set_syscfg_partner_values( value,"IPv6SecondaryDhcpServerOptions" );
   }
   if ( 0 == strcmp ( key, "Device.ManagementServer.EnableCWMP") )
   {
         set_syscfg_partner_values( value,"Syndication_EnableCWMP" );
   }
   if ( 0 == strcmp ( key, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.HomeSec.SSIDprefix") )
   {
      set_syscfg_partner_values( value,"XHS_SSIDprefix" );
      IsPSMMigrationNeeded = 1;
   }
   if ( 0 == strcmp ( key, "Device.WiFi.X_RDKCENTRAL-COM_Syndication.WiFiRegion.Code") )
   {
      set_syscfg_partner_values( value,"WiFiRegionCode" );
      IsPSMMigrationNeeded = 1;
   }
   if ( 0 == strcmp ( key, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.TR69CertLocation") )
   {
      set_syscfg_partner_values( value,"TR69CertLocation" );
      IsPSMMigrationNeeded = 1;
   }
   if ( 0 == strcmp ( key, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.RDKB_UIBranding.AllowEthernetWAN") )
   {
         set_syscfg_partner_values( value,"AllowEthernetWAN" );
   }
#ifdef MTA_TR104SUPPORT
      if ( 0 == strcmp ( key, "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.TR104.Enable" ) )
      {
          set_syscfg_partner_values( value, "TR104Enable");
      }
#else
      if ( 0 == strcmp ( key, "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.TR104.Enable" ) )
      {
          APPLY_PRINT("TR104 is not supported so making TR104 value as false\n");
          set_syscfg_partner_values( "false", "TR104Enable");
      }
#endif
   //Check whether migration needs to be handled or not
   if( 1 == IsPSMMigrationNeeded )
   {
      APPLY_PRINT("%s - Updating member in %s file\n", __FUNCTION__, BOOTSTRAP_INFO_FILE);
      APPLY_PRINT("%s - PSM Migration needed for %s param so touching %s file\n", __FUNCTION__, key, PARTNER_DEFAULT_MIGRATE_FOR_NEW_PSM_MEMBER );

      //Need to touch /tmp/.apply_partner_defaults_new_psm_member for PSM migration handling
      system("touch "PARTNER_DEFAULT_MIGRATE_FOR_NEW_PSM_MEMBER);
   }
}

// This function can be removed after a few release cycles
int init_bootstrap_json(char * partner_nvram_obj, char *partner_etc_obj, char *PartnerID)
{
   APPLY_PRINT("%s\n", __FUNCTION__);

   cJSON * root_nvram_json = cJSON_Parse(partner_nvram_obj);
   cJSON * root_etc_json = cJSON_Parse(partner_etc_obj);

   cJSON *root_nvram_bs_json;
   root_nvram_bs_json = cJSON_CreateObject();

   cJSON *prop = cJSON_GetObjectItem(root_etc_json,"properties");
   cJSON_AddItemReferenceToObject(root_nvram_bs_json, "properties", prop);

   cJSON * subitem_etc = cJSON_GetObjectItem(root_etc_json,PartnerID);
   cJSON * subitem_nvram = cJSON_GetObjectItem(root_nvram_json,PartnerID);

   if( subitem_etc )
   {
      cJSON *param = subitem_etc->child;
      cJSON *newPartnerObj = cJSON_CreateObject();
      while( param )
      {
         char *key = NULL, *value = NULL, *value_nvram = NULL;
         cJSON *newParamObj = cJSON_CreateObject();
         key = param->string;
         cJSON * value_obj = cJSON_GetObjectItem(subitem_etc, key);
         if (value_obj)
            value = value_obj->valuestring;

         if (value == NULL)
         {
            APPLY_PRINT("etc value is NULL for key = %s, skip it...\n", key);
            param = param->next;
            continue;
         }

         cJSON_AddStringToObject(newParamObj, "DefaultValue", value);
         cJSON_AddStringToObject(newParamObj, "BuildTime", getBuildTime());

         if ( subitem_nvram )
         {
            cJSON * value_nvram_obj = cJSON_GetObjectItem(subitem_nvram, key);
            if (value_nvram_obj)
               value_nvram = value_nvram_obj->valuestring;

            if ( value_nvram && strcmp(value, value_nvram) != 0)
            {
               APPLY_PRINT("nvram value = %s\n", value_nvram);
               cJSON_AddStringToObject(newParamObj, "ActiveValue", value_nvram);
               cJSON_AddStringToObject(newParamObj, "UpdateTime", "unknown");
               cJSON_AddStringToObject(newParamObj, "UpdateSource", "webpa"); //Assuming as webpa since we don't know who actually updated
            }
            else
            {
               cJSON_AddStringToObject(newParamObj, "ActiveValue", value);
               cJSON_AddStringToObject(newParamObj, "UpdateTime", "-");
               cJSON_AddStringToObject(newParamObj, "UpdateSource", "-");
            }
         }
         else
         {
            cJSON_AddStringToObject(newParamObj, "ActiveValue", value);
            cJSON_AddStringToObject(newParamObj, "UpdateTime", "-");
            cJSON_AddStringToObject(newParamObj, "UpdateSource", "-");
         }
         cJSON_AddItemToObject(newPartnerObj, key, newParamObj);
         addInSysCfgdDB(key, value);
         param = param->next;
      }
      cJSON_AddItemToObject(root_nvram_bs_json, PartnerID, newPartnerObj);

      char *out = cJSON_Print(root_nvram_bs_json);
      //APPLY_PRINT("out1 = %s\n", out);
      writeToJson(out, BOOTSTRAP_INFO_FILE);
      out = NULL;
   }

   cJSON_Delete(root_nvram_json);
   cJSON_Delete(root_etc_json);

   return 0;
}

int compare_partner_json_param(char *partner_nvram_bs_obj,char *partner_etc_obj,char *PartnerID)
{
   APPLY_PRINT("%s\n", __FUNCTION__);

   cJSON * root_nvram_bs_json = cJSON_Parse(partner_nvram_bs_obj);

   /* The below block of code identifies any unknown/wrong objects in nvram/bootstrap.json and removes them */
   if (!root_nvram_bs_json)
   {
      APPLY_PRINT("json parse error for bootstrap.json\n");
      char  cmd[256] = {0};
      snprintf(cmd, sizeof(cmd), "rm %s", BOOTSTRAP_INFO_FILE);
      APPLY_PRINT("%s\n",cmd);
      system(cmd);
      char *ptr_nvram_json = json_file_parse( PARTNERS_INFO_FILE );
      init_bootstrap_json( ptr_nvram_json, partner_etc_obj, PartnerID );
      free(ptr_nvram_json);
      return -1;
   }

   cJSON *root_nvram_bs_json_copy=cJSON_Parse(partner_nvram_bs_obj);
   cJSON *current_element = NULL;
   bool jsonChanged = false;
   cJSON_ArrayForEach(current_element, root_nvram_bs_json_copy)
   {
      char *current_key = current_element->string;
      /* if the key is not properties and not partnerID, then remove that object */
      if (strcmp(current_key, "properties") && strcmp(current_key, PartnerID))
      {
         jsonChanged = true;
         cJSON_DeleteItemFromObject(root_nvram_bs_json, current_key);
         printf("Remove unknown object: %s\n", current_key);
      }
   }
   if (jsonChanged)
   {
      char *out = cJSON_Print(root_nvram_bs_json);
      writeToJson(out, BOOTSTRAP_INFO_FILE);
      out = NULL;
   }
   /* The above code block can be removed in future when we ae sure there will be no unknown objects in bootstrap.json */

   /* Check if version exists or has changed.
      If the function returns "true" proceed for comparison
      else there is no change. Just return
   */
   bool do_compare = false;
   cJSON * root_etc_json = cJSON_Parse(partner_etc_obj);
   ValidateAndUpdatePartnerVersionParam(root_etc_json, root_nvram_bs_json, &do_compare);
   if (!do_compare && !jsonChanged)
      return -1;
   printf("versions are different...\n");
   char* ptr_nvram_bs_json = NULL;
   ptr_nvram_bs_json = json_file_parse( BOOTSTRAP_INFO_FILE );
   root_nvram_bs_json=cJSON_Parse(ptr_nvram_bs_json);
   root_etc_json = cJSON_Parse(partner_etc_obj);
   cJSON * subitem_etc = cJSON_GetObjectItem(root_etc_json,PartnerID);
   cJSON * subitem_nvram_bs = cJSON_GetObjectItem(root_nvram_bs_json,PartnerID);
   char *key=NULL, *value=NULL;
   if( subitem_etc )
   {
      cJSON *param = subitem_etc->child;
      while( param )
      {
         key = param->string;
         cJSON * value_obj = cJSON_GetObjectItem(subitem_etc, key);
         if (value_obj)
            value = value_obj->valuestring;
         else
            return -1;

         APPLY_PRINT("key = %s value = %s\n", key, value);

         cJSON *bs_obj = cJSON_GetObjectItem(subitem_nvram_bs, key);
         if (bs_obj == NULL)
         {
            APPLY_PRINT("param %s does not exist in nvram bootstrap json. Adding it...\n", key);
            cJSON *newParamObj = cJSON_CreateObject();
            cJSON_AddStringToObject(newParamObj, "DefaultValue", value);
            cJSON_AddStringToObject(newParamObj, "BuildTime", getBuildTime());
            cJSON_AddStringToObject(newParamObj, "ActiveValue", value);
            cJSON_AddStringToObject(newParamObj, "UpdateTime", "-");
            cJSON_AddStringToObject(newParamObj, "UpdateSource", "-");
            cJSON_AddItemToObject(subitem_nvram_bs, key, newParamObj);

            addInSysCfgdDB(key, value);

            //Also add in the /nvram/partners_defaults.json
            addParamInPartnersFile(key, PartnerID, value);
         }
         else
         {
            cJSON * value_bs_obj = cJSON_GetObjectItem(bs_obj, "ActiveValue");
            char * value_bs = NULL;
            if (value_bs_obj)
               value_bs = value_bs_obj->valuestring;
            else
               return -1;

            cJSON * source_bs_obj = cJSON_GetObjectItem(bs_obj, "UpdateSource");
            char * source_bs = NULL;
            if (source_bs_obj)
               source_bs = source_bs_obj->valuestring;
            else
               return -1;

            //printf("value_bs = %s, source_bs = %s\n", value_bs, source_bs);
            if (strcmp(value, value_bs))
            {
               APPLY_PRINT("** Param %s value changed in firmware **\n", key);
               cJSON_ReplaceItemInObject(bs_obj,"DefaultValue", cJSON_CreateString(value));
               cJSON_ReplaceItemInObject(bs_obj,"BuildTime", cJSON_CreateString(getBuildTime()));
               if (!strcmp(source_bs,"-"))
               {
                  APPLY_PRINT(" ** Param was not overridden previously. Update the active value..\n");
                  cJSON_ReplaceItemInObject(bs_obj,"ActiveValue", cJSON_CreateString(value));
                  updateSysCfgdDB(key, value);
               }
            }
         }
         param = param->next;
      }

      /* Check if nvram file has same count as etc file
         if nvram has more entries, we may need to check what was
         removed from etc in current release.
      */
      int subitem_etc_count = cJSON_GetArraySize(subitem_etc);
      int subitem_nvram_bs_count = cJSON_GetArraySize(subitem_nvram_bs);
      int iCount = 0;
      if ( subitem_etc_count < subitem_nvram_bs_count)
      {
         for (iCount=0;iCount<subitem_nvram_bs_count;iCount++)
         {
            key=cJSON_GetArrayItem(subitem_nvram_bs,iCount)->string;
            //printf("String key is : %s\n",key);
            cJSON * etc_key=cJSON_GetObjectItem(subitem_etc,key);
            if(etc_key == NULL)
            {
               APPLY_PRINT("Delete parameter %s from /nvram/bootstrap.json\n", key);
               //key=cJSON_GetArrayItem(subitem_nvram_bs,iCount);
               cJSON_DeleteItemFromArray(subitem_nvram_bs,iCount);
               //Decrement the count when an element is deleted
               subitem_nvram_bs_count --;
            }
         }//for loop
      }

      char *out = cJSON_Print(root_nvram_bs_json);
      //printf("compare out = %s\n", out);
      writeToJson(out, BOOTSTRAP_INFO_FILE);
      out = NULL;
   }

   cJSON_Delete(root_nvram_bs_json);
   cJSON_Delete(root_etc_json);

   return 0;
}

#ifdef RETRY_COUNT
#undef RETRY_COUNT
#endif
#define RETRY_COUNT 3

int apply_partnerId_default_values(char *data, char *PartnerID)
{
	cJSON 	*partnerObj 	= NULL;
	cJSON 	*json 			= NULL;
	cJSON	*paramObjVal	= NULL;
	char 	*userName 		= NULL, 
		    *defaultAdminIP = NULL,	 
		    *passWord 		= NULL,	 
		    *subnetRange 	= NULL,
	*minAddress = NULL,
	*maxAddress = NULL,
        *allow_ethernet_wan = NULL,
        *initialForwardedMark = NULL,
        *initialOutputMark = NULL,
        *startupipmode = NULL,
        *pridhcpoption = NULL,
        *secdhcpoption = NULL;
    int	    isNeedToApplyPartnersDefault = 1;
    int	    isNeedToApplyPartnersPSMDefault = 0;
    char    ntpServer1[64]     = {0};
    char    *jsonNTPServer1    = NULL;
    cJSON   *alwaysPartnerObj  = NULL;
    cJSON   *alwaysJson        = NULL;
    cJSON   *alwaysParamObjVal = NULL;
    char    *error_ptr         = NULL;
    int     iterator           = 0;

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
   		APPLY_PRINT("%s - Deletion of %s file handled in PSM init \n", __FUNCTION__, PARTNER_DEFAULT_APPLY_FILE );
		//Delete at PSM init
		//system( "rm -rf /nvram/.apply_partner_defaults" );
  	}

	if ( access( PARTNER_DEFAULT_MIGRATE_PSM , F_OK ) == 0 )  
	{
		isNeedToApplyPartnersPSMDefault = 1;

		APPLY_PRINT("%s - %s file available so need to do partner's PSM member migration \n", __FUNCTION__, PARTNER_DEFAULT_MIGRATE_PSM );
		APPLY_PRINT("%s - Deletion of %s file handled in PSM init \n", __FUNCTION__, PARTNER_DEFAULT_MIGRATE_PSM );
	}

	if( ( 1 == isNeedToApplyPartnersDefault ) || \
		( 1 == isNeedToApplyPartnersPSMDefault ) 
	  )
	{
          	APPLY_PRINT("%s - Applying  %s default configuration\n", __FUNCTION__, PartnerID );
		json = cJSON_Parse( data );
		if( !json ) 
		{
			APPLY_PRINT(  "%s-%d : json file parser error\n", __FUNCTION__,__LINE__ );
			return -1;
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
					paramObjVal = cJSON_GetObjectItem(cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.RDKB_UIBranding.LocalUI.DefaultLoginUsername"), "ActiveValue");
					if ( paramObjVal != NULL )
					{
						userName = paramObjVal->valuestring; 
					
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

					paramObjVal = cJSON_GetObjectItem(cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.RDKB_UIBranding.LocalUI.DefaultLoginPassword"), "ActiveValue");
                                        if ( paramObjVal != NULL )
					{
						passWord = paramObjVal->valuestring;

                                                if (strstr( PartnerID, "sky-" ) != NULL)
                                                {
                                                    APPLY_PRINT("%s - Fetching %s password from serial data \n",__FUNCTION__, PartnerID);
                                                    // For Sky, we need to pull the default login from the /tmp/serial.txt file.
                                                    FILE *fp = NULL;
                                                    char DefaultPassword[25] = {0};

                                                    fp = popen("grep 'WIFIPASSWORD' /tmp/serial.txt | cut -d '=' -f 2 | tr -d [:space:]", "r");
                                                    if (fp == NULL)
                                                    {
                                                        APPLY_PRINT("%s - ERROR Grabbing the default password\n",__FUNCTION__);
                                                    } else {
                                                                fgets(DefaultPassword, sizeof(DefaultPassword), fp);
                                                                pclose(fp);
                                                    }
 
                                                    if (DefaultPassword[0] != '\0')
                                                        {
                                                                set_syscfg_partner_values(DefaultPassword,"user_password_3");
                                                        }
                                                        else
                                                        {
                                                                APPLY_PRINT("%s - DefaultLoginPassword Value is NULL\n", __FUNCTION__ );
                                                        }
                                                }
					
						else if (passWord != NULL) 
						{
							set_syscfg_partner_values(passWord,"user_password_3");
							passWord = NULL;
						}	
						else
						{
							APPLY_PRINT("%s - DefaultLoginUsername Value is NULL\n", __FUNCTION__ );
						}	
					}

					paramObjVal = cJSON_GetObjectItem(cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.RDKB_UIBranding.DefaultAdminIP"), "ActiveValue");
                                        if ( paramObjVal != NULL )
                                        {
						defaultAdminIP = paramObjVal->valuestring; 
					
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

					paramObjVal = cJSON_GetObjectItem(cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.RDKB_UIBranding.DefaultLocalIPv4SubnetRange"), "ActiveValue");
                                        if ( paramObjVal != NULL )
                                        {
						subnetRange = paramObjVal->valuestring; 
					
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
                                        paramObjVal = cJSON_GetObjectItem(cJSON_GetObjectItem( partnerObj, "Device.DHCPv4.Server.Pool.1.MinAddress"), "ActiveValue");
                                        if ( paramObjVal != NULL )
                                        {
						minAddress = paramObjVal->valuestring;

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
                                        paramObjVal = cJSON_GetObjectItem(cJSON_GetObjectItem( partnerObj, "Device.DHCPv4.Server.Pool.1.MaxAddress"), "ActiveValue");
                                        if ( paramObjVal != NULL )
                                        {
                                                maxAddress = paramObjVal->valuestring;

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
					paramObjVal = cJSON_GetObjectItem(cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.RDKB_UIBranding.AllowEthernetWAN"), "ActiveValue");
					if ( paramObjVal != NULL )
                    {
                        allow_ethernet_wan = paramObjVal->valuestring;

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
					paramObjVal = cJSON_GetObjectItem(cJSON_GetObjectItem( partnerObj, "Device.WiFi.X_RDKCENTRAL-COM_Syndication.WiFiRegion.Code"), "ActiveValue");
                                        if ( paramObjVal != NULL )
                                        {
						char *pcWiFiRegionCode = NULL;
						
						pcWiFiRegionCode = paramObjVal->valuestring; 
			
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

					paramObjVal = cJSON_GetObjectItem(cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.TR69CertLocation"), "ActiveValue");
                                        if ( paramObjVal != NULL )
                                        {
							char *tr69CertLocation = NULL;
					
							tr69CertLocation = paramObjVal->valuestring;
					
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
					paramObjVal = cJSON_GetObjectItem(cJSON_GetObjectItem( partnerObj, "Device.X_RDK_WebConfig.URL"), "ActiveValue");
					if ( paramObjVal != NULL )
					{
						char *webconfigurl = NULL;
					 	webconfigurl = paramObjVal->valuestring;

						if (webconfigurl != NULL)
						{
							 set_syscfg_partner_values(webconfigurl,"WEBCONFIG_INIT_URL");
							webconfigurl = NULL;
						}
						else
						{
							APPLY_PRINT("%s - webconfigurl Value is NULL\n", __FUNCTION__ );
						}
					}

					paramObjVal = cJSON_GetObjectItem(cJSON_GetObjectItem( partnerObj, "Device.X_RDK_WebConfig.SupplementaryServiceUrls.Telemetry"), "ActiveValue");
					if ( paramObjVal != NULL )
					{
						char *telemetryurl = NULL;
						telemetryurl = paramObjVal->valuestring;

						if (telemetryurl != NULL)
						{
							set_syscfg_partner_values(telemetryurl,"TELEMETRY_INIT_URL");
							telemetryurl = NULL;
						}
						else
						{
							APPLY_PRINT("%s - telemetryurl Value is NULL\n", __FUNCTION__ );
						}
					}

					paramObjVal = cJSON_GetObjectItem(cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.HomeSec.SSIDprefix"), "ActiveValue");
                                        if ( paramObjVal != NULL )
                                        {
						char *pcSSIDprefix = NULL;
						
						pcSSIDprefix = paramObjVal->valuestring; 
			
						if (pcSSIDprefix != NULL) 
						{
							set_syscfg_partner_values(pcSSIDprefix,"XHS_SSIDprefix");
							pcSSIDprefix = NULL;
						}	
						else
						{
							APPLY_PRINT("%s - XHS_SSIDprefix Value is NULL\n", __FUNCTION__ );
						}	
					}
                                        paramObjVal = cJSON_GetObjectItem( cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.OAUTH.AuthMode" ), "ActiveValue" );
                                        if( paramObjVal != NULL )
                                        {
                                            char *pcAuthMode = NULL;
                    
                                            pcAuthMode = paramObjVal->valuestring;
                    
                                            if( pcAuthMode != NULL )
                                            {
                                                set_syscfg_partner_values( pcAuthMode, "OAUTHAuthMode" );
                                                pcAuthMode = NULL;
                                            }
                                            else
                                            {
                                                APPLY_PRINT( "%s - OAUTHAuthMode is NULL\n", __FUNCTION__ );
                                            }
                                        }
                                        paramObjVal = cJSON_GetObjectItem( cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.OAUTH.ServerUrl" ), "ActiveValue" );
                                        if( paramObjVal != NULL )
                                        {
                                            char *pcServerUrl = NULL;
                    
                                            pcServerUrl = paramObjVal->valuestring;
                    
                                            if( pcServerUrl != NULL )
                                            {
                                                set_syscfg_partner_values( pcServerUrl, "OAUTHServerUrl" );
                                                pcServerUrl = NULL;
                                            }
                                            else
                                            {
                                                APPLY_PRINT( "%s - OAUTHServerUrl is NULL\n", __FUNCTION__ );
                                            }
                                        }
                                        paramObjVal = cJSON_GetObjectItem( cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.OAUTH.TokenEndpoint" ), "ActiveValue" );
                                        if( paramObjVal != NULL )
                                        {
                                            char *pcTokenEndpoint = NULL;
                    
                                            pcTokenEndpoint = paramObjVal->valuestring;
                    
                                            if( pcTokenEndpoint != NULL )
                                            {
                                                set_syscfg_partner_values( pcTokenEndpoint, "OAUTHTokenEndpoint" );
                                                pcTokenEndpoint = NULL;
                                            }
                                            else
                                            {
                                                APPLY_PRINT( "%s - OAUTHTokenEndpoint is NULL\n", __FUNCTION__ );
                                            }
                                        }
                                        paramObjVal = cJSON_GetObjectItem( cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.OAUTH.ClientId" ), "ActiveValue" );
                                        if( paramObjVal != NULL )
                                        {
                                            char *pcClientId = NULL;
                    
                                            pcClientId = paramObjVal->valuestring;
                    
                                            if( pcClientId != NULL )
                                            {
                                                set_syscfg_partner_values( pcClientId, "OAUTHClientId" );
                                                pcClientId = NULL;
                                            }
                                            else
                                            {
                                                APPLY_PRINT( "%s - OAUTHClientId is NULL\n", __FUNCTION__ );
                                            }
                                        }
                                }

				if( 1 == isNeedToApplyPartnersDefault )
				{
					paramObjVal = cJSON_GetObjectItem(cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.SyndicationFlowControl.InitialForwardedMark"), "ActiveValue");
                                        if ( paramObjVal != NULL )
                                        {
					  initialForwardedMark = paramObjVal->valuestring; 
					  if (initialForwardedMark[0] != '\0')
					  {
						set_syscfg_partner_values(initialForwardedMark,"DSCP_InitialForwardedMark");
						initialForwardedMark = NULL;
					  }
					}
					else
					{
					  APPLY_PRINT("%s - Default Value of InitialForwardedMark is NULL\n", __FUNCTION__ );
					}

					paramObjVal = cJSON_GetObjectItem(cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.SyndicationFlowControl.InitialOutputMark"), "ActiveValue");
                                        if ( paramObjVal != NULL )
                                        {
					  initialOutputMark = paramObjVal->valuestring; 
					  if (initialOutputMark[0] != '\0')
					  {
						set_syscfg_partner_values(initialOutputMark,"DSCP_InitialOutputMark");
						initialOutputMark = NULL;
					  }
					}
					else
					{
					  APPLY_PRINT("%s - Default Value of InitialOutputMark is NULL\n", __FUNCTION__ );
					}

					paramObjVal = cJSON_GetObjectItem(cJSON_GetObjectItem( partnerObj, "Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.StartupIPMode"), "ActiveValue");
                                        if ( paramObjVal != NULL )
                                        {
					   startupipmode = paramObjVal->valuestring;
					   if(startupipmode[0]!='\0')
					   {
					            set_syscfg_partner_values(startupipmode,"StartupIPMode");
					            startupipmode = NULL;
					   }
					}
					else
				        {
				            APPLY_PRINT("%s - Default Value of StartupIPMode is NULL\n", __FUNCTION__ );
				        }

paramObjVal = cJSON_GetObjectItem(cJSON_GetObjectItem( partnerObj, "Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.IPv4PrimaryDhcpServerOptions"), "ActiveValue");
if ( paramObjVal != NULL )
{
   pridhcpoption = paramObjVal->valuestring;
   if(pridhcpoption[0]!='\0')
       {
            set_syscfg_partner_values(pridhcpoption,"IPv4PrimaryDhcpServerOptions");
            pridhcpoption = NULL;
       }
}
   else
       {
            APPLY_PRINT("%s - Default Value of primary dhcp server option is NULL\n", __FUNCTION__ );
       }

paramObjVal = cJSON_GetObjectItem(cJSON_GetObjectItem( partnerObj, "Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.IPv4SecondaryDhcpServerOptions"), "ActiveValue");
if ( paramObjVal != NULL )
{
   secdhcpoption = paramObjVal->valuestring;
   if(secdhcpoption[0]!='\0')
       {
            set_syscfg_partner_values(secdhcpoption,"IPv4SecondaryDhcpServerOptions");
            secdhcpoption = NULL;
       }
}
   else
       {
            APPLY_PRINT("%s - Default Value of Secondary dhcp server option is NULL\n", __FUNCTION__ );
       }

paramObjVal = cJSON_GetObjectItem(cJSON_GetObjectItem( partnerObj, "Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.IPv6PrimaryDhcpServerOptions"), "ActiveValue");
if ( paramObjVal != NULL )
{
   pridhcpoption = paramObjVal->valuestring;
   if(pridhcpoption[0]!='\0')
       {
            set_syscfg_partner_values(pridhcpoption,"IPv6PrimaryDhcpServerOptions");
            pridhcpoption = NULL;
       }
}
   else
       {
            APPLY_PRINT("%s - Default Value of primary dhcp server option is NULL\n", __FUNCTION__ );
       }

paramObjVal = cJSON_GetObjectItem(cJSON_GetObjectItem( partnerObj, "Device.X_RDKCENTRAL-COM_EthernetWAN_MTA.IPv6SecondaryDhcpServerOptions"), "ActiveValue");
if ( paramObjVal != NULL )
{
   secdhcpoption = paramObjVal->valuestring;
   if(secdhcpoption[0]!='\0')
       {
            set_syscfg_partner_values(secdhcpoption,"IPv6SecondaryDhcpServerOptions");
            secdhcpoption = NULL;
       }
}
   else
       {
            APPLY_PRINT("%s - Default Value of Secondary dhcp server option is NULL\n", __FUNCTION__ );
       }

					paramObjVal = cJSON_GetObjectItem(cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.WANsideSSH.Enable"), "ActiveValue");
					if ( paramObjVal != NULL )
					{
							char *WANsideSSHEnable = NULL;

							WANsideSSHEnable = paramObjVal->valuestring;

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

					paramObjVal = cJSON_GetObjectItem(cJSON_GetObjectItem( partnerObj, "Device.ManagementServer.EnableCWMP"), "ActiveValue");
                                        if ( paramObjVal != NULL )
                                        {
						char *pcEnableCWMP = NULL;
						
						pcEnableCWMP = paramObjVal->valuestring; 
			
						if (pcEnableCWMP != NULL) 
						{
							set_syscfg_partner_values( pcEnableCWMP,"Syndication_EnableCWMP" );
							pcEnableCWMP = NULL;
						}	
						else
						{
							APPLY_PRINT("%s - Default Syndication_EnableCWMP Value is NULL\n", __FUNCTION__ );
						}	
					}
#ifdef MTA_TR104SUPPORT
                                        paramObjVal = cJSON_GetObjectItem(cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_RFC.Feature.TR104.Enable"), "ActiveValue");
                                                            if ( paramObjVal != NULL )
{
    char *TR104Enable = NULL;
    TR104Enable = paramObjVal->valuestring;
    if(TR104Enable != NULL)
    {
        set_syscfg_partner_values(TR104Enable,"TR104Enable");
        TR104Enable = NULL;
    }
    else
    {
        APPLY_PRINT("%s - TR104Enable Value is NULL\n", __FUNCTION__ );
    }
}
#else
    APPLY_PRINT("TR104 is not supported so making TR104 value as false\n");
    set_syscfg_partner_values("false","TR104Enable");
#endif
				}
			}
			else
			{
				APPLY_PRINT("%s - partnerObj Object is NULL\n", __FUNCTION__ );
			}
		}
	}

    //Objects that always need to be checked
    // RDKB-28869 With Two Box Solutions NTP is now critical. JSON NTP Server 1 value must always be added if nothing exists for XBs to come online
    for (iterator = 0; iterator <= RETRY_COUNT; iterator++)
    {
       if (   ( 0 == syscfg_get(NULL, "ntp_server1", ntpServer1, sizeof(ntpServer1)))
           || (RETRY_COUNT == iterator)
       )
       {
          if(   (0 == strncmp(ntpServer1, "no_ntp_address", sizeof(ntpServer1)))
             || (0 == strnlen(ntpServer1, sizeof(ntpServer1)))
          )
          {
             alwaysJson = cJSON_Parse( data );
             if( !alwaysJson ) 
             {
                error_ptr = (char *)cJSON_GetErrorPtr();
                if (error_ptr != NULL)
                {
                   APPLY_PRINT(  "%s-%d : json file parser error at %s\n", __FUNCTION__,__LINE__, error_ptr);
                }
                else
                {
                   APPLY_PRINT(  "%s-%d : json file parser error\n", __FUNCTION__,__LINE__ );
                }
                return -1;
             } 
             else
             {
                APPLY_PRINT("%s - Applying  %s default ntp_server1 configuration\n", __FUNCTION__, PartnerID );
                alwaysPartnerObj = cJSON_GetObjectItem( alwaysJson, PartnerID );

                if(NULL != alwaysPartnerObj)
                {
                   /* NTP Server1 is blank set JSON value */
                   alwaysParamObjVal = cJSON_GetObjectItem(cJSON_GetObjectItem( alwaysPartnerObj, "Device.Time.NTPServer1"), "ActiveValue");

                   if ( alwaysParamObjVal != NULL )
                   {
                      jsonNTPServer1 = alwaysParamObjVal->valuestring;

                      if(jsonNTPServer1 != NULL)
                      {
                         if(0 != strnlen(jsonNTPServer1, sizeof(jsonNTPServer1)))
                         {
                            set_syscfg_partner_values(jsonNTPServer1,"ntp_server1");
                            APPLY_PRINT(" %s ntp_server1 set to json value:%s\n", __FUNCTION__, jsonNTPServer1);
                         }
                         else
                         {
                            APPLY_PRINT(" %s ntp_server1 NOT SET as json value from parse was EMPTY String\n", __FUNCTION__);
                         }
                         jsonNTPServer1 = NULL;
                      }
                      else
                      {
                         APPLY_PRINT("%s - jsonNTPServer1 Value is NULL\n", __FUNCTION__ );
                      }
                   } //if ( alwaysParamObjVal != NULL )
                   else
                   {
                      APPLY_PRINT("%s - alwaysParamObjVal Object is NULL\n", __FUNCTION__ );
                   }
                } //if(NULL == alwaysPartnerObj)
                else
                {
                   APPLY_PRINT("%s - alwaysPartnerObj Object is NULL\n", __FUNCTION__ );
                }
             } //if( !alwaysJson )
          }
          else
          {
             APPLY_PRINT(" %s ntp_server1 not default\n", __FUNCTION__);
          }

          break;
       }
       else
       {
          APPLY_PRINT("%s syscfg_get %d for ntp_server1 failed!\n", __FUNCTION__, iterator+1);
          sleep(1);
       }
    } //For Loop

    return 0;
}
#if defined (_XB6_PRODUCT_REQ_) || defined(_HUB4_PRODUCT_REQ_)
static void getPartnerIdWithRetry(char* buf, char* PartnerID)
{
        int i;
        //RDKB-23050: Chnage: Adding few retries to get the partnerId from syscfg.db and if still fails fall back to factory_partnerId
        for(i=0; i < RETRY_COUNT;i++)
        {
                //Get the partner ID
                syscfg_get( NULL, "PartnerID", buf, sizeof( buf ));
                if(buf[0] !=  '\0')
                {
                        strncpy( PartnerID, buf , strlen( buf ) );
                        APPLY_PRINT("%s:partnerId read from syscfg=%s\n",__FUNCTION__,PartnerID);
                        return;
                }
                else
                {
                        APPLY_PRINT("%s: will retry to get partnerId after 1 sec retrynum=%d\n",__FUNCTION__,i+1);
                        sleep(1);
                }
        }
        if(i == RETRY_COUNT)
        {
                APPLY_PRINT("%s: Did not get the partner Id with rety=%d also and fall back to factory_partnerId\n",__FUNCTION__,RETRY_COUNT);
                //fall back to factory_partnerId
                get_PartnerID(buf);
                if( buf[ 0 ] != '\0' )
                {
                        strncpy( PartnerID, buf , strlen( buf ) );
                }
                APPLY_PRINT("%s:partnerId read from factory=%s\n",__FUNCTION__,PartnerID);

        }
        return;
}
#endif

/*
 * main()
 */
int main( int argc, char **argv )
{
   char *ptr_etc_json = NULL, *ptr_nvram_json = NULL, *ptr_nvram_bs_json = NULL, *db_val = NULL;
   char  cmd[512] = {0};
   char  PartnerID[ PARTNER_ID_LEN+255 ]  = { 0 };
   int   isNeedToApplyPartnersDefault = 1;
   int   isMigrationReq = 0;
   int   rc;
   int retryCount = RETRY_COUNT;

   //Fill basic contents
   server_port = SE_SERVER_WELL_KNOWN_PORT;

   snprintf( server_ip, sizeof( server_ip ), "%s", SE_WELL_KNOWN_IP );
   snprintf( default_file, sizeof( default_file ), "%s", DEFAULT_FILE );

   syscfg_dirty = 0;

   parse_command_line(argc, argv);
   while ( retryCount && ((global_fd = sysevent_open(server_ip, server_port, SE_VERSION, SE_NAME, &global_id)) <= 0 ))
   {
      struct timeval t;

      APPLY_PRINT("[Utopia] global_fd is %d\n",global_fd);

      APPLY_PRINT("[Utopia] %s unable to register with sysevent daemon.\n", argv[0]);
      printf("[Utopia] %s unable to register with sysevent daemon.\n", argv[0]);

      //sleep with subsecond precision
      t.tv_sec = 0;
      t.tv_usec = 100000;
      select(0, NULL, NULL, NULL, &t);

      retryCount--;
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
      APPLY_PRINT("Number_Of_Entries_Commited_to_Sysconfig_Database=%d\n",syscfg_dirty);
   }

   sysevent_close(global_fd, global_id);

#if defined(_SYNDICATION_BUILDS_)
   system( "sh /lib/rdk/apply_partner_customization.sh" );
#endif

  if ( access( PARTNER_DEFAULT_APPLY_FILE , F_OK ) != 0 )  
  {
	  isNeedToApplyPartnersDefault = 0;
        APPLY_PRINT("%s - Device in Reboot mode :%s\n", __FUNCTION__, PARTNER_DEFAULT_APPLY_FILE );
	if ( access(PARTNERS_INFO_FILE, F_OK ) != 0 ) // Fix: RDKB-21731, Check if is single build migration
	{
		isMigrationReq = 1;
		APPLY_PRINT("%s - Device in Reboot mode, Syndication Migration Required\n", __FUNCTION__ )
	}
	
  }
  else
  {
	  isMigrationReq = 1;
	  APPLY_PRINT("%s - Device in FR mode :%s\n", __FUNCTION__, PARTNER_DEFAULT_APPLY_FILE );
  }
  
  if( (1 == isNeedToApplyPartnersDefault)||(isMigrationReq == 1) )  
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

#if !defined (_XB6_PRODUCT_REQ_) && !defined(_HUB4_PRODUCT_REQ_) && !defined(_SR300_PRODUCT_REQ_)
		//Partner ID is null so need to set default partner ID as "comcast"
		memset( PartnerID, 0, sizeof( PartnerID ) );
#if defined (_RDK_REF_PLATFORM_)
                sprintf( PartnerID, "%s", "RDKM");
#else
		sprintf( PartnerID, "%s", "comcast" );
#endif
		set_syscfg_partner_values( PartnerID, "PartnerID" );
		APPLY_PRINT("%s - PartnerID is NULL so set default partner :%s\n", __FUNCTION__, PartnerID );		
#else
 		//RDKB-23050: Change: Adding few retries to get the partnerId from syscfg.db and if still fails fall back to factory_partnerId
                getPartnerIdWithRetry(buf,PartnerID);
#endif

	}
  }

   APPLY_PRINT("%s - PartnerID :%s\n", __FUNCTION__, PartnerID );

   ptr_etc_json = json_file_parse( PARTNERS_INFO_FILE_ETC );
   if ( ptr_etc_json )
   {
      ptr_nvram_bs_json = json_file_parse( BOOTSTRAP_INFO_FILE );
      if ( ptr_nvram_bs_json == NULL )
      {
         ptr_nvram_json = json_file_parse( PARTNERS_INFO_FILE ); // nvram/partners_defaults.json can be removed after a few sprints.
         init_bootstrap_json( ptr_nvram_json, ptr_etc_json, PartnerID );
         if ( ptr_nvram_json == NULL )
         {
            snprintf(cmd, sizeof(cmd), "cp %s %s", PARTNERS_INFO_FILE_ETC, PARTNERS_INFO_FILE);
            APPLY_PRINT("%s\n",cmd);
            system(cmd);

            //Need to touch /tmp/.apply_partner_defaults_psm for PSM migration handling
            system("touch "PARTNER_DEFAULT_MIGRATE_PSM); // FIX: RDKB-20566 to handle migration
         }
         else
            free( ptr_nvram_json );
      }
      else
      {
         compare_partner_json_param( ptr_nvram_bs_json, ptr_etc_json, PartnerID );
         free( ptr_nvram_bs_json );
      }
      free( ptr_etc_json );
   }

   //Apply partner default values during FR/partner FR case
   db_val = json_file_parse( BOOTSTRAP_INFO_FILE );

   if( db_val )
   {
		apply_partnerId_default_values( db_val ,PartnerID );

		if( NULL != db_val )
		free( db_val );
   }

   return(0);
}

