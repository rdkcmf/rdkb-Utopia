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
 *    FileName:    pal_xml.c
 *      Author:    Barry Wang (bowan@cisco.com)
 *        Date:    2009-05-05
 * Description:    PAL XML abstract interfaces
 *****************************************************************************/
/*$Id: pal_xml.c,v 1.3 2009/05/19 07:41:12 bowan Exp $
 *
 *$Log: pal_xml.c,v $
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

#define _GNU_SOURCE 
#include <stdlib.h>
#include <stdio.h>
#include <upnp/ixml.h>
#include "pal_xml.h"
#include <stdlib.h>
#include <string.h>

#ifdef PAL_DEBUG
#define pal_debug(fmt, args...) fprintf(stdout, "Debug[%s,%3d]: "fmt, __FILE__,__LINE__, ##args)
#else
#define pal_debug(fmt, args...)
#endif

#define CHARACTER_LT '<'
#define CHARACTER_GT '>'
#define CHARACTER_QUOT '"'
#define CHARACTER_AMP '&'

static VOID PAL_xml_escape_convert(const CHAR *src_str, CHAR *dest_str, INT32 * len, INT32 attr);

/************************************************************
 * Function: PAL_xml_nodelist_GetbyName 
 *
 *  Parameters:	
 *      top: Input. Search elemenets from this top tree.
 *      name: Input. Search target name of tag.
 *      ns_url: Input. Search the tag with name space url.  
 * 
 *  Description:
 *       Returns a nodeList of all the descendant Elements with a given
 *       name and namespace in the order in which they are encountered
 *       in a preorder traversal of the element tree.
 *      
 *  Return Values: pal_xml_nodelist *
 *      return all matched elemenets list, NULL if nothing found.
 ************************************************************/
pal_xml_nodelist *PAL_xml_nodelist_GetbyName(IN pal_xml_top *top,
                                                IN const CHAR *name,
                                                IN const CHAR *ns_url)
{
    pal_xml_nodelist *list = NULL;

    pal_debug("PAL_xml_nodelist_get_by_name start! %s\r\n", name);

    if (top == NULL || name == NULL)
        return NULL;
    
    if (ns_url){
        list = (pal_xml_nodelist *)ixmlDocument_getElementsByTagNameNS((IXML_Document *)top,ns_url,name);
    }else{
        list = (pal_xml_nodelist *)ixmlDocument_getElementsByTagName((IXML_Document *)top,name);
        if (list == NULL)
            list = (pal_xml_nodelist *)ixmlDocument_getElementsByTagNameNS((IXML_Document *)top,"*",name);
    }

    pal_debug("PAL_xml_nodelist_get_by_name end! %p\r\n", list);

    return list;
}

/************************************************************
 * Function: PAL_xml_nodelist_item 
 *
 *  Parameters:	
 *      list: Input. To get node from this list.
 *      index: Input. Need to get the indexth node.
 * 
 *  Description:
 *       Returns the indexth item in the collection. If index is greater
*       than or equal to the number of nodes in the list, this returns 
*       null.
 *      
 *  Return Values: pal_xml_node *
 *      return the matched elemenet node, NULL if nothing found.
 ************************************************************/                             
pal_xml_node *PAL_xml_nodelist_item (IN pal_xml_nodelist *list, IN ULONG idx)
{
    pal_xml_node *node = NULL;

    pal_debug("PAL_xml_nodelist_item start! %u\r\n", idx);

    if (list == NULL)
        return NULL;

    node = (pal_xml_node *)ixmlNodeList_item((IXML_NodeList *)list, idx);

    pal_debug("PAL_xml_nodelist_item end! %p\r\n", node);

    return node;
}


/************************************************************
 * Function: PAL_xml_node_GetFirstChild 
 *
 *  Parameters:	
 *      node: Input. As the parent node.
 * 
 *  Description:
 *      Returns the first child of node.
 *      
 *  Return Values: pal_xml_node *
 *      return the matched elemenet node, NULL if nothing found.
 ************************************************************/                           
