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
# hslparser.py - HNAP Specification Language (HSL) parser
#

import os
import re
import traceback


#
# HSL model class
#
class HSLModel:

    def __init__(self, actions, events, types, states, actionStates, services, docSections):

        self.actions = sorted(actions)
        self.events = sorted(events)
        self.types = sorted(types)
        self.states = sorted(states)
        self.actionStates = sorted(actionStates)
        self.services = sorted(services)
        self.docSections = list(docSections)

    # Return the list of action-referenced states
    def referenced_states(self):

        states = {}

        # Action-referenced states
        for actionState in self.actionStates:
            for stateMember in sorted(actionState.stateMembers.itervalues()):
                states[stateMember.state.uri] = stateMember.state

        return sorted(states.itervalues())

    # Return the list of action-referenced types
    def referenced_types(self):

        return self.all_types(user_types = False, state_types = False)

    # Return the list of all types (including built-in, auto, and state-only types)
    def all_types(self, user_types = True, state_types = True):

        types = {}

        # Action-referenced types
        for action in self.actions:
            self._all_types_helper(types, action.inputMember.type, include_type = False)
            self._all_types_helper(types, action.outputMember.type, include_type = False)

        # User types
        if user_types:
            for type in self.types:
                self._all_types_helper(types, type)

        # Action_state-referenced types
        if state_types:
            for state in self.referenced_states():
                self._all_types_helper(types, state.type)

        return sorted(types.itervalues())

    def _all_types_helper(self, types, type, include_type = True):

        if include_type:
            types[type.uri] = type
        if type.isStruct:
            for member in type.members:
                self._all_types_helper(types, member.type)


#
# HSL object class - all HSL objects derive from this class
#
class HSLObject:

    def __init__(self, file, line, namespace, name, doctext = []):

        self.file = file
        self.line = line
        self.namespace = namespace
        self.name = name
        self.doctext = doctext

        # Generate the URI from the namespace and name.
        if self.namespace[-1:] != "/":
            self.uri = self.namespace + "/" + self.name
        else:
            self.uri = self.namespace + self.name

    def __cmp__(self, other):
        return cmp((self.namespace, self.name), (other.namespace, other.name))


#
# HSL action class
#
class HSLAction(HSLObject):

    def __init__(self, file, line, namespace, name, parserOrder,
                 isHidden = False, isNoAuth = False, doctext = []):

        HSLObject.__init__(self, file, line, namespace, name, doctext = doctext)

        self.resultEnum = HSLType(file, line, namespace, name + "Result", parserOrder, isEnum = True)
        self.resultEnum.addEnumValue(file, line, "OK")
        self.resultEnum.addEnumValue(file, line, "ERROR")

        self.inputMember = HSLMember(file, line, namespace, name,
                                     HSLType(file, line, namespace, name, parserOrder, isStruct = True))
        self.outputMember = HSLMember(file, line, namespace, name + "Response",
                                      HSLType(file, line, namespace, name + "Response", parserOrder, isStruct = True),
                                      isErrorOutput = True)
        self.outputMember.type.addMember(HSLMember(file, line, namespace, name + "Result", self.resultEnum, isErrorOutput = True))

        self.isHidden = isHidden
        self.isNoAuth = isNoAuth

    @property
    def resultMember(self):
        return self.outputMember.type.members[0]


#
# HSL event class
#
class HSLEvent(HSLObject):

    def __init__(self, file, line, namespace, name, parserOrder, doctext = []):

        HSLObject.__init__(self, file, line, namespace, name, doctext = doctext)

        self.member = HSLMember(file, line, namespace, name,
                                HSLType(file, line, namespace, name, parserOrder, isStruct = True))


#
# HSL type class
#
class HSLType(HSLObject):

    def __init__(self, file, line, namespace, name, parserOrder, schemaName = None,
                 isStruct = False, isArray = False, isEnum = False,
                 isBuiltin = False, doctext = []):

        HSLObject.__init__(self, file, line, namespace, name, doctext = doctext)

        if schemaName is not None:
            self.schemaName = schemaName
        else:
            self.schemaName = name
        self.parserOrder = parserOrder
        if isStruct or isArray:
            self.members = []
        else:
            self.members = None
        if isEnum:
            self.enumValues = []
            self.enumValuesEx = []
        else:
            self.enumValues = None
            self.enumValuesEx = None
        self.isArray = isArray
        self.isBuiltin = isBuiltin

    # Structs
    @property
    def isStruct(self):
        return self.members is not None
    def addMember(self, member):
        self.members.append(member)

    # Enumerations
    @property
    def isEnum(self):
        return self.enumValues is not None
    def addEnumValue(self, file, line, value, doctext = []):
        if value not in self.enumValues:
            self.enumValues.append(value)
            self.enumValues.sort(key = self._sortEnumValues_Key)
            self.enumValuesEx.append(HSLObject(file, line, self.namespace, value, doctext = doctext))
            self.enumValuesEx.sort(key = self._sortEnumValues_Key)
    def resetEnumValues(self):
        self.enumValues[:] = []
        self.enumValuesEx[:] = []

    # Helper method for sortEnumValues
    def _sortEnumValues_Key(self, value):

        if isinstance(value, HSLObject):
            value = value.name

        if value == "OK":
            return "A0"
        elif value == "REBOOT":
            return "A1"
        elif value == "ERROR":
            return "A2"
        else:
            return "B" + value

    # Arrays
    @property
    def arrayType(self):
        return self.members[0].type

    # Is the given type representable as a csv value?
    # - All child members are bounded
    # - Any optional members (including children) are last
    def validateCSV(self):
        return self._validateCSVHelper(True, True, None)

    def _validateCSVHelper(self, bAllowOptional, bAllowArray, prevMember):

        if self.isArray and not bAllowArray:
            return False                # array type not allowed at this level

        if self.isStruct:

            # Check the members
            for member in self.members:

                if member.isOptional:
                    if not bAllowOptional:
                        return False    # invalid optional member
                elif prevMember is not None and prevMember.isOptional:
                    return False        # invalid required member

                error = member.type._validateCSVHelper(not self.isArray, False, prevMember)
                if not error:
                    return error

                prevMember = member

        # Atomic types are always representable in csv
        return True


