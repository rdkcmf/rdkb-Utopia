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
#include "service_multinet_util.h"
#include <stdlib.h>

// Zen is implementing collections

static void addItem(PList list, PListItem item) {
    if (!list->count) {
        list->first = item;
        list->last = item;
    } else {
        item->prev = list->last;
        list->last = item->prev->next = item;
    }
    
    list->count++;
}


int addToList(PList list, void* itemData) {
    PListItem item = calloc(1, sizeof(ListItem));
    item->data = itemData;
    
    addItem(list, item);
    return 0;
}

void* addAndAlloc(PList list, int dataSize) {
    PListItem item = calloc(1, sizeof(ListItem) + dataSize);
    addItem(list, item);
    
    item->data= (void*)(item +1);
    
    return item->data;
}

int removeFromList(PList list, PListItem item) {

    if (!item || !list)
        return -1;
    
    if (item->next) {
        item->next->prev = item->prev;
    } else {
        //item is last
       list->last = item->prev;
    }
    
    if (item->prev) {
        item->prev->next = item->next;
    } else { 
        //item is first
        list->first = item->next;
    }
    
    list->count--;
    free(item);
    item = NULL;
    return 0;
}

int clearList(PList list) {
    PListItem item;
    PListItem last;
    
    item = list->first;
    
    while(item) {
        last = item;
        item = item->next;
        free(last);
	last = NULL;
    }
    
    list->first = list->last = NULL;
    list->count = 0;
    return 0;
}

int listSize(PList list) {
    return list->count;
}

int initIterator(PList list, PListIterator iterator) {
    iterator->current = list->first;
    iterator->bGiven = 0;
    iterator->list = list;
    return 0;
}
//int copyIterator(PListIterator to, PListIterator from);
PListItem getNext(PListIterator iterator) {
    if (iterator->bGiven) {
        if ( iterator->current ) {
            iterator->current = iterator->current->next;
        }
    } else {
        iterator->bGiven = 1;
    }
    
    return iterator->current;
}
PListItem getCurrent(PListIterator iterator) {
    iterator->bGiven = 1;
    return iterator->current;
}
int removeCurrent(PListIterator iterator) {
    if (iterator->current) {
        PListItem delItem = iterator->current;
        iterator->bGiven = 0;
        iterator->current = iterator->current->next;
        removeFromList(iterator->list, delItem);
    }
    return 0;
}
