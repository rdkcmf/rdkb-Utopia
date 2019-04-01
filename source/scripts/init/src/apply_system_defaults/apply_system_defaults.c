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
#if defined (_XB6_PRODUCT_REQ_)
#include "platform_hal.h"
#endif
#include "cJSON.h"
#include <unistd.h>
#include <stdbool.h>
#define PARTNERS_INFO_FILE  							"/nvram/partners_defaults.json"
#define PARTNERS_INFO_FILE_ETC                                                 "/etc/partners_defaults.json"
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
}

static int get_PartnerID( char *PartnerID)
{
	char buf[PARTNER_ID_LEN];
	memset(buf, 0, sizeof(buf));
	int isValidPartner = 0;

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
			isValidPartner = validatePartnerId ( PartnerID );
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
			isValidPartner = validatePartnerId ( PartnerID );
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

void ValidateAndUpdatePartnerVersionParam(cJSON *root_etc_json,cJSON *root_nvram_json, bool *do_compare)
{
    cJSON *properties_etc = NULL;
    cJSON *properties_nvram = NULL;
    char *version_etc = NULL;
    char *version_nvram = NULL;
    char *version_nvram_key = NULL;

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
            version_nvram = cJSON_GetObjectItem(properties_nvram,"version")->valuestring;

            if (version_etc)
            {        
                sscanf(version_etc,"%d.%d",&etc_major,&etc_minor);
                printf ("\n READ version ######## etc: major %d minor %d \n",etc_major, etc_minor);
            }
            /* Check if version exists inside properties object */
            version_nvram_key = cJSON_GetObjectItem(properties_nvram,"version");
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
        writeToJson(out);
        out = NULL;
    }
}