#
# HSL struct member class
#
class HSLMember(HSLObject):

    def __init__(self, file, line, namespace, name, type, isUnbounded = False, isOptional = False, isCSV = False,
                 isErrorOutput = False, doctext = []):

        HSLObject.__init__(self, file, line, namespace, name, doctext = doctext)

        self.type = type
        self.isUnbounded = isUnbounded
        self.isOptional = isOptional
        self.isCSV = isCSV
        self.isErrorOutput = isErrorOutput


#
# HSL state class
#
class HSLState(HSLObject):

    def __init__(self, file, line, namespace, name, type, doctext = []):

        HSLObject.__init__(self, file, line, namespace, name, doctext = doctext)

        self.type = type


#
# HSL action state class
#
class HSLActionState(HSLObject):

    def __init__(self, file, line, namespace, name, doctext = []):

        HSLObject.__init__(self, file, line, namespace, name, doctext = doctext)

        self.stateMembers = {}


#
# HSL state member class
#
class HSLActionStateMember(HSLObject):

    def __init__(self, file, line, state, isGet, isSet, doctext = []):

        HSLObject.__init__(self, file, line, state.namespace, state.name, doctext = doctext)

        self.state = state
        self.isGet = isGet
        self.isSet = isSet


#
# HSL service class
#
class HSLService(HSLObject):

    def __init__(self, file, line, namespace, name, doctext = []):

        HSLObject.__init__(self, file, line, namespace, name, doctext = doctext)

        self.actions = {}
        self.events = {}


#
# HSL documentation section
#
class HSLDocSection(HSLObject):

    def __init__(self, file, line, type, title):

        HSLObject.__init__(self, file, line, "", "", doctext = [])

        self.type = type
        self.title = title


