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
   Copyright [2015] [Cisco Systems, Inc.]
 
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
#ifndef MNET_UTIL_H
#define MNET_UTIL_H

typedef struct listItem {
    struct listItem* prev;
    struct listItem* next;
    
    void* data;
} ListItem, *PListItem;

typedef struct list {
    PListItem first;
    PListItem last;
    
    int count;
} List, *PList;

typedef struct listIterator {
    PList list;
    PListItem current;
    int bGiven;
} ListIterator, *PListIterator;

int addToList(PList list, void* itemData);
void* addAndAlloc(PList list, int dataSize);
int removeFromList(PList list, PListItem item);
int clearList(PList list);
int listSize(PList list);

int initIterator(PList list, PListIterator iterator);
//int copyIterator(PListIterator to, PListIterator from);
PListItem getNext(PListIterator iterator);
PListItem getCurrent(PListIterator iterator);
int removeCurrent(PListIterator iterator);


#endif