pal_xml_node *PAL_xml_node_GetFirstChild(IN pal_xml_node *node)
{
    pal_xml_node *child_node = NULL;

    pal_debug("PAL_xml_node_get_first_child start! %s\r\n", ((IXML_Node *)node)->nodeName);

    if (node == NULL)
        return NULL;

    child_node = (pal_xml_node *)ixmlNode_getFirstChild((IXML_Node *)node);

    pal_debug("PAL_xml_node_get_first_child end!\r\n");
    
    return child_node;
}

/************************************************************
 * Function: PAL_xml_node_get_value 
 *
 *  Parameters:	
 *      node: Input. 
 * 
 *  Description:
 *      Returns the value of node.
 *      
 *  Return Values: CHAR *
 *      return string value of the elemenet, NULL if nothing found.
 ************************************************************/  
CHAR *PAL_xml_node_get_value(IN pal_xml_node *node)
{
    IXML_Node * text_node = NULL;
    CHAR *value = NULL;
    
    pal_debug("PAL_xml_node_get_value! %s\r\n",((IXML_Node *)node)->nodeName);
    
    if (node == NULL)
        return NULL;

    text_node = PAL_xml_node_GetFirstChild(node);
    if (text_node){
        value = (CHAR *)ixmlNode_getNodeValue((IXML_Node *)text_node);
    }

    pal_debug("PAL_xml_node_get_value! %s\r\n",value);

    return value;
}

/************************************************************
 * Function: PAL_xml_nodelist_free 
 *
 *  Parameters:	
 *      list: Input. The list to be freed.
 * 
 *  Description:
 *      Free a pal_node_list.
 *      
 *  Return Values: VOID
 *      
 ************************************************************/ 
VOID PAL_xml_nodelist_free(IN pal_xml_nodelist *list)
{
    pal_debug("PAL_xml_nodelist_free start!\r\n");
    ixmlNodeList_free((IXML_NodeList *)list);
    pal_debug("PAL_xml_nodelist_free end!\r\n");
}


/************************************************************
 * Function: PAL_xml_node_GetFirstbyName 
 *
 *  Parameters:	
 *      top: Input. Search the tag from this top tree.
 *      name: Input. Search target name of tag.
 *      ns_url: Input. Search the tag with name space url.  
 * 
 *  Description:
 *       Returns the first node mathc the given name and namespace.
 *      
 *  Return Values: pal_xml_node *
 *      return the matched elemenet node, NULL if nothing found.
 ************************************************************/
pal_xml_node *PAL_xml_node_GetFirstbyName(IN pal_xml_top *top,
                                             IN const CHAR *name,
                                             IN const CHAR *ns_url)
{
    pal_xml_nodelist *list = NULL;
    pal_xml_node * tmp_node = NULL;
    
    pal_debug("PAL_xml_node_get_first_by_name start!\r\n");

    if (top == NULL || name == NULL)
        return NULL;

    list = PAL_xml_nodelist_GetbyName(top,name,ns_url);
    if (list){
        tmp_node = PAL_xml_nodelist_item(list, 0);
        PAL_xml_nodelist_free(list);
    }

    pal_debug("PAL_xml_node_get_first_by_name end! %p\r\n", tmp_node);

    return tmp_node;
}


/************************************************************
 * Function: PAL_xml_parse_buffer 
 *
 *  Parameters:	
 *      buffer: Input. The buffer stores XML file.
 * 
 *  Description:
 *      Parse xml file stored in buffer.
 *      
 *  Return Values: pal_xml_top *
 *      return parsed tree. NULL if fail. 
 ************************************************************/
pal_xml_top *PAL_xml_parse_buffer(IN const CHAR *buffer)
{
    pal_debug("PAL_xml_parse_buffer start!\r\n");
    if (buffer == NULL)
        return NULL;

    pal_debug("PAL_xml_parse_buffer end!\r\n");

    return (pal_xml_top *)ixmlParseBuffer(buffer);
}