#
# HSL parser class
#
class HSLParser:

    # HNAP namespace
    HNAPNamespace = "http://cisco.com/HNAP/"
    HNAPNamespaceLegacy = "http://purenetworks.com/HNAP1/"
    HNAPNamespaceLegacyBase = "http://purenetworks.com/HNAP/"

    # Automatic types and actions
    HNAPGetDeviceSettings = (HNAPNamespaceLegacy, "GetDeviceSettings")
    HNAPGetServices = (HNAPNamespace, "GetServices")
    HNAPGetServiceInfo = (HNAPNamespace, "GetServiceInfo")

    def __init__(self, importPath = [], noauth = False):

        # Regex
        sComment = '\s*(#(?P<doc>@(?P<docsect>@\s*(?P<doctype>\w*))?)?\s*(?P<doctext>.*?)\s*)?$'
        sAttr = '(\[\s*(?P<attr>(\w+)(\s*=\s*"\w+")?(\s*,\s*(\w+)(\s*=\s*"\w+")?)*)\s*\]\s*)?'
        sType = '((?P<ns>\w+):)?(?P<type>\w+)(?P<array>\s*\[\])?'
        self._reAttribute = re.compile('^(?P<name>\w+)\s*=\s*"(?P<value>\w+)"$')
        self._reComment = re.compile('^' + sComment)
        self._reLineContinuation = re.compile('\\\\' + sComment)
        self._reImport = re.compile('^import\s+"\s*(?P<file>.+?)\s*"' + sComment)
        self._reNamespace = re.compile('^namespace(\s+(?P<ns>\w+))?\s+"\s*(?P<namespace>.+?)\s*"' + sComment)
        self._reNamedDef = re.compile('^' + sAttr + '(?P<object>enum|struct|action|action_state|event|service)\s+(?P<name>\w+)' + sComment)
        self._reActionDef = re.compile('^\s+(?P<object>input|output|result)' + sComment)
        self._reActionStateDef = re.compile('^\s+(?P<object>state)' + sComment)
        self._reMember = re.compile('^\s+' + sAttr + sType + '\s+(?P<name>\w+)' + sComment)
        self._reValue = re.compile('^\s+"(?P<value>[\w\.\-\+\/ ]*?)"' + sComment)
        self._reState = re.compile('^' + sAttr + '(?P<object>state)\s+' + sType + '\s+(?P<name>\w+)' + sComment)
        self._reReference = re.compile('^\s+' + sAttr + '((?P<ns>\w+):)?(?P<name>\w+)' + sComment)
        self._reServiceDef = re.compile('^\s+(?P<object>actions|events)' + sComment)
        self._reNamespaceURI = re.compile('^(([a-zA-Z][0-9a-zA-Z+\-.]*:)?/{0,2}[0-9a-zA-Z;/?:@&=+$.\-_!~*\'\(\)%]+)?(#[0-9a-zA-Z;/?:@&=+$.\-_!~*\'\(\)%]+)?$')

        # Valid attribute (name, isNameValue) tuples
        self._validAttributes = { "enum": [],
                                  "struct": [],
                                  "action": [("hidden", False), ("noauth", False)],
                                  "_member": [("optional", False), ("error", False), ("csv", False)],
                                  "state": [],
                                  "action_state": [],
                                  "_action_state_state": [("get", False), ("set", False)],
                                  "service": [],
                                  "_service_action": [],
                                  }

        # Namespaces
        self._nsSchema = "http://www.w3.org/2001/XMLSchema"

        # Built-in types
        self._builtinTypeNames = {}
        builtinParserOrder = -1
        for builtinName, builtinNS, builtinSchema in \
                (("bool",       self._nsSchema,    "boolean"),
                 ("int",        self._nsSchema,    "int"),
                 ("string",     self._nsSchema,    "string"),
                 ("long",       self._nsSchema,    "long"),
                 ("datetime",   self._nsSchema,    "dateTime"),
                 ("blob",       self._nsSchema,    "base64Binary"),
                 ("IPAddress",  HSLParser.HNAPNamespace, "IPAddress"),
                 ("MACAddress", HSLParser.HNAPNamespace, "MACAddress"),
                 ("UUID", HSLParser.HNAPNamespace, "UUID")):

            builtinType = HSLType("builtin", 0, builtinNS, builtinName, builtinParserOrder,
                                  schemaName = builtinSchema, isBuiltin = True)

            self._builtinTypeNames[builtinType.name] = builtinType
            builtinParserOrder -= 1

        # Set the import path
        self._importPath = importPath

        # Default action "noauth" state
        self._noauth = noauth

        # Reset the parser state
        self.reset()

    #
    # Parser result access
    #
    def getmodel(self):

        return HSLModel(self._actions.itervalues(),
                        self._events.itervalues(),
                        self._types.itervalues(),
                        self._states.itervalues(),
                        self._actionStates.itervalues(),
                        self._services.itervalues(),
                        [ docSection for docSection in self._docSections if docSection is not None ])

    #
    # Reset the parser state
    #
    def reset(self):

        self._actions = {}
        self._events = {}
        self._types = {}
        self._states = {}
        self._actionStates = {}
        self._services = {}
        self._docSections = [ None ]
        self._imports = {}
        self.errors = []
        self._ixParserOrder = 0
        self._auto_types = {}

    # Parser object order accessor
    def _parserOrder(self):
        result = self._ixParserOrder
        self._ixParserOrder += 1
        return result

    # Add an HSL parsing errors
    def _addError(self, text):
        self.errors.append("Error: %s" % (text))
    def _addFileError(self, file, line, text):
        self.errors.append("%s:%d: %s" % (file.replace('\\', '/'), line, text))

    # Add doctext
    def _addDoctext(self, file, ixLine, doctext, m):

        if m.group("docsect") is not None:

            # Handle the doc section
            if m.group("doctype") == "end":
                if self._docSections[-1] is not None:
                    self._docSections.append(None)
            elif m.group("doctype") in ("top", "h1", "h2", "h3", "h4", "title", "version", "date", "abstract", "copyright"):
                if m.group("doctext"):
                    if self._docSections[-1] is None:
                        del self._docSections[-1]
                    self._docSections.append(HSLDocSection(file, ixLine, m.group("doctype"), m.group("doctext")))
                else:
                    self._addFileError(file, ixLine, "Empty documentation section title")
            else:
                self._addFileError(file, ixLine, "Unknown documentation section type")

        elif m.group("doc") is not None:

            # Doc section?
            if self._docSections[-1] is not None:
                self._docSections[-1].doctext.append(m.group("doctext"))
            else:
                doctext.append(m.group("doctext"))

    # Parse attribute list into dictionary and validate
    def _parseAttributes(self, file, ixLine, attrList, objName):

        # Split on commas
        attrs = re.split("\s*,\s*", attrList)

        # Parse the attributes into name, value pairs
        attrDict = {}
        for attr in attrs:
            name = attr
            value = None
            m_attr = self._reAttribute.search(attr)
            if m_attr:
                name = m_attr.group("name")
                value = m_attr.group("value")

            # Return an error if there are duplicate attributes
            if name in attrDict:
                self._addFileError(file, ixLine, "duplicate attribute '%s'" % (name))
            attrDict[name] = value

        # Check that each attribute is valid
        for invalidAttr in [attrName for attrName, attrValue in attrDict.iteritems() \
                                if attrName not in [attr for attr, pair in self._validAttributes[objName]]]:
            self._addFileError(file, ixLine, "invalid attribute '%s'" % (invalidAttr))

        # Check that each attribute that is a name/value pair has a value and that
        # each attribute that is not a name/value pair does not have a value
        for attrName, attrValue in attrDict.iteritems():
            for attr, isNameValue in self._validAttributes[objName]:
                if attrName == attr:
                    if attrValue is not None and isNameValue is False:
                        self._addFileError(file, ixLine, "'%s' is not a name/value pair attribute" % (attrName))
                    if attrValue is None and isNameValue is True:
                        self._addFileError(file, ixLine, "'%s' is a name/value pair attribute" % (attrName))

        return attrDict

    # Create a member type
    def _createMemberType(self, file, ixLine, namespace, namespaces, typeNSAbbrev, type, isArray, attrDict):

        memberType = None

        # Get the type namespace
        typeNS = namespace
        if typeNSAbbrev is not None:
            if typeNSAbbrev not in namespaces:
                return (None, "namespace '%s' not defined" % (typeNSAbbrev))
            typeNS = namespaces[typeNSAbbrev]

        # Get the member type
        if typeNSAbbrev is None and type in self._builtinTypeNames:
            memberType = self._builtinTypeNames[type]
        else:
            typeUri = HSLType(file, ixLine, typeNS, type, 0).uri
            if typeUri in self._types:
                memberType = self._types[typeUri]
            else:
                return (None, "type '%s' not defined" % (type))

        # Array members create an array type (if necessary)
        if isArray:
            arrayTypeName = "ArrayOf" + memberType.schemaName[0].upper() + memberType.schemaName[1:]

            # The array type's member namespace
            if memberType.isStruct:
                arrayMemberNS = memberType.namespace
            else:
                arrayMemberNS = namespace

            # The array type's member name
            if memberType.isEnum:
                arrayMemberName = "string"
            else:
                arrayMemberName = memberType.name

            # Create the array type
            arrayType = HSLType(file, ixLine, namespace, arrayTypeName, self._parserOrder(), isArray = True)
            arrayType.addMember(HSLMember(file, ixLine, arrayMemberNS, arrayMemberName, memberType,
                                           isOptional = True, isUnbounded = True))
            if arrayType.uri in self._types:
                self._addFileError(file, ixLine, "redefinition of '%s' type" % (arrayType.name))
            elif arrayType.uri in self._auto_types:
                if not self._auto_types[arrayType.uri].isArray:
                    self._addFileError(file, ixLine, "redefinition of '%s' type" % (arrayType.name))
                else:
                    arrayType = self._auto_types[arrayType.uri]
            else:
                self._auto_types[arrayType.uri] = arrayType
            memberType = arrayType

        return (memberType, None)

    #
    # Parse an HSL file
    #
    def parseFile(self, file, isImport = False):

        # Add the file to the imports
        self._imports[os.path.abspath(file)] = file

        # Parse the file
        stream = None
        try:
            stream = open(file, "r")
            self.parseStream(stream, file = file, isImport = isImport)
        except:
            self._addError("couldn't open '%s'" % (file))
        if stream is not None:
            stream.close()

    #
    # Finalize parsing - call after last parseFile
    #
    def parseDone(self):

        # Verify action_state
        for actionState in sorted(self._actionStates.itervalues(), key = lambda x: (x.file, x.line)):

            # Verify action references
            if actionState.uri not in self._actions:
                self._addFileError(actionState.file, actionState.line, "action '%s' not defined" % (actionState.name))

            # Empty stateMembers not allowed
            if len(actionState.stateMembers) == 0:
                self._addFileError(actionState.file, actionState.line, "action_state '%s' is empty" % (actionState.name))

        # Verify services
        for service in sorted(self._services.itervalues(), key = lambda x: (x.file, x.line)):

            # Verify action references
            for actionUri in sorted(service.actions.iterkeys()):
                if actionUri not in self._actions:
                    self._addFileError(service.file, service.line, "action '%s' not defined" % (actionUri))
                else:
                    service.actions[actionUri] = self._actions[actionUri]

            # Verify event references
            for uri in sorted(service.events.iterkeys()):
                if uri not in self._events:
                    self._addFileError(service.file, service.line, "event '%s' not defined" % (uri))
                else:
                    service.events[uri] = self._events[uri]

    #
    # Parse an HSL stream
    #
    def parseStream(self, stream, file = "", isImport = False):

        # Parser state
        lineContinuation = None
        ixLine = 0
        curNamespace = None
        namespaces = {}
        curObject = None
        curActionObject = None
        curActionStateObjectMap = None
        curServiceObjectMap = None
        enums = []
        doctext = []

        # Parse each line
        try:
            for line in stream:
                ixLine += 1

                # Ignore Comment lines
                m = self._reComment.search(line)
                if m:
                    self._addDoctext(file, ixLine, doctext, m)
                    continue

                # Handle line continuation
                m = self._reLineContinuation.search(line)
                if m:
                    # Add the comment doctext
                    self._addDoctext(file, ixLine, doctext, m)

                    # Continue the line
                    if lineContinuation is None:
                        lineContinuation = self._reLineContinuation.sub("", line)
                    else:
                        lineContinuation += self._reLineContinuation.sub("", line)
                    continue
                elif lineContinuation is not None:
                    line = lineContinuation + line
                    lineContinuation = None

                # Add imports to the file list
                m = self._reImport.search(line)
                if m:
                    # Add the comment doctext
                    self._addDoctext(file, ixLine, doctext, m)

                    # Add the import
                    importFile = m.group("file")
                    importPath = [os.path.dirname(file)]
                    importPath.extend(self._importPath)
                    importFiles = [os.path.join(importDir, importFile) for
                                   importDir in importPath if
                                   os.path.isfile(os.path.join(importDir, importFile))]
                    if not importFiles:
                        self._addFileError(file, ixLine, "import '%s' not found" % (importFile))
                    else:
                        importAbs = os.path.abspath(importFiles[0])
                        if importAbs not in self._imports:

                            # Save the doctext sections
                            docSectionsOld = self._docSections
                            self._docSections = [ None ]

                            # Import the file
                            self._imports[importAbs] = file
                            self.parseFile(importFiles[0], isImport = True)

                            # Restore the doctext sections
                            self._docSections = docSectionsOld
                    continue

                # Handle namespace definitions
                m = self._reNamespace.search(line)
                if m:
                    # Add the comment doctext
                    self._addDoctext(file, ixLine, doctext, m)

                    # Validate the namespace URI
                    if not self._reNamespaceURI.search(m.group("namespace")):
                        self._addFileError(file, ixLine, "invalid namespace URI '%s'" % (m.group("namespace")))

                    # Set the current namespace
                    if m.group("ns") is not None:
                        namespaces[m.group("ns")] = m.group("namespace")
                    else:
                        curNamespace = m.group("namespace")
                    continue

                # Handle named definitions
                m = self._reNamedDef.search(line)
                if m:
                    object = m.group("object")
                    name = m.group("name")

                    # Parse attributes
                    attrDict = {}
                    if m.group("attr") is not None:
                        attrDict = self._parseAttributes(file, ixLine, m.group("attr"), object)

                    # Add the comment doctext
                    self._addDoctext(file, ixLine, doctext, m)

                    # Namespace required
                    if curNamespace is None:
                        self._addFileError(file, ixLine, "namespace required for %s definition" % (object))
                        break

                    # Create the object
                    if object == "action":

                        # Create the action
                        curObject = HSLAction(file, ixLine, curNamespace, name, self._parserOrder(),
                                               isHidden = "hidden" in attrDict,
                                               isNoAuth = self._noauth or "noauth" in attrDict,
                                               doctext = doctext)
                        curActionObject = None
                        doctext = []

                        # Add the action
                        if curObject.uri in self._actions:
                            redef = self._actions[curObject.uri]
                            if os.path.abspath(file) != os.path.abspath(redef.file) or ixLine != redef.line:
                                self._addFileError(file, ixLine, "redefinition of '%s' action" % (curObject.name))
                        elif not isImport:
                            self._actions[curObject.uri] = curObject

                            # Check for redefinition of the action's result enum type
                            if curObject.resultEnum.uri in self._types:
                                self._addFileError(file, ixLine, "redefinition of '%s' type" % (curObject.resultEnum.name))
                            elif curObject.resultEnum.uri in self._auto_types:
                                self._addFileError(file, ixLine, "redefinition of '%s' type" % (curObject.resultEnum.name))
                            else:
                                self._auto_types[curObject.resultEnum.uri] = curObject.resultEnum

                    elif object == "action_state":

                        # Create the action_state (URI is identical to corresponding action)
                        curObject = HSLActionState(file, ixLine, curNamespace, name, doctext = doctext)
                        curActionStateObjectMap = None
                        doctext = []

                        # Add the action_state
                        if curObject.uri in self._actionStates:
                            redef = self._actionStates[curObject.uri]
                            if os.path.abspath(file) != os.path.abspath(redef.file) or ixLine != redef.line:
                                self._addFileError(file, ixLine, "redefinition of '%s' action_state" % (curObject.name))
                        elif not isImport:
                            self._actionStates[curObject.uri] = curObject

                    elif object == "event":

                        # Create the event
                        curObject = HSLEvent(file, ixLine, curNamespace, name, self._parserOrder(),
                                             doctext = doctext)
                        curEventObject = None
                        doctext = []

                        # Add the event
                        if curObject.uri in self._events:
                            redef = self._events[curObject.uri]
                            if os.path.abspath(file) != os.path.abspath(redef.file) or ixLine != redef.line:
                                self._addFileError(file, ixLine, "redefinition of '%s' event" % (curObject.name))
                        elif not isImport:
                            self._events[curObject.uri] = curObject

                    elif object == "service":

                        # Create the service
                        curObject = HSLService(file, ixLine, curNamespace, name, doctext = doctext)
                        curServiceObjectMap = None
                        doctext = []

                        # Add the service
                        if curObject.uri in self._services:
                            redef = self._services[curObject.uri]
                            if os.path.abspath(file) != os.path.abspath(redef.file) or ixLine != redef.line:
                                self._addFileError(file, ixLine, "redefinition of '%s' service" % (curObject.name))
                        elif not isImport:
                            self._services[curObject.uri] = curObject

                    else:

                        # Create the enum or struct
                        if object == "enum":
                            curObject = HSLType(file, ixLine, curNamespace, name, self._parserOrder(), isEnum = True, doctext = doctext)
                            enums.append(curObject)
                            doctext = []
                        elif object == "struct":
                            curObject = HSLType(file, ixLine, curNamespace, name, self._parserOrder(), isStruct = True, doctext = doctext)
                            doctext = []

                        # Add the type
                        if name in self._builtinTypeNames:
                            self._addFileError(file, ixLine, "redefinition of '%s' built-in type" % (name))
                        elif curObject.uri in self._types:
                            redef = self._types[curObject.uri]
                            if os.path.abspath(file) != os.path.abspath(redef.file) or ixLine != redef.line:
                                self._addFileError(file, ixLine, "redefinition of '%s' type" % (curObject.name))
                        elif curObject.uri in self._auto_types:
                            redef = self._auto_types[curObject.uri]
                            if os.path.abspath(file) != os.path.abspath(redef.file) or ixLine != redef.line:
                                self._addFileError(file, ixLine, "redefinition of '%s' type" % (curObject.name))
                        else:
                            self._types[curObject.uri] = curObject
                    continue

                # Handle state definitions
                m = self._reState.search(line)
                if m:
                    object = m.group("object")
                    typeNSAbbrev = m.group("ns")
                    type = m.group("type")
                    bArray = (m.group("array") is not None)
                    name = m.group("name")

                    # Parse attributes
                    attrDict = {}
                    if m.group("attr") is not None:
                        attrDict = self._parseAttributes(file, ixLine, m.group("attr"), object)

                    # Add the comment doctext
                    self._addDoctext(file, ixLine, doctext, m)

                    # Namespace required
                    if curNamespace is None:
                        self._addFileError(file, ixLine, "namespace required for %s definition" % (object))
                        break

                    # Create the member type
                    (memberType, typeError) = self._createMemberType(file, ixLine, curNamespace, namespaces, typeNSAbbrev, type, bArray, attrDict)
                    if typeError is not None:
                        self._addFileError(file, ixLine, typeError)
                        continue

                    # Add the state
                    state = HSLState(file, ixLine, curNamespace, name, memberType, doctext = doctext)
                    doctext = []
                    if state.uri in self._states:
                        redef = self._states[state.uri]
                        if os.path.abspath(file) != os.path.abspath(redef.file) or ixLine != redef.line:
                            self._addFileError(file, ixLine, "redefinition of '%s' state" % (state.uri))
                    else:
                        self._states[state.uri] = state
                    continue

                # Handle action definitions
                m = self._reActionDef.search(line)
                if m:
                    # Add the comment doctext
                    self._addDoctext(file, ixLine, doctext, m)

                    # Set the current action struct
                    object = m.group("object")
                    if not isinstance(curObject, HSLAction):
                        self._addFileError(file, ixLine, "'%s' keyword only allowed within action definition" % (object))
                    else:
                        if object == "input":
                            curActionObject = curObject.inputMember.type
                        elif object == "output":
                            curActionObject = curObject.outputMember.type
                        elif object == "result":
                            curActionObject = curObject.resultEnum
                    continue

                # Handle action_state definitions
                m = self._reActionStateDef.search(line)
                if m:
                    # Add the comment doctext
                    self._addDoctext(file, ixLine, doctext, m)

                    # Set the current action struct
                    object = m.group("object")
                    if not isinstance(curObject, HSLActionState):
                        self._addFileError(file, ixLine, "'%s' keyword only allowed within action_state definition" % (object))
                    else: # "state"
                        curActionStateObjectMap = curObject.stateMembers
                    continue

                # Handle service definitions
                m = self._reServiceDef.search(line)
                if m:
                    # Add the comment doctext
                    self._addDoctext(file, ixLine, doctext, m)

                    # Set the current service struct
                    object = m.group("object")
                    if not isinstance(curObject, HSLService):
                        self._addFileError(file, ixLine, "'%s' keyword only allowed within service definition" % (object))
                    elif object == "actions":
                        curServiceObjectMap = curObject.actions
                    else: # "events"
                        curServiceObjectMap = curObject.events
                    continue

                # Handle member definitions
                m = self._reMember.search(line)
                if m:
                    typeNSAbbrev = m.group("ns")
                    type = m.group("type")
                    bArray = (m.group("array") is not None)
                    name = m.group("name")

                    # Parse attributes
                    attrDict = {}
                    if m.group("attr") is not None:
                        attrDict = self._parseAttributes(file, ixLine, m.group("attr"), "_member")
                    bOptional = "optional" in attrDict
                    bCSV = "csv" in attrDict
                    bErrorOutput = "error" in attrDict

                    # Add the comment doctext
                    self._addDoctext(file, ixLine, doctext, m)

                    # Get the parent struct
                    parentStruct = None
                    if isinstance(curObject, HSLAction) and curActionObject is not None and curActionObject.isStruct:
                        parentStruct = curActionObject
                        if bErrorOutput and curActionObject is not curObject.outputMember.type:
                            self._addFileError(file, ixLine, "attribute 'error' only valid for action output members")
                    elif isinstance(curObject, HSLType) and curObject.isStruct:
                        parentStruct = curObject
                        if bErrorOutput:
                            self._addFileError(file, ixLine, "attribute 'error' only valid for action output members")
                    elif isinstance(curObject, HSLEvent):
                        parentStruct = curObject.member.type
                        if bErrorOutput:
                            self._addFileError(file, ixLine, "attribute 'error' only valid for action output members")
                    if parentStruct is None:
                        self._addFileError(file, ixLine, "members only allowed with struct definition")
                        continue

                    # Create the member type
                    (memberType, typeError) = self._createMemberType(file, ixLine, curNamespace, namespaces, typeNSAbbrev, type, bArray, attrDict)
                    if typeError is not None:
                        self._addFileError(file, ixLine, typeError)
                        continue

                    # Recursive struct definitions are not allowed
                    if memberType.isStruct and memberType == parentStruct:
                        self._addFileError(file, ixLine, "recursive structure definitions not allowed")
                        continue

                    # Add the member
                    if len([member for member in parentStruct.members if name == member.name]) > 0:
                        self._addFileError(file, ixLine, "redefinition of '%s' member" % (name))
                        continue
                    elif bCSV:
                        if not memberType.validateCSV():
                            self._addFileError(file, ixLine, "member %s is not representable as a comma-seperated value" % (name))
                            continue

                    parentStruct.addMember(HSLMember(file, ixLine, parentStruct.namespace, name, memberType,
                                                     isOptional = bOptional, isCSV = bCSV,
                                                     isErrorOutput = bErrorOutput,
                                                     doctext = doctext))
                    doctext = []
                    continue

                # Handle value definitions
                m = self._reValue.search(line)
                if m:
                    value = m.group("value")

                    # Add the comment doctext
                    self._addDoctext(file, ixLine, doctext, m)

                    # Get the parent enum object
                    parentEnum = None
                    bResultEnum = False
                    if isinstance(curObject, HSLAction) and curActionObject is not None and curActionObject.isEnum:
                        parentEnum = curActionObject
                        bResultEnum = True
                    elif isinstance(curObject, HSLType) and curObject.isEnum:
                        parentEnum = curObject
                    if parentEnum is None:
                        self._addFileError(file, ixLine, "values only allowed with enum definition")
                        continue

                    # Add the enum value
                    if bResultEnum and (value == "OK" or value == "ERROR"):
                        self._addFileError(file, ixLine, "%s result is implicitly defined" % (value))
                    elif value in parentEnum.enumValues:
                        self._addFileError(file, ixLine, "redefinition of '%s' enum value" % (value))
                    elif bResultEnum and value != "REBOOT" and not value.startswith("ERROR_"):
                        self._addFileError(file, ixLine, "custom error results must begin with 'ERROR_'")
                    else:
                        parentEnum.addEnumValue(file, ixLine, value, doctext = doctext)
                        doctext = []
                    continue

                # Handle action_state state member definitions
                if isinstance(curObject, HSLActionState) and curActionStateObjectMap is curObject.stateMembers:

                    m = self._reReference.search(line)
                    if m:
                        stateNSAbbrev = m.group("ns")
                        name = m.group("name")

                        # Handle the attributes
                        attrDict = {}
                        if m.group("attr") is not None:
                            attrDict = self._parseAttributes(file, ixLine, m.group("attr"), "_action_state_state")
                        isGet = "get" in attrDict
                        isSet = "set" in attrDict
                        if not isGet and not isSet:
                            self._addFileError(file, ixLine, "state members must have 'get' and/or 'set' attribute")
                            continue

                        # Add the comment doctext
                        self._addDoctext(file, ixLine, doctext, m)

                        # Get the state namespace
                        stateNS = curNamespace
                        if stateNSAbbrev is not None:
                            if stateNSAbbrev not in namespaces:
                                self._addFileError(file, ixLine, "namespace '%s' not defined" % (stateNSAbbrev))
                                continue
                            stateNS = namespaces[stateNSAbbrev]

                        # Lookup the state
                        stateUri = HSLState(file, ixLine, stateNS, name, None).uri
                        if stateUri not in self._states:
                            self._addFileError(file, ixLine, "state '%s' not defined" % (name))
                            continue
                        state = self._states[stateUri]

                        # Add the state member
                        stateMember = HSLActionStateMember(file, ixLine, state, isGet, isSet)
                        if stateMember.uri in curActionStateObjectMap:
                            self._addFileError(file, ixLine, "multiple specification of '%s' state" % (name))
                        else:
                            curActionStateObjectMap[stateMember.uri] = stateMember
                            doctext = []
                        continue

                # Handle service action definitions
                if isinstance(curObject, HSLService) and curServiceObjectMap is curObject.actions:

                    m = self._reReference.search(line)
                    if m:
                        name = m.group("name")
                        actionNSAbbrev = m.group("ns")

                        # Handle the attributes
                        attrDict = {}
                        if m.group("attr") is not None:
                            attrDict = self._parseAttributes(file, ixLine, m.group("attr"), "_service_action")

                        # Add the comment doctext
                        self._addDoctext(file, ixLine, doctext, m)

                        # Get the action namespace
                        actionNS = curNamespace
                        if actionNSAbbrev is not None:
                            if actionNSAbbrev not in namespaces:
                                self._addFileError(file, ixLine, "namespace '%s' not defined" % (actionNSAbbrev))
                                continue
                            actionNS = namespaces[actionNSAbbrev]

                        # Add the action reference
                        actionUri = HSLAction(file, ixLine, actionNS, name, None).uri
                        if actionUri in curServiceObjectMap:
                            self._addFileError(file, ixLine, "multiple specification of action '%s'" % (name))
                        else:
                            curServiceObjectMap[actionUri] = None
                            doctext = []
                        continue

                # Handle service event definitions
                if isinstance(curObject, HSLService) and curServiceObjectMap is curObject.events:

                    m = self._reReference.search(line)
                    if m:
                        name = m.group("name")
                        eventNSAbbrev = m.group("ns")

                        # Handle the attributes
                        attrDict = {}
                        if m.group("attr") is not None:
                            attrDict = self._parseAttributes(file, ixLine, m.group("attr"), "_service_event")

                        # Add the comment doctext
                        self._addDoctext(file, ixLine, doctext, m)

                        # Get the action namespace
                        eventNS = curNamespace
                        if eventNSAbbrev is not None:
                            if eventNSAbbrev not in namespaces:
                                self._addFileError(file, ixLine, "namespace '%s' not defined" % (eventNSAbbrev))
                                continue
                            eventNS = namespaces[eventNSAbbrev]

                        # Add the action reference
                        eventUri = HSLEvent(file, ixLine, eventNS, name, None).uri
                        if eventUri in curServiceObjectMap:
                            self._addFileError(file, ixLine, "multiple specification of event '%s'" % (name))
                        else:
                            curServiceObjectMap[eventUri] = None
                            doctext = []
                        continue

                # Generic syntax error
                self._addFileError(file, ixLine, "syntax error")

        except:
            self._addFileError(file, ixLine, "an unexpected error occurred: %s" % (traceback.format_exc()))

        # End-of-file line continuations are not allowed
        if lineContinuation is not None:
                self._addFileError(file, 1, "End-of-file line continuation")

        # Empty enumerations are not allowed
        for enum in enums:
            if not enum.enumValues:
                self._addFileError(file, enum.line, "enum '%s' is empty" % (enum.name))

    #
    # HSL entity filtering (inclusively or exclusively)
    #

    def filterActions(self, actionNames, bExclude):

        self._filterEntities("Action", "actions", actionNames, bExclude)

    def filterEvents(self, eventNames, bExclude):

        self._filterEntities("Event", "events", eventNames, bExclude)

    def _filterEntities(self, entityTitle, entitiesAttr, entityNames, bExclude):

        # Build the new entities map
        entities = getattr(self, "_" + entitiesAttr)
        entities_new = {}
        matched = {}
        for entity in entities.itervalues():
            if entity.name in entityNames or entity.uri in entityNames:
                if not bExclude:
                    entities_new[entity.uri] = entity
                matched[entity.name] = entity
                matched[entity.uri] = entity
            elif bExclude:
                entities_new[entity.uri] = entity

        # Compute the removed entities
        removed_entities = {}
        for entity in entities.itervalues():
            if entity not in entities_new.itervalues():
                removed_entities[entity.uri] = entity

        # Remove services that contain references to removed entities
        services = {}
        for service in self._services.itervalues():
            bRemoveService = False
            for entity_uri in getattr(service, entitiesAttr):
                if entity_uri in removed_entities:
                    bRemoveService = True
                    break
            if not bRemoveService:
                services[service.uri] = service

        # Report unmatched entity names
        for entityName in entityNames:
            if entityName not in matched:
                self._addError("%s '%s' not found" % (entityTitle, entityName))

        # Update the entities and services collections
        setattr(self, "_" + entitiesAttr, entities_new)
        self._services = services