int compare_partner_json_param(char *partner_nvram_obj,char *partner_etc_obj,char *PartnerID)
{


	  cJSON   *subitem_nvram = NULL,  *subitem_etc = NULL;
	  char    *key = NULL,    *value = NULL, *nvram_key = NULL, *etc_key = NULL;
	  int     subitem_etc_count = 0, subitem_nvram_count = 0, iCount = 0;;
	  bool do_compare = false;

	  cJSON * root_nvram_json = cJSON_Parse(partner_nvram_obj);
	  cJSON * root_etc_json = cJSON_Parse(partner_etc_obj);  

        /* Check if version exists or has changed.
           If the function returns "true" proceed for comparison
           else there is no change. Just return
        */
        ValidateAndUpdatePartnerVersionParam(root_etc_json, root_nvram_json, &do_compare);
   	if (!do_compare)
        		return -1;
	  
	  /* Get row counts from json. It will give available no. of partner blocks in both files*/      
	  //rowCnt_etc = cJSON_GetArraySize(root_etc_json);
	  //rowCnt_nvram = cJSON_GetArraySize(root_nvram_json);
	  //printf("Counts == etc: %d, nvram: %d\n", rowCnt_etc,rowCnt_nvram);

    /* We reached here because there is some change in partners default.
       Update the nvram pointer.
       Get the object array for the current partner from:
       /nvram/partners_default.json and /etc/partners_default.json.
    */
          char* ptr_nvram_json = NULL;
          ptr_nvram_json = json_file_parse( PARTNERS_INFO_FILE );
          root_nvram_json=cJSON_Parse(ptr_nvram_json);
          root_etc_json = cJSON_Parse(partner_etc_obj);

	  subitem_etc = cJSON_GetObjectItem(root_etc_json,PartnerID);
	  subitem_nvram = cJSON_GetObjectItem(root_nvram_json,PartnerID);

	  //Printing sub object
	  /*char *json_string = cJSON_Print(subitem_etc);
	  if (json_string) 
	  {
	    printf("%s\n", json_string);
	  }*/

	  /* Partner Exists in /etc/partners_default.json and 
	     doesnt exist in /nvram/partners_default.json.
	     Add partner to nvram 
	   */
	  if (( subitem_nvram == NULL) && (subitem_etc != NULL))
	  {
	      printf("Partner object is not available in nvram\n");
	      cJSON_AddItemReferenceToObject(root_nvram_json,PartnerID,subitem_etc);
	      char *out = cJSON_Print(root_nvram_json);
	      writeToJson(out);
	      out = NULL;

	   }
	   else
	   {
	        /* 
	           Partner exists at both places.. Check count of partner variables.
	           Traverse through /etc/partners_default.json and check if the element
	           existing in /nvram/partners_default.json
	        */
	        subitem_etc_count = cJSON_GetArraySize(subitem_etc);
	        subitem_nvram_count = cJSON_GetArraySize(subitem_nvram);
	        
	        for (iCount=0;iCount<subitem_etc_count;iCount++)
	        {
	              key=cJSON_GetArrayItem(subitem_etc,iCount)->string;
	              //printf("String key is : %s\n",key);
	              nvram_key=cJSON_GetObjectItem(subitem_nvram,key);
	              if(nvram_key == NULL)
	              {	                 
	                 printf("Add parameter to /nvram/partners_default.json object array \n");
	                 value = cJSON_GetArrayItem(subitem_etc, iCount)->valuestring; 
	                 cJSON_AddItemToObject(subitem_nvram, key, cJSON_CreateString(value));

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
                         if ( 0 == strcmp ( key, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.HomeSec.SSIDprefix") )
                         {
                              set_syscfg_partner_values( value,"XHS_SSIDprefix" );
                              IsPSMMigrationNeeded = 1;
                         }
                         
                         //Check whether migration needs to be handled or not
                         if( 1 == IsPSMMigrationNeeded )											
                         {
                            APPLY_PRINT("%s - Adding new member in %s file\n", __FUNCTION__, PARTNERS_INFO_FILE);
                            APPLY_PRINT("%s - PSM Migration needed for %s param so touching %s file\n", __FUNCTION__, key, PARTNER_DEFAULT_MIGRATE_FOR_NEW_PSM_MEMBER );
												
                            //Need to touch /tmp/.apply_partner_defaults_new_psm_member for PSM migration handling
                            system("touch "PARTNER_DEFAULT_MIGRATE_FOR_NEW_PSM_MEMBER);			
                         }

	              }//if (nvram_key)

	         }//for loop 
	        
	        /* Check if nvram file has same count as etc file
	           if nvram has more entries, we may need to check what was 
	           removed from etc in current release.
	        */
	        subitem_etc_count = cJSON_GetArraySize(subitem_etc);
	        subitem_nvram_count = cJSON_GetArraySize(subitem_nvram);
	        if ( subitem_etc_count < subitem_nvram_count)
	        {
	            for (iCount=0;iCount<subitem_nvram_count;iCount++)
	            {
	               key=cJSON_GetArrayItem(subitem_nvram,iCount)->string;
	               //printf("String key is : %s\n",key);
	               etc_key=cJSON_GetObjectItem(subitem_etc,key);
	               if(etc_key == NULL)
	               {
	               	    printf("Delete parameter from /nvram/partners_default.json array \n");
	                    key=cJSON_GetArrayItem(subitem_nvram,iCount);
	                    cJSON_DeleteItemFromArray(subitem_nvram,iCount);
	                    //Decrement the count when an element is deleted
	                    subitem_nvram_count --;
	                }
	            }//for loop
	        }

	              char *out = cJSON_Print(root_nvram_json);
	              writeToJson(out);
	              out = NULL;
	    }

    cJSON_Delete(root_nvram_json);
    cJSON_Delete(root_etc_json);

    return 0;
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

					if ( cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.HomeSec.SSIDprefix") != NULL )
					{
						char *pcSSIDprefix = NULL;
						
						pcSSIDprefix = cJSON_GetObjectItem( partnerObj, "Device.DeviceInfo.X_RDKCENTRAL-COM_Syndication.HomeSec.SSIDprefix")->valuestring; 
			
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
   int   isMigrationReq = 0;
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