/************************************************************
 * Function: PAL_xml_nodelist_length 
 *
 *  Parameters:	
 *      list: Input. pal node list.
 * 
 *  Description:
 *      Returns the number of nodes in the list.  The range of valid
 *      child node indices is 0 to length-1 inclusive.
 *      
 *  Return Values: unsigned long
 *      Returns the number of nodes in the list.   
 ************************************************************/
ULONG PAL_xml_nodelist_length(IN pal_xml_nodelist *list)
{
    pal_debug("PAL_xml_nodelist_length start!\r\n");
    if (list == NULL)
        return 0;

    pal_debug("PAL_xml_nodelist_length end!\r\n");
    return ixmlNodeList_length((IXML_NodeList *)list);
}


/************************************************************
 * Function: PAL_xml_top_free 
 *
 *  Parameters:	
 *      top: Input. The top tree to be freed.
 * 
 *  Description:
 *      Free the top tree.
 *      
 *  Return Values: VOID   
 ************************************************************/
VOID PAL_xml_top_free(IN pal_xml_top *top)
{
    pal_debug("PAL_xml_top_free start!\r\n");
    if (top){
        ixmlDocument_free((IXML_Document *)top);
    }

    pal_debug("PAL_xml_top_free end!\r\n");
}


/************************************************************
 * Function: PAL_xml_node_print 
 *
 *  Parameters:	
 *      node: Input. The node to be printed.
 * 
 *  Description:
 *      Print DOM tree under node. Puts lots of white spaces
 *      
 *  Return Values: CHAR *
 *      return a string pointed to parsed result.
 ************************************************************/
CHAR *PAL_xml_node_print(IN pal_xml_node *node)
{
    pal_debug("PAL_xml_node_print start!\r\n");
    if (node == NULL)
        return NULL;
    pal_debug("PAL_xml_node_print end!\r\n");
    return ixmlPrintNode((IXML_Node *)node);
}


/************************************************************
 * Function: PAL_xml_top_print 
 *
 *  Parameters:	
 *      top: Input. The top tree to be printed.
 * 
 *  Description:
 *      Prints entire document, prepending XML prolog first.
 *      Puts lots of white spaces. 
 *      
 *  Return Values: CHAR *
 *      return a string pointed to parsed result.
 ************************************************************/
CHAR *PAL_xml_top_print(IN pal_xml_top *top)
{
    pal_debug("PAL_xml_top_print start!\r\n");
    if (top == NULL)
        return NULL;

    pal_debug("PAL_xml_top_print end!\r\n");
    return ixmlPrintDocument((IXML_Document *)top);
}


/************************************************************
 * Function: PAL_xml_top_creat 
 *
 *  Parameters:	
 *      N/A
 * 
 *  Description:
 *      Creates an xml top object.
 *            
 *  Return Values: pal_xml_top *
 *      A new top object with the nodeName set to "#document".
 ************************************************************/
pal_xml_top *PAL_xml_top_creat()
{
    pal_debug("PAL_xml_top_creat!\r\n");
    return (pal_xml_top *)ixmlDocument_createDocument();
}


/************************************************************
 * Function: PAL_xml_create_element 
 *
 *  Parameters:	
 *      top: Input. The xml top related to the element.
 *      namespace: Input. the namespace URI of the element to create.
 *      node_name: Input. the qualified name of the element to instantiate.
 * 
 *  Description:
 *      Creates an xml top object.
 *            
 *  Return Values: pal_xml_element *
 *      The new element object created.
 ************************************************************/
pal_xml_element *PAL_xml_element_create (IN pal_xml_top * top,
                                         IN const CHAR* tag_name,
                                         IN const CHAR* namespace_url)

{
    pal_xml_element * el = NULL;

    pal_debug("PAL_xml_element_create start!\r\n");
    
    if (top == NULL || tag_name == NULL)
        return NULL;

    if (namespace_url){
        el = (pal_xml_element *)ixmlDocument_createElementNS((IXML_Document *)top, namespace_url, tag_name);
    }else{
        el = (pal_xml_element *)ixmlDocument_createElement((IXML_Document *)top,tag_name); 
    }

    pal_debug("PAL_xml_element_create end!\r\n");

    return el;
}


