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

/**********************************************************************
 *    FileName:    pal_xml2s.c
 *      Author:    Barry Wang (bowan@cisco.com)
 *        Date:    2009-05-05
 * Description:    PAL translator for XML to a data structure
 *****************************************************************************/
/*$Id: pal_xml2s.c,v 1.3 2009/05/19 07:41:12 bowan Exp $
 *
 *$Log: pal_xml2s.c,v $
 *Revision 1.3  2009/05/19 07:41:12  bowan
 *change some comments and data type as per common type definition
 *
 *Revision 1.2  2009/05/13 08:37:11  bowan
 *Change the header file seq to avoid the compile error of definition confict.
 *
 *Revision 1.1  2009/05/13 07:55:27  bowan
 *no message
 *
 *
 **/

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <upnp/ixml.h>
#include "pal_xml2s.h"
#include "string.h"


#ifdef PAL_DEBUG
#define pal_debug(fmt, args...) fprintf(stdout, "Debug[%s,%3d]: "fmt, __FILE__,__LINE__, ##args)
#else
#define pal_debug(fmt, args...)
#endif

#define PAL_DATA_MASKFILED_SET(data_l,mask_b)  \
    do{*(PAL_XML2S_FDMSK *)(data_l) |= mask_b;}while(0)
#define PAL_DATA_MASKFIELD_ISSET(data_l,mask_b) \
    (*(PAL_XML2S_FDMSK *)(data_l)& mask_b)
#define PAL_DATA_IS_OPTIONAL(rule_s) \
    (rule_s->type & PAL_XML2S_OPTIONAL)
#define PAL_DATA_IS_ARRAY(rule_s) \
    (rule_s->type & PAL_XML2S_ARRAY)
#define PAL_DATA_ARRAY_SIZEFD(data_l) \
    (*(PAL_ARRAY_SIZE *)(data_l))

LOCAL UINT32 PAL_xml2s_arraysize(UINT32 type, UINT32 st_size)
{
    switch(type){
        case PAL_XML2S_INT8:
        case PAL_XML2S_UINT8:
            return sizeof(INT8);
        case PAL_XML2S_INT16:
        case PAL_XML2S_UINT16:
            return sizeof(INT16);
        case PAL_XML2S_INT32:
        case PAL_XML2S_UINT32:
            return sizeof(INT32);
        case PAL_XML2S_STRING:
            return sizeof(CHAR *);
        case PAL_XML2S_STRUCT:
            return st_size;
        default:
            return 0;
    }
}

#define PAL_XML2S_STRING_MAX_LEN  16

LOCAL INT32 PAL_xml2s_inner_atoi(CHAR *pHandle,UINT32 *pValue)
{
	INT32 i;
	UINT32 v=0;

    pal_debug("PAL_xml2s_inner_atoi start! %s\r\n",pHandle);

	if(pHandle[0] == 0)
		return PAL_XML2S_E_INVALID_ARG;

	for(i=0;i< PAL_XML2S_STRING_MAX_LEN;i++){
		if(pHandle[i]==0)
			break;
		if((pHandle[i]>='0')&&(pHandle[i]<='9'))
			v = v*10 + (pHandle[i]-'0');
		else
			return PAL_XML2S_E_FORMAT;
	}
	if(i >= PAL_XML2S_STRING_MAX_LEN)
		return PAL_XML2S_E_FORMAT;
    
	*pValue = v;

    pal_debug("PAL_xml2s_inner_atoi end! %d\r\n",*pValue);
    
	return PAL_XML2S_E_SUCCESS;
}


/************************************************************
 * Function: PAL_xml2s_setvalue 
 *
 *  Parameters:	
 *      location: Input/Output, Point the location of the memory to be set
 *      type: Input. Data type
 *      value: Input. String value in the XML content.
 * 
 *  Description:
 *      Set the value to the location accroding to the datatype.      
 *      
 *  Return Values: UINT32
 *      0 if sucessful, otherwise return error code.
 ************************************************************/
