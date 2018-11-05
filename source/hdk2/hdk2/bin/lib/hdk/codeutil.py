##########################################################################
# If not stated otherwise in this file or this component's Licenses.txt
# file the following copyright and licenses apply:
#
# Copyright 2015 RDK Management
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
##########################################################################
#######################################################################
#   Copyright [2014] [Cisco Systems, Inc.]
# 
#   Licensed under the Apache License, Version 2.0 (the \"License\");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
# 
#       http://www.apache.org/licenses/LICENSE-2.0
# 
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an \"AS IS\" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#######################################################################

#
# codeutil.py - Code generation utilities
#

from hslparser import HSLParser
import re

#
# Make a unique global symbol
#
def makeGlobalSymbol(prefix, namespace, name, namePrefix = "_", nameSuffix = ""):

    # Create the namespace prefix
    if not namespace:
        nsPrefix = ""
    elif namespace.startswith(HSLParser.HNAPNamespace):
        nsPrefix = namespace[len(HSLParser.HNAPNamespace):]
    elif namespace == HSLParser.HNAPNamespaceLegacy:
        nsPrefix = "PN"
    elif namespace.startswith(HSLParser.HNAPNamespaceLegacyBase) and \
            len(namespace) > len(HSLParser.HNAPNamespaceLegacyBase):
        nsPrefix = "PN/" + namespace[len(HSLParser.HNAPNamespaceLegacyBase):]
    else:
        nsPrefix = re.sub("^\w+:/+(www\.)?", "", namespace)
        nsPrefix = re.sub("\.com", "", nsPrefix)
        nsPrefix = re.sub("/+HNAPExt", "", nsPrefix)

    # Cleanup the namespace prefix
    if nsPrefix:
        nsPrefix = re.sub("/+$", "", nsPrefix)
        nsPrefix = nsPrefix[0].upper() + nsPrefix[1:]

    # Create the symbol prefix
    if prefix and nsPrefix:
        symPrefix = prefix + "_" + nsPrefix
    elif prefix:
        symPrefix = prefix
    else:
        symPrefix = nsPrefix

    # Create the symbol
    if name:
        if symPrefix:
            return makeSymbol(symPrefix + namePrefix + name + nameSuffix)
        else:
            return makeSymbol(name + nameSuffix)
    else:
        return makeSymbol(symPrefix)

#
# Make symbol
#
def makeSymbol(s, prefix = "", suffix = ""):

    return re.sub("[^\w]", "_", prefix + s + suffix)
