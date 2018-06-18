#!/usr/bin/python
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

import re

from hslparser import HSLParser, HSLMember, HSLState
from codeutil import makeGlobalSymbol, makeSymbol


#
# HDK module action location class
#
class HDKActionLocation:

    def __init__(self, http_method, http_location, action, noauth):

        self.http_method = http_method
        self.http_location = http_location
        self.action = action
        self.noauth = noauth


#
# HDK module class
#
class HDKModule:

    # Class initializer
    def __init__(self, model, baseName, actionLocation, bIncludeStructSchemas):

        self.baseName = baseName
        self.actions = model.actions
        self.events = model.events
        self.services = model.services
        self.states = model.states
        self.schema = Schema(model, bIncludeStructSchemas)

        # Compute the action locations
        self._action_locs = []
        for action in self.actions:

            # Authenticated HNAP methods
            self._action_locs.append(HDKActionLocation("POST", actionLocation, action, action.isNoAuth))

            # HNAP GET
            if (action.namespace, action.name) == HSLParser.HNAPGetDeviceSettings:
                self._action_locs.append(HDKActionLocation("GET", actionLocation, action, True))


    # Module filename
    def filename(self, suffix):

        return self.baseName.lower() + suffix

    # Header file sentinel
    def header_sentinel(self, suffix):

        return makeSymbol(self.filename(suffix).upper(), prefix = "__", suffix = "__")

    # Export macro
    def export_macro(self):

        return makeSymbol(self.baseName.upper(), suffix = "_EXPORT")

    # Build macro
    def build_macro(self):

        return makeSymbol(self.baseName.upper(), suffix = "_BUILD")

    # Element enumeration
    def element_enum(self):

        return makeSymbol(self.baseName.upper(), suffix = '_Element')

    # Element value
    def element_value(self, element):

        return makeGlobalSymbol(self.element_enum(), element[0], element[1])

    # Enum enum
    def enum_enum(self, type):

        return makeGlobalSymbol(self.baseName.upper() + "_Enum", type.namespace, type.name)

    # Enum value
    def enum_value(self, type, value, is_unknown = False):

        if is_unknown:
            return self.enum_enum(type) + "__UNKNOWN__"
        else:
            return self.enum_enum(type) + "_" + re.sub("[^A-Za-z0-9]+", "_", value[:1].upper() + value[1:])

    # Enum type enum
    def enum_type_enum(self):

        return makeSymbol(self.baseName.upper(), suffix = "_EnumType")

    # Enum type value
    def enum_type_value(self, type):

        return makeGlobalSymbol(self.enum_type_enum(), type.namespace, type.name)

    # Enum type string array
    def enum_type_strings_static(self, type):

        return makeGlobalSymbol("s_enum", type.namespace, type.name)

    # Enum type accessor
    def enum_accessor(self, type, fn):

        return makeGlobalSymbol(self.baseName.upper() + '_' + fn, type.namespace, type.name)

    # Type enumeration value
    def type_value(self, type):

        if not type or type.isStruct:
            return "HDK_XML_BuiltinType_Struct"
        elif type.isEnum:
            return self.enum_type_value(type)
        elif type.isBuiltin and type.name == "datetime":
            return "HDK_XML_BuiltinType_DateTime"
        else:
            return "HDK_XML_BuiltinType_" + type.name[:1].upper() + type.name[1:]

    # Action enum
    def action_enum(self):

        return makeSymbol(self.baseName.upper(), suffix = '_MethodEnum')

    # Action enum value
    def action_value(self, action_loc):

        # Non-post action?
        if action_loc.http_method != "POST":
            suffix = "_" + action_loc.http_method
        else:
            suffix = ""

        return makeGlobalSymbol(self.action_enum() + suffix, action_loc.action.namespace, action_loc.action.name)

    # Event enum
    def event_enum(self):

        return self.baseName.upper() + '_EventEnum'

    # Event enum value
    def event_value(self, event):

        return makeGlobalSymbol(self.event_enum(), event.namespace, event.name)

    # Action function sentinel
    def action_sentinel(self, action):

        return "__" + self.action_fn(action).upper() + "__"

    # Action function
    def action_fn(self, action):

        return makeGlobalSymbol(self.baseName.upper() + '_Method', action.namespace, action.name)

    # Action schema nodes static
    def schema_nodes_static(self, hsl, suffix = ""):

        return makeGlobalSymbol('s_schemaNodes', hsl.namespace, hsl.name) + suffix

    # Action schema static
    def schema_static(self, hsl, suffix = ""):

        return makeGlobalSymbol('s_schema', hsl.namespace, hsl.name) + suffix

    # Action schema element path static
    def schema_element_path_static(self, hsl, suffix = ""):

        return makeGlobalSymbol('s_elementPath', hsl.namespace, hsl.name) + suffix

    # DOM (struct) schema function
    def dom_schema_fn(self, struct):

        return makeGlobalSymbol(self.baseName.upper() + '_Schema', struct.namespace, struct.name)

    # State enum
    def state_enum(self):

        return makeSymbol(self.baseName.upper(), suffix = '_ADI')

    # State enum value
    def state_value(self, state):

        return makeGlobalSymbol(self.state_enum(), state.namespace, state.name)

    # State value sentinel
    def state_value_sentinel(self, state, usage):

        return "__" + makeGlobalSymbol(self.state_enum() + usage, state.namespace, state.name).upper() + "__"

    # State enum
    def module_fn(self):

        return makeSymbol(self.baseName.upper(), suffix = '_Module')

    # Service actions static
    def service_actions_name(self, service):

        return makeGlobalSymbol('s_service', service.uri, "Methods")

    # Service events static
    def service_events_name(self, service):

        return makeGlobalSymbol('s_service', service.uri, "Events")

    # Get the list of action location objects
    def action_locations(self):
        return self._action_locs