/************************************************************
 * Function: PAL_xml_element_set_attr 
 *
 *  Parameters:	
 *       element: Input. The xml element.
 *       name: Input. The name of the attribute to create or alter.
 *       value: Input. Value to set in string form
 * 
 *  Description:
 *      Adds a new attribute.  If an attribute with that name is already
 *       present in the element, its value is changed to be that of the value
 *       parameter. If not, a new attribute is inserted into the element.
 *            
 *  Return Values: INT32
 *      0 or failure code. 
 ************************************************************/
INT32 PAL_xml_element_set_attr(IN pal_xml_element *element,
                              IN const CHAR *name,
                              IN const CHAR *value)

{
    INT32 ret = 0;

    pal_debug("PAL_xml_element_set_attr start!\r\n");
    
    if (!element || !name || !value)
        return PAL_XML_E_INVALID_PARAM;

    ret = ixmlElement_setAttribute((IXML_Element *)element, name, value);

    pal_debug("PAL_xml_element_set_attr end!\r\n");

    return ret;
}


/************************************************************
 * Function: PAL_xml_node_append_child 
 *
 *  Parameters:
 *       node: Input. The node as parent.
 *       new_child: Input. the node to add as child.
 * 
 *  Description:
 *      Adds the node newChild to the end of the list of children of this node.
 *      If the newChild is already in the tree, it is first removed.
 *            
 *  Return Values: INT32
 *      0 or failure code. 
 ************************************************************/
INT32 PAL_xml_node_append_child(IN pal_xml_node * node, IN pal_xml_node * new_child)
{
    INT32 ret = 0;

    pal_debug("PAL_xml_node_append_child start!\r\n");
    
    if (!node || !new_child)
        return PAL_XML_E_INVALID_PARAM;

    ret = ixmlNode_appendChild((IXML_Node *)node, (IXML_Node *)new_child);

    pal_debug("PAL_xml_node_append_child end!\r\n");
    return ret;
}


/************************************************************
 * Function: PAL_xml_top_create_textnode 
 *
 *  Parameters:
 *       top: Input. Related xml top.
 *       data: text data for the text node. It is stored in nodeValue field.
 *  Description:
 *       Creates an text node.
 *            
 *  Return Values: pal_xml_node *
 *      The new text node. 
 ************************************************************/
pal_xml_node *PAL_xml_top_create_textnode(IN pal_xml_top * top, IN const CHAR *data)
{
    pal_debug("PAL_xml_top_create_textnode !\r\n");
    if (!top || !data)
        return NULL;

    return (pal_xml_node *)ixmlDocument_createTextNode((IXML_Document *)top, data);
}


/************************************************************
 * Function: PAL_xml_top_AddElementTextValue 
 *
 *  Parameters:
 *       top: Input. Related xml top.
 *       parent: Input. The element to be changed.
 *       tag_name: Input. The sub element to be added.
 *       value: Input. text value of the node.
 *  Description:
 *       Add a sub element with only text value.
 *            
 *  Return Values: VOID
 *      
 ************************************************************/
VOID PAL_xml_top_AddElementTextValue(IN pal_xml_top *top, IN pal_xml_element *parent, IN CHAR *tag_name, IN CHAR *value)
{
    pal_xml_element *child_elt = NULL;
    pal_xml_node *child = NULL;

    pal_debug("PAL_xml_top_add_element_text_value start !\r\n");

    child_elt = PAL_xml_element_create(top,tag_name,NULL);
    child = PAL_xml_top_create_textnode(top,value);
    PAL_xml_node_append_child((pal_xml_node *)child_elt, child);
    PAL_xml_node_append_child ((pal_xml_node *)parent, child_elt);

    pal_debug("PAL_xml_top_add_element_text_value end !\r\n");
}


/************************************************************
 * Function: PAL_xml_top_AddElementIntValue 
 *
 *  Parameters:
 *       top: Input. Related xml top.
 *       parent: Input. The element to be changed.
 *       tag_name: Input. The sub element to be added.
 *       value: Input. INT32 value of the node.
 *  Description:
 *       Add a sub element with INT32 value.
 *            
 *  Return Values: VOID
 *      
 ************************************************************/
