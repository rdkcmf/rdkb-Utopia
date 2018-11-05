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
 *    FileName:    pal_xml.h
 *      Author:    Barry Wang (bowan@cisco.com)
 *        Date:    2009-05-05
 * Description:    Header file of PAL XML abstract interfaces
 *****************************************************************************/
/*$Id: pal_xml.h,v 1.2 2009/05/19 07:41:12 bowan Exp $
 *
 *$Log: pal_xml.h,v $
 *Revision 1.2  2009/05/19 07:41:12  bowan
 *change some comments and data type as per common type definition
 *
 *Revision 1.1  2009/05/13 07:53:02  bowan
 *no message
 *
 *
 **/
 
#ifndef __PAL_XML_H__
#define __PAL_XML_H__

#include "pal_def.h"

#ifndef IN
	#define IN
#endif

#ifndef OUT
	#define OUT
#endif

#ifndef INOUT
	#define INOUT
#endif

#define PAL_XML_E_SUCCESS              0
#define PAL_XML_E_INVALID_PARAM        -1

typedef VOID  pal_xml_node;
typedef VOID  pal_xml_nodelist;
typedef VOID  pal_xml_top;


typedef struct{
    pal_xml_node *node;
    CHAR *tag_name;
}pal_xml_element;

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
                                                IN const CHAR *ns_url);


/************************************************************
 * Function: PAL_xml_node_GetFirstbyName 
 *
 *  Parameters:	
 *      top: Input. Search the tag from this top tree.
 *      name: Input. Search target name of tag.
 *      ns_url: Input. Search the tag with name space url.  
 * 
 *  Description:
 *       Returns the first node matched the given name and namespace.
 *      
 *  Return Values: pal_xml_node *
 *      return the matched elemenet node, NULL if nothing found.
 ************************************************************/
 pal_xml_node *PAL_xml_node_GetFirstbyName(IN pal_xml_top *top,
                                                 IN const CHAR *name,
                                                 IN const CHAR *ns_url);


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
 pal_xml_node *PAL_xml_nodelist_item (IN pal_xml_nodelist *list, IN ULONG index);


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
 pal_xml_node *PAL_xml_node_GetFirstChild(IN pal_xml_node *node);


/************************************************************
 * Function: PAL_xml_node_get_value 
 *
 *  Parameters:	
 *      node: Input. The node to get value.
 * 
 *  Description:
 *      Returns the value of node.
 *      
 *  Return Values: CHAR *
 *      return string value of the elemenet, NULL if nothing found.
 ************************************************************/  
 CHAR *PAL_xml_node_get_value(IN pal_xml_node *node);


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
 VOID PAL_xml_nodelist_free(IN pal_xml_nodelist *list);



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
 pal_xml_top *PAL_xml_parse_buffer(IN const CHAR *buffer);


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
 *  Return Values: ULONG
 *      Returns the number of nodes in the list.   
 ************************************************************/
 ULONG PAL_xml_nodelist_length(IN pal_xml_nodelist *list);


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
 VOID PAL_xml_top_free(IN pal_xml_top *top);


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
 CHAR *PAL_xml_node_print(IN pal_xml_node *node);


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
 CHAR *PAL_xml_top_print(IN pal_xml_top *top);


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
 pal_xml_top *PAL_xml_top_creat();


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
                                         IN const CHAR* namespace);


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
                              IN const CHAR *value);


/************************************************************
 * Function: PAL_xml_node_append_child 
 *
 *  Parameters:
 *       node: Input. The node as parent.
 *       newChild: Input. the node to add as child.
 * 
 *  Description:
 *      Adds the node newChild to the end of the list of children of this node.
 *      If the newChild is already in the tree, it is first removed.
 *            
 *  Return Values: INT32
 *      0 or failure code. 
 ************************************************************/
INT32 PAL_xml_node_append_child(IN pal_xml_node * node, IN pal_xml_node * newChild);


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
 VOID PAL_xml_top_AddElementTextValue(IN pal_xml_top *top, 
                                    IN pal_xml_element *parent, 
                                    IN CHAR *tag_name, 
                                    IN CHAR *value);


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
 VOID PAL_xml_top_AddElementIntValue(IN pal_xml_top *top, 
                                        IN pal_xml_element *parent, 
                                        IN CHAR *tagname, 
                                        IN INT32 value);


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
 VOID PAL_xml_top_AddElementLongValue(IN pal_xml_top *top, 
                                         IN pal_xml_element *parent, 
                                         IN CHAR *tagname, 
                                         IN long long value);


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
pal_xml_node *PAL_xml_top_create_textnode(IN pal_xml_top * top, IN const CHAR *data);


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
CHAR *PAL_xml_escape(IN const CHAR *src_str, IN BOOL attribute);



#endif //__PAL_XML_H__