#
# Schema node class
#
class SchemaNode:

    # Class initializer
    def __init__(self, parent, hsl, namespace = None, name = None, sibling_order = 0,
                 is_optional = False, is_any_element = False, is_error_element = False):

        # Schema node parent node
        self.parent = parent

        # Determine the HSL type
        if isinstance(hsl, HSLMember):
            self.type = hsl.type
            self.is_optional = is_optional or hsl.isOptional
            self.is_unbounded = hsl.isUnbounded
            self.is_error = is_error_element or hsl.isErrorOutput
            self.is_any_element = is_any_element
            self.is_csv = hsl.isCSV
        elif isinstance(hsl, HSLState): # ADI schema node
            self.type = hsl.type
            self.is_optional = True
            self.is_unbounded = False
            self.is_error = False
            self.is_any_element = is_any_element
            self.is_csv = False
        else:
            self.type = hsl
            self.is_optional = is_optional
            self.is_unbounded = False
            self.is_error = is_error_element
            self.is_any_element = is_any_element
            self.is_csv = False

        # Determine the child members
        if self.type and self.type.isStruct:
            self._members = self.type.members
        else:
            self._members = []

        # Element namespace
        if namespace:
            self.namespace = namespace
        else:
            self.namespace = hsl.namespace

        # Element name
        if name:
            self.name = name
        else:
            self.name = hsl.name

        # Sibling order
        self.sibling_order = sibling_order

    # Get the child schema nodes
    def children(self, child_inherits_is_error = True):

        result = []
        sibling_order = 0
        for member in self._members:
            childNode = SchemaNode(self, member, sibling_order = sibling_order,
                                   is_error_element = child_inherits_is_error and self.is_error)
            sibling_order += 1
            result.append(childNode)
            result.extend(childNode.children())
        return result

    # The node element
    def element(self):

        return (self.namespace, self.name)


