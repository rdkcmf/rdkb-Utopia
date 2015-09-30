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
###########################################################################
#
# Copyright (c) 2008-2010 Cisco Systems, Inc. All rights reserved.
#
# Cisco Systems, Inc. retains all right, title and interest (including all
# intellectual property rights) in and to this computer program, which is
# protected by applicable intellectual property laws.  Unless you have obtained
# a separate written license from Cisco Systems, Inc., you are not authorized
# to utilize all or a part of this computer program for any purpose (including
# reproduction, distribution, modification, and compilation into object code),
# and you must immediately destroy or return to Cisco Systems, Inc. all copies
# of this computer program.  If you are licensed by Cisco Systems, Inc., your
# rights to utilize this computer program are limited by the terms of that
# license.  To obtain a license, please contact Cisco Systems, Inc.
#
# This computer program contains trade secrets owned by Cisco Systems, Inc.
# and, unless unauthorized by Cisco Systems, Inc. in writing, you agree to
# maintain the confidentiality of this computer program and related information
# and to not disclose this computer program and related information to any
# other person or entity.
#
# THIS COMPUTER PROGRAM IS PROVIDED AS IS WITHOUT ANY WARRANTIES, AND CISCO
# SYSTEMS, INC. EXPRESSLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
# INCLUDING THE WARRANTIES OF MERCHANTIBILITY, FITNESS FOR A PARTICULAR
# PURPOSE, TITLE, AND NONINFRINGEMENT.
#
###########################################################################

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