UINT32 PAL_xml2s_setvalue(INOUT char *location, IN UINT32 type, IN char* value)
{
    INT32 ret = 0;
    UINT32 av = 0;
    
    
    if (!location || !value)
        return PAL_XML2S_E_INVALID_ARG;

    pal_debug("PAL_xml2s_setvalue start!%p %d, %s\r\n",location, type, value);

    if ((type >=  PAL_XML2S_INT8) && (type <= PAL_XML2S_UINT32)){
        ret = PAL_xml2s_inner_atoi(value, &av);
        if (ret != PAL_XML2S_E_SUCCESS)
            return ret;
    }
    
    switch (type){
    /*simple type*/
    case PAL_XML2S_INT8:
        *(INT8*)location = (INT8)av;
        break;
    case PAL_XML2S_INT16:
        *(INT16*)location = (INT16)av;
        break;
    case PAL_XML2S_INT32:
        *(INT32*)location = (INT32)av;
        break;
    case PAL_XML2S_UINT8:
        *(UINT8*)location = (UINT8)av;
        break;
    case PAL_XML2S_UINT16:
        *(UINT16*)location = (UINT16)av;
        break;
    case PAL_XML2S_UINT32:
        *(UINT32*)location = av;
        break;
    case PAL_XML2S_STRING: /*string type*/
        *(CHAR **)location = strdup(value);                                                             
        break;
    case PAL_XML2S_STRUCT: /*structure type*/
    default:
        //do nothing for unknow type
        break;
    }

    pal_debug("PAL_xml2s_setvalue end\r\n");

    return ret;
}


/************************************************************
 * Function: PAL_xml2s_process 
 *
 *  Parameters:	
 *      xml: Input. the xml dom tree(node) to be translated
 *      trans_table: Input. the translate table
 *      data_buff: InputOutput. Data structure memory buffer.
 * 
 *  Description:
 *       Translate the xml tree to a given data sturct according to the rules
 *       in translate table.
 *      
 *      
 *  Return Values: INT32
 *      0 if sucessful, otherwise return error code.
 ************************************************************/
INT32 PAL_xml2s_process(IN pal_xml_top *xml_in, IN PAL_XML2S_TABLE *table, INOUT VOID *data_out)
{
    UINT32 i = 0;
    PAL_XML2S_TABLE *rule = NULL;
    pal_xml_node *node = NULL;
    pal_xml_nodelist *node_list = NULL;
    INT32 ret = PAL_XML2S_E_SUCCESS;
    CHAR *array_buffer = NULL;
    CHAR *data_buffer = NULL;
    CHAR *val = NULL;

    pal_debug("PAL_xml2s_process start!\r\n");
    
    if (!xml_in || !table || !data_out)
        return PAL_XML2S_E_INVALID_ARG;

    rule = &(table[0]);
    data_buffer = data_out;

    while (rule && rule->tag_name){
        /*process array*/
        if (PAL_DATA_IS_ARRAY(rule)){
            /*get node list by name*/
            node_list = PAL_xml_nodelist_GetbyName(xml_in, rule->tag_name, NULL);
            if (node_list == NULL){ //empty list
                PAL_DATA_ARRAY_SIZEFD(data_buffer) = 0;
                return PAL_XML2S_E_SUCCESS; //return length of zero
            }
            
            /*get the length of the array*/
            ULONG length = PAL_xml_nodelist_length(node_list);
            if (length != 0){
               //calculate & allocate the memroy of array
               UINT32 size = PAL_xml2s_arraysize(rule->type & PAL_XML2S_MAX_BITS, rule->mask_bit);
               array_buffer = (CHAR*)calloc(length, size);
               if (array_buffer == NULL){
                   ret = PAL_XML2S_E_OUTOF_MEM;
                   break;
               }

               /*set array field*/
               PAL_DATA_ARRAY_SIZEFD(data_buffer) = length;
               *(VOID **)(data_buffer + rule->offset) = array_buffer;
      

               /*process node by node*/
               for (i=0; i<length; i++){
                   /*get single node from node list*/
                   pal_xml_node *tmp_node = PAL_xml_nodelist_item(node_list, i);

                   /*recursive handle for structure*/
                   if ((rule->type & PAL_XML2S_MAX_BITS) == PAL_XML2S_STRUCT){
                       ret = PAL_xml2s_process((pal_xml_top *)tmp_node, rule->child_s,array_buffer + (size*i));
                   }else{//simple type, including string
                       val = PAL_xml_node_get_value(tmp_node); //node value -- string
                       if (val == NULL) //ignore this error
                           continue;
                       ret = PAL_xml2s_setvalue((array_buffer + (size*i)),rule->type & PAL_XML2S_MAX_BITS,val);
                   }
                    
                   if (ret != PAL_XML2S_E_SUCCESS){//error occur
                       break;
                   }
               }//for (i=0; i<length; i++)                 
            }

            PAL_xml_nodelist_free(node_list);node_list = NULL;
            
            if (ret != PAL_XML2S_E_SUCCESS)
                break;
            
        }else{ //not array
            /*find the single tag accroding to its name*/
            
            node = PAL_xml_node_GetFirstbyName((pal_xml_node *)xml_in,rule->tag_name,NULL);
            if (node){
                /*set the maskfiled*/
                PAL_DATA_MASKFILED_SET(data_buffer,rule->mask_bit); 
                pal_debug("check the filedmask %d\r\n",*(PAL_XML2S_FDMSK *)(data_buffer));
                pal_debug("data type is %d\r\n", rule->type);

                /*recursive handle for structure*/
                if ((rule->type & PAL_XML2S_MAX_BITS) == PAL_XML2S_STRUCT){
                    ret = PAL_xml2s_process((pal_xml_top *)node,rule->child_s,data_buffer+rule->offset);
                }else{

                    val = PAL_xml_node_get_value(node);
                    if (val){
                        pal_debug("got the value %s!\r\n", val);
                        ret = PAL_xml2s_setvalue(data_buffer + rule->offset,rule->type&PAL_XML2S_MAX_BITS,val);
                    }else{
                        pal_debug("no value got from node %s!\r\n",((IXML_Node *)node)->nodeName);
                    }
                }
                if (ret != PAL_XML2S_E_SUCCESS) //if error, jump out
                    break;

            }else{ //not found the tag
               if (!PAL_DATA_IS_OPTIONAL(rule)){          
                   ret = PAL_XML2S_E_TAG_MISSING;
                   break;
               }
            }
        }//else for single element

        rule ++;
        
    }//while (rule && rule->tag_name)


    if (ret != PAL_XML2S_E_SUCCESS){
        PAL_xml2s_free(data_out, table);//need to free all the allocate memory
    }

    pal_debug("PAL_xml2s_process end!\r\n");
    
    return ret;
}


