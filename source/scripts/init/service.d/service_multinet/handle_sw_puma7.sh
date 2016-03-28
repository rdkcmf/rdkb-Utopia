#!/bin/sh

#------------------------------------------------------------------
# ENTRY
#------------------------------------------------------------------


# This is a empty switch handler in puma7 we don't need to call swctl directly rather we should be calling vlan utils to 
# create bridge and adding interfaces. Need to make changes in utopia scripts and code to handle it properly which is WIP
# Once utopia is modified we can remove this handler.

exit 0