VOID PAL_xml_top_AddElementIntValue(IN pal_xml_top *top, IN pal_xml_element *parent, IN CHAR *tagname, IN INT32 value)
{
    CHAR *buf = NULL; 

    pal_debug("PAL_xml_top_add_element_int_value start !\r\n");
    
    /*CID 72621: Unchecked return value */
    if (asprintf(&buf,"%d",value) < 0)
    {
        pal_debug("PAL_xml_top_add_element_int_value asprintf failed !\r\n");
        return;
    }

    PAL_xml_top_AddElementTextValue(top, parent, tagname, buf); 
    
    free(buf); 

    pal_debug("PAL_xml_top_add_element_int_value end !\r\n");
}


/************************************************************
 * Function: PAL_xml_top_AddElementLongValue 
 *
 *  Parameters:
 *       top: Input. Related xml top.
 *       parent: Input. The element to be changed.
 *       tag_name: Input. The sub element to be added.
 *       value: Input. long value of the node.
 *  Description:
 *       Add a sub element with INT32 value.
 *            
 *  Return Values: VOID
 *      
 ************************************************************/
VOID PAL_xml_top_AddElementLongValue(IN pal_xml_top *top, IN pal_xml_element *parent,IN CHAR *tagname, IN long long value)
{
    CHAR *buf = NULL; 

    pal_debug("PAL_xml_top_add_element_long_value start !\r\n");

    /*CID 73193: Unchecked return value */
    if (asprintf(&buf,"%lld",value) < 0)
    {
	pal_debug("PAL_xml_top_add_element_long_value asprintf failed !\r\n");
	return;
    }
    PAL_xml_top_AddElementTextValue(top, parent, tagname, buf); 
    
    free(buf); 

    pal_debug("PAL_xml_top_add_element_long_value end !\r\n");
}


/************************************************************
 * Function: PAL_xml_escape 
 *
 *  Parameters:
 *       src_str: Input. Src string.
 *       attribute: Input. This string is attribute or not.
 *  Description:
 *       Escape the a src XML string to a normal string.
 *            
 *  Return Values: CHAR *
 *      Return the result escaped string. 
 ************************************************************/
CHAR *PAL_xml_escape(IN const CHAR *src_str, IN BOOL attribute)
{
	INT32 len = 0;
	CHAR *out = NULL;

    pal_debug("PAL_xml_escape start !\r\n");

	PAL_xml_escape_convert(src_str, NULL, &len, attribute);//caculate the memory size first
	out = malloc(len + 1);
    if (out)
	    PAL_xml_escape_convert(src_str, out, NULL, attribute);

    pal_debug("PAL_xml_escape end !\r\n");
    
	return out;
}

static VOID PAL_xml_escape_convert(const CHAR *src_str, CHAR *dest_str, INT32 * len, INT32 attr)
{
    INT32 length = 0;
    const char *XML_ESCAPE_STRINGS[] = {"&lt;","%22","&gt;","&amp;"};
    const int STRING_INDEX[] = {strlen(XML_ESCAPE_STRINGS[0]),strlen(XML_ESCAPE_STRINGS[1]),strlen(XML_ESCAPE_STRINGS[2]),strlen(XML_ESCAPE_STRINGS[3])};
    int index;

    while(*src_str) {
        switch(*src_str)
        {
        case CHARACTER_LT:
            index = 0;
            break;
        case CHARACTER_QUOT:
            index = attr ? 1 : -1;
            break;
        case CHARACTER_GT:
            index = 2;
            break;
        case CHARACTER_AMP:
            index = 3;
            break;
        default:
            index = -1;
            break;
        }

        if (index != -1)
        {
            if (dest_str)
                memcpy(dest_str+ length, XML_ESCAPE_STRINGS[index], STRING_INDEX[index]);
            length += STRING_INDEX[index];
        }
        else
        {
            if (dest_str)
                dest_str[length] = *src_str;
            length++;
        }
        src_str++;
    }

    if (dest_str)
        dest_str[length] = '\0';

    if (len)
        *len = length;

}