#
# Schema class
#
class Schema:

    # Class initializer
    def __init__(self, model, bIncludeStructSchemas = False):

        self.namespaces = []
        self.elements = []
        self.enums = []
        self.structs = []

        self._elements = {}
        self._namespaces = {}
        self._enums = {}
        self._states = model.states

        # Structs
        if bIncludeStructSchemas:
            for type in model.types:
                if type.isStruct:
                    self.structs.append(type)

        # Get the action elements
        for action in model.actions:
            for node in self.struct_nodes(action.inputMember, soap = True):
                self._elements[(node.namespace, node.name)] = None
                if node.type and node.type.isEnum:
                    self._enums[node.type.uri] = node.type
            for node in self.struct_nodes(action.outputMember, soap = True):
                self._elements[(node.namespace, node.name)] = None
                if node.type and node.type.isEnum:
                    self._enums[node.type.uri] = node.type

        # Get the event elements
        for event in model.events:
            for node in self.struct_nodes(event.member):
                self._elements[(node.namespace, node.name)] = None
                if node.type and node.type.isEnum:
                    self._enums[node.type.uri] = node.type

        # Get the state elements
        for node in self.state_nodes():
            self._elements[(node.namespace, node.name)] = None
            if node.type and node.type.isEnum:
                self._enums[node.type.uri] = node.type

        # Get the struct elements
        for struct in self.structs:
            for node in self.struct_nodes(struct):
                self._elements[(node.namespace, node.name)] = None
                if node.type and node.type.isEnum:
                    self._enums[node.type.uri] = node.type

        # Create the sorted elements
        self.elements = sorted(self._elements.iterkeys())
        ix = 0
        for element in self.elements:
            self._namespaces[element[0]] = None
            self._elements[element] = ix
            ix += 1

        # Create the sorted namespaces
        self.namespaces = sorted(self._namespaces.iterkeys())
        ix = 0
        for namespace in self.namespaces:
            self._namespaces[namespace] = ix
            ix += 1

        # Create the sorted enum types
        self.enums = sorted(self._enums.itervalues())
        ix = -1
        for enum in self.enums:
            self._enums[enum.uri] = ix
            ix -= 1

    # Namespace index
    def namespace_index(self, namespace):

        return self._namespaces[namespace]

    # Namespace index
    def element_index(self, element):

        return self._elements[element]

    # Enum type index
    def enum_index(self, enum):

        return self._enums[enum.uri]

    # Schema node sort key
    def _node_key(self, node):

        key = []
        while node:
            key.append(node.element())
            key.append(node.sibling_order)
            node = node.parent
        key.append(len(key))
        key.reverse()
        return key

    # Struct schema nodes
    def struct_nodes(self, struct, soap = False):

        # Add the struct's schema nodes
        nodes, envelope_node, struct_node, element_path = self._struct_nodes_envelope(struct, soap = soap)
        nodes.append(struct_node)
        nodes.extend(struct_node.children(child_inherits_is_error = False))

        return sorted(nodes, key = self._node_key)

    # Action schema nodes envelope helper
    def struct_nodes_envelope(self, struct, soap = False):

        return self._struct_nodes_envelope(struct, soap)[3]

    # Action schema nodes envelope helper
    def _struct_nodes_envelope(self, struct, soap = False):

        # Create the envelope
        if soap:
            soap_ns = "http://schemas.xmlsoap.org/soap/envelope/"
            soap_envelope = SchemaNode(None, None, namespace = soap_ns, name = "Envelope",
                                       is_any_element = True, is_error_element = True)
            soap_header = SchemaNode(soap_envelope, None, namespace = soap_ns, name = "Header",
                                    is_any_element = True, is_optional = True, sibling_order = 0)
            soap_body = SchemaNode(soap_envelope, None, namespace = soap_ns, name = "Body",
                                   is_error_element = True, sibling_order = 1)
            envelope_nodes = [ soap_envelope, soap_header, soap_body ]
            envelope_node = soap_body
            element_path = [ soap_body.element() ]
        else:
            envelope_nodes = []
            envelope_node = None
            element_path = None

        # Build the element path
        struct_node = SchemaNode(envelope_node, struct)
        if element_path:
            element_path.append(struct_node.element())

        return (envelope_nodes, envelope_node, struct_node, element_path)

    # States schema nodes
    def state_nodes(self):

        # Add the nodes
        nodes = []
        if self._states:
            parent = SchemaNode(None, None, namespace = HSLParser.HNAPNamespace, name = "ADI")
            nodes.append(parent)
            sibling_order = 0
            for state in self._states:
                node = SchemaNode(parent, state, sibling_order = sibling_order)
                sibling_order += 1
                nodes.append(node)
                nodes.extend(node.children())

        return sorted(nodes, key = self._node_key)