/************************************************************
 * Function: PAL_xml2s_free
 *
 *  Parameters:	
 *      data_buff: Input. The buffer to be freed.
 *      trans_table: The rule table for the free.
 *  Description:
 *       free all the memroy allocated during  PAL_xml2s_process.
 *       The data_buff must be got from PAL_xml2s_process with the same rule table.
 *      
 *  Return Values: VOID
 *  
 ************************************************************/
VOID PAL_xml2s_free(IN VOID *data_in, IN PAL_XML2S_TABLE *table)
{
    UINT32 i = 0;
    PAL_XML2S_TABLE *rule = NULL;
    //INT32 ret = 0;
    //CHAR *array_buffer = NULL;
    CHAR *data_buffer = NULL;

    pal_debug("PAL_xml2s_free start!\r\n");
    
    if (!data_in || !table)
        return;

    rule = &(table[0]);
    data_buffer = data_in;

    while (rule && rule->tag_name){
        /*process array*/
        if (PAL_DATA_IS_ARRAY(rule)){
            PAL_ARRAY_SIZE length = PAL_DATA_ARRAY_SIZEFD(data_buffer);
            UINT32 size = PAL_xml2s_arraysize(rule->type & PAL_XML2S_MAX_BITS, rule->mask_bit);
            CHAR *array_buffer = *(CHAR **)(data_buffer + rule->offset);

            if (length){ //at least one node in the list
               /*process node by node recursivly*/
               for (i=0; i<length; i++){
                   if ((rule->type & PAL_XML2S_MAX_BITS) == PAL_XML2S_STRUCT){
                       PAL_xml2s_free(array_buffer + (size*i), rule->child_s); 
                   }else if ((rule->type & PAL_XML2S_MAX_BITS) == PAL_XML2S_STRING){
                       CHAR *string_value = *(CHAR **)(array_buffer + (size*i));
                       pal_debug("free string of <%s> %s\r\n", rule->tag_name, string_value);
                       if (string_value){
                           free(string_value);
                       }
                   }
               }//for (i=0; i<length; i++)
               free (array_buffer);
            }            
        }else{
            if (PAL_DATA_MASKFIELD_ISSET(data_buffer,rule->mask_bit)){
                if ((rule->type & PAL_XML2S_MAX_BITS) == PAL_XML2S_STRUCT){
                    PAL_xml2s_free(data_buffer+rule->offset,rule->child_s);    
                }else if ((rule->type & PAL_XML2S_MAX_BITS) == PAL_XML2S_STRING){
                    CHAR *string_value = *(CHAR **)(data_buffer + rule->offset);
                    pal_debug("free string of <%s> %s\r\n", rule->tag_name, string_value);
                    free (string_value);
                }
            }
        }//else for single element

        rule ++;
    }//while (rule && rule->tag_name)

    pal_debug("PAL_xml2s_free end!\r\n");
}

