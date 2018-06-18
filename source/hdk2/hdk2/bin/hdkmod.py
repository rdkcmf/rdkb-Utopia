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

import optparse
import os
import re
import sys
import traceback
import uuid

sys.path.append(os.path.join(os.path.dirname(sys.argv[0]), "lib"))
from hdk.hslparser import HSLParser
from hdk.modutil import HDKModule


#
# Main
#
def main():

    # Command line options
    cmdParser = optparse.OptionParser("usage: %prog [options] hsl...")
    cmdParser.add_option("-o", action = "store", type = "string", dest = "outputDir", default = ".",
                         help = "output directory", metavar = "DIR")
    cmdParser.add_option("-b", action = "store", type = "string", dest = "baseName", default = "hdk",
                         help = "base file name (e.g. 'hdk')", metavar = "NAME")
    cmdParser.add_option("-I", action = "append", dest = "importDirs", default = [],
                         help = "add directory to import path", metavar = "DIR")
    cmdParser.add_option("--action-include", action = "append", dest = "actionsInclusive",
                         help = "include only specified actions", metavar = "ACTION")
    cmdParser.add_option("--action-exclude", action = "append", dest = "actionsExclusive",
                         help = "exclude specified actions", metavar = "ACTION")
    cmdParser.add_option("--event-include", action = "append", dest = "eventsInclusive",
                         help = "include only specified events", metavar = "EVENT")
    cmdParser.add_option("--event-exclude", action = "append", dest = "eventsExclusive",
                         help = "exclude specified events", metavar = "EVENT")
    cmdParser.add_option("--noid", action = "store", dest = "noid", default = None,
                         help = "network object identifier (optional)")
    cmdParser.add_option("--friendly-name", action = "store", dest = "friendly_name", default = None,
                         help = "network object friendly name (optional)", metavar = "NAME")
    cmdParser.add_option("--client", action = "store_true", dest = "client", default = False,
                         help = "generate client module"),
    cmdParser.add_option("--noauth", action = "store_true", dest = "noauth", default = False,
                         help = "set noauth attribute for all module actions"),
    cmdParser.add_option("--action-location", action = "store", type = "string", dest = "actionLocation", default = "/HNAP1",
                         help = "specify the module actions HTTP location", metavar = "PATH")
    cmdParser.add_option("--dom-schema", action = "store_true", dest = "dom_schema", default = False,
                         help = "generate DOM schemas for user-defined structs")
    cmdParser.add_option("--server-methods", action = "store_true", dest = "server_methods", default = False,
                         help = "generate server method implementation file"),
    cmdParser.add_option("--adi-report", action = "store_true", dest = "adi_report", default = False,
                         help = "generate module ADI report"),

    # Parse command line options
    (cmdOptions, cmdArgs) = cmdParser.parse_args()
    if cmdArgs is None or not cmdArgs:
        cmdParser.error("Must specify at least one HSL file")

    # Check we have a valid UUID.
    noid = None
    if cmdOptions.noid is not None:
        noid = uuid.UUID(cmdOptions.noid)

    # Parse HSL files
    parser = HSLParser(importPath = cmdOptions.importDirs,
                       noauth = cmdOptions.noauth)
    for file in cmdArgs:
        parser.parseFile(file)
    parser.parseDone()
    if parser.errors:
        for error in parser.errors:
            print >> sys.stderr, error
        sys.exit(1)

    # Include/exclude actions and events
    if cmdOptions.actionsInclusive is not None:
        parser.filterActions(cmdOptions.actionsInclusive, False)
    if cmdOptions.actionsExclusive is not None:
        parser.filterActions(cmdOptions.actionsExclusive, True)
    if cmdOptions.eventsInclusive is not None:
        parser.filterEvents(cmdOptions.eventsInclusive, False)
    if cmdOptions.eventsExclusive is not None:
        parser.filterEvents(cmdOptions.eventsExclusive, True)
    if parser.errors:
        for error in parser.errors:
            print >> sys.stderr, error
        sys.exit(1)

    # Get the parsed actions, types and services
    model = parser.getmodel()
    if not model.actions and not cmdOptions.dom_schema:
        print >> sys.stderr, "Error: No actions defined"
        sys.exit(1)

    # Ensure the output directory exists
    if not os.path.isdir(cmdOptions.outputDir):
        try:
            os.makedirs(cmdOptions.outputDir)
        except:
            print >> sys.stderr, "Error: Couldn't create directory '%s'" % cmdOptions.outputDir
            sys.exit(1)

    # Generate code
    try:
        gen = ModuleGenerator(model, cmdOptions.baseName, noid, cmdOptions.friendly_name,
                              cmdOptions.actionLocation, cmdOptions.dom_schema,
                              cmdOptions.server_methods, cmdOptions.client, cmdOptions.adi_report)
        gen.generate(cmdOptions.outputDir, sys.stdout)
    except:
        print >> sys.stderr, "Error: An unexpected exception occurred generating code:"
        traceback.print_exc()
        sys.exit(1)


#
# HDK module code generation
#
class ModuleGenerator:

    # Class initializer
    def __init__(self, model, baseName, noid, friendlyName,
                 actionLocation, bGenerateDOMSchemas,
                 bGenerateMethods, bClient, bADIReport):

        self._model = model

        # Options
        self._baseName = baseName
        self._noid = noid
        self._friendlyName = friendlyName
        self._bGenerateMethods = bGenerateMethods
        self._bADIReport = bADIReport

        # Generator state
        self._out = None

        # Module
        self._module = HDKModule(model, baseName, actionLocation, bGenerateDOMSchemas)

        # Generation options
        self._bGenerateActions = not bClient and self._module.actions
        self._bGenerateServices = not bClient and self._module.services
        self._bGenerateModule = self._module.actions or self._module.events
        self._bGenerateModuleDynamic = not bClient and self._bGenerateModule

    #
    # Generate code
    #

    def generate(self, dir, fhReport):

        for file, fn, bGenerate in \
                [(os.path.join(dir, self._module.filename('.h')), self._generate_h, not self._bGenerateMethods),
                 (os.path.join(dir, self._module.filename('.c')), self._generate_c, not self._bGenerateMethods),
                 (os.path.join(dir, self._module.filename('_methods.c')), self._generate_methods_c, self._bGenerateMethods),
                 (os.path.join(dir, self._module.filename('_adi.txt')), self._generate_adi_txt, self._bADIReport)]:

            if bGenerate:

                if fhReport is not None:
                    print >>fhReport, 'Generating "%s" ...' % file

                self._out = open(file, "w")
                try:
                    fn()
                except:
                    raise
                finally:
                    self._out.close()


    #
    # Output helper methods
    #

    # Output
    def _write(self, s):

        self._out.write(s)

    # Write .c file header
    def _write_header_c(self):

        self._write('''\
/*
 * Copyright 2014 Cisco Systems, Inc.
 * Licensed under the Apache License, Version 2.0 
 */
''')

    # Write .h file header
    def _write_header_h(self):

        self._write_header_c();
        self._write('''\

#ifndef %s
#define %s
''' % (self._module.header_sentinel('.h'), self._module.header_sentinel('.h')))

    # Write .h file footer
    def _write_footer_h(self):

        self._write('''\

#endif /* %s */
''' % (self._module.header_sentinel('.h')))

    # Write a section comment
    def _write_section(self, comment):

            self._write('''\


/*
 * %s
 */

''' % (comment))

    # Write enumeration definition
    def _write_enumeration(self, name, values, ix_start = 0, ix_delta = 1, value_symbols = {}):

        # Enum header
        self._write('''\
typedef enum _%s
{
''' % (name))

        # Write enum values
        ix_value = ix_start
        ix_value_last = ix_start + ix_delta * (len(values) - 1)
        sep = ","
        for value in values:

            # Does the value have a symbol?
            if value_symbols.has_key(value):
                str_value = value_symbols[value]
            else:
                str_value = str(ix_value)

            # Don't separate the last value
            if ix_value == ix_value_last:
                sep = ""

            # Write the enum value
            self._write('''\
    %s = %s%s
''' % (value, str_value, sep))

            ix_value += ix_delta

        # Enum footer
        self._write('''\
} %s;
''' % (name))

    # Write schema definition
    def _write_schema(self, schema_nodes_static, schema_static, nodes,
                      element_path_static = None, element_path = None):

        # Schema nodes header
        self._write('''\
static const HDK_XML_SchemaNode %s[] =
{
''' % (schema_nodes_static))

        # Write the schema nodes
        ix = 0
        for node in nodes:

            # Compute the parent index
            if node.parent:
                ix_parent = nodes.index(node.parent)
            else:
                ix_parent = 0

            # Compute the options
            options = []
            if node.is_optional:
                options.append("HDK_XML_SchemaNodeProperty_Optional")
            if node.is_unbounded:
                options.append("HDK_XML_SchemaNodeProperty_Unbounded")
            if node.is_any_element:
                options.append("HDK_XML_SchemaNodeProperty_AnyElement")
            if node.is_error:
                options.append("HDK_XML_SchemaNodeProperty_ErrorOutput")
            if node.is_csv:
                options.append("HDK_XML_SchemaNodeProperty_CSV")
            if options:
                options = " | ".join(options)
            else:
                options = "0"

            # Write the schema node
            self._write('''\
    /* %d */ { %d, %s, %s, %s },
''' % (ix, ix_parent,
       self._module.element_value(node.element()),
       self._module.type_value(node.type),
       options))
            ix += 1

        # Schema nodes footer
        self._write('''\
    HDK_XML_Schema_SchemaNodesEnd
};
''')

        # Schema struct element path
        if element_path_static and element_path:

            self._write('''\

static const HDK_XML_Element %s[] =
{
''' % (element_path_static))
            for element in element_path:
                self._write('''\
    %s,
''' % (self._module.element_value(element)))
            self._write('''\
    HDK_MOD_ElementPathEnd
};
''')

        # Schema
        self._write('''\

static const HDK_XML_Schema %s =
{
    s_namespaces,
    s_elements,
    %s,
    %s
};
''' % (schema_static,
       schema_nodes_static,
       "s_enumTypes" if self._module.schema.enums else "0"))

    # Write action definition
    def _write_action(self, action_loc):

        http_method, http_location, action, noauth = \
            (action_loc.http_method, action_loc.http_location, action_loc.action, action_loc.noauth)

        # Compute the SOAP action
        is_get = (http_method.lower() == "get")
        if is_get:
            soap_action = "0"
        else:
            soap_action = '"' + action.uri + '"'

        # Compute the action function name
        if self._bGenerateActions:
            action_fn = self._module.action_fn(action)
        else:
            action_fn = "0"

        # Compute the schema struct element path symbols
        if not is_get and self._module.schema.struct_nodes_envelope(action.inputMember, soap = True):
            input_element_path = self._module.schema_element_path_static(action, "_Input")
        else:
            input_element_path = "0"
        if self._module.schema.struct_nodes_envelope(action.outputMember, soap = True):
            output_element_path = self._module.schema_element_path_static(action, "_Output")
        else:
            output_element_path = "0"

        # Compute the method options
        options = []
        if noauth:
            options.append("HDK_MOD_MethodOption_NoBasicAuth")
        if is_get:
            options.append("HDK_MOD_MethodOption_NoInputStruct")
        if options:
            options = " | ".join(options)
        else:
            options = "0"

        # Compute the OK and REBOOT values
        result_member = action.outputMember.type.members[0]
        if "REBOOT" in action.resultEnum.enumValues:
            result_reboot = "REBOOT"
        else:
            result_reboot = "OK"

        # Write the method struct
        self._write('''\
    {
        "%s",
        "%s",
        %s,
        %s,
        &%s,
        &%s,
        %s,
        %s,
        %s,
        %s,
        %s,
        %s,
        %s
    },
''' % (http_method,
       http_location,
       soap_action,
       action_fn,
       self._module.schema_static(action, suffix = "_Input"),
       self._module.schema_static(action, suffix = "_Output"),
       input_element_path,
       output_element_path,
       options,
       self._module.element_value((result_member.namespace, result_member.name)),
       self._module.type_value(result_member.type),
       self._module.enum_value(result_member.type, "OK"),
       self._module.enum_value(result_member.type, result_reboot)))

    # Write event definition
    def _write_event(self, event):

        # Write the method struct
        self._write('''\
    {
        "%s",
        &%s
    },
''' % (event.uri,
       self._module.schema_static(event)))


    #
    # *.h code generator
    #

    def _generate_h(self):

        # File header
        self._write_header_h()

        # Includes
        self._write('''\

#include "hdk_mod.h"


/*
 * Macro to control public exports
 */

#ifdef __cplusplus
#  define %s_PREFIX extern "C"
#else
#  define %s_PREFIX extern
#endif
#ifdef HDK_MOD_STATIC
#  define %s %s_PREFIX
#else /* ndef HDK_MOD_STATIC */
#  ifdef _MSC_VER
#    ifdef %s
#      define %s %s_PREFIX __declspec(dllexport)
#    else
#      define %s %s_PREFIX __declspec(dllimport)
#    endif
#  else /* ndef _MSC_VER */
#    ifdef %s
#      define %s %s_PREFIX __attribute__ ((visibility("default")))
#    else
#      define %s %s_PREFIX
#    endif
#  endif /*def _MSC_VER */
#endif /* def HDK_MOD_STATIC */
''' % (self._module.export_macro(),
       self._module.export_macro(),
       self._module.export_macro(), self._module.export_macro(),
       self._module.build_macro(),
       self._module.export_macro(), self._module.export_macro(),
       self._module.export_macro(), self._module.export_macro(),
       self._module.build_macro(),
       self._module.export_macro(), self._module.export_macro(),
       self._module.export_macro(), self._module.export_macro()))

        # Element enumeration
        self._write_section("Elements")
        self._write_enumeration(self._module.element_enum(),
                                [ self._module.element_value(element) for element in self._module.schema.elements ])

        # Enumeration type definition
        if self._module.schema.enums:

            self._write_section("Enum types enumeration")
            self._write_enumeration(self._module.enum_type_enum(),
                                    [ self._module.enum_type_value(enum) for enum in self._module.schema.enums ],
                                    ix_start = -1, ix_delta = -1)

        # Enumeration declarations
        for enum in self._module.schema.enums:

            # Enumeration type definition
            self._write_section("Enumeration %s" % (enum.uri))
            value_unknown = self._module.enum_value(enum, None, is_unknown = True)
            values = [ value_unknown ]
            values.extend([ self._module.enum_value(enum, value) for value in enum.enumValues ])
            self._write_enumeration(self._module.enum_enum(enum), values, ix_start = -1,
                                    value_symbols = { value_unknown: "HDK_XML_Enum_Unknown" })

            # Enumeration accessor declarations
            self._write('''\

#define %s(pStruct, element, value) HDK_XML_Set_Enum(pStruct, element, %s, 0 ? %s : (value))
#define %s(pStruct, element, value) HDK_XML_Append_Enum(pStruct, element, %s, 0 ? %s : (value))
#define %s(pStruct, element) (%s*)HDK_XML_Get_Enum(pStruct, element, %s)
#define %s(pStruct, element, value) (%s)HDK_XML_GetEx_Enum(pStruct, element, %s, 0 ? %s : (value))
#define %s(pMember) (%s*)HDK_XML_GetMember_Enum(pMember, %s)
''' % (self._module.enum_accessor(enum, "Set"), self._module.enum_type_value(enum), self._module.enum_value(enum, enum.enumValues[0]),
       self._module.enum_accessor(enum, "Append"), self._module.enum_type_value(enum), self._module.enum_value(enum, enum.enumValues[0]),
       self._module.enum_accessor(enum, "Get"), self._module.enum_enum(enum), self._module.enum_type_value(enum),
       self._module.enum_accessor(enum, "GetEx"), self._module.enum_enum(enum), self._module.enum_type_value(enum), self._module.enum_value(enum, enum.enumValues[0]),
       self._module.enum_accessor(enum, "GetMember"), self._module.enum_enum(enum), self._module.enum_type_value(enum)))

        # Action enumeration
        if self._module.actions:
            self._write_section("Method enumeration")
            self._write_enumeration(self._module.action_enum(),
                                    [ self._module.action_value(action_loc) for action_loc in self._module.action_locations() ])

        # Action sentinels
        if self._bGenerateActions:
            self._write_section("Method sentinels")
            for action in self._module.actions:
                self._write('''\
#define %s
''' % (self._module.action_sentinel(action)))

        # Action declarations
        if self._bGenerateActions:
            self._write_section("Methods")
            for action in self._module.actions:
                self._write('''\
extern void %s(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput);
''' % (self._module.action_fn(action)))

        # Event enumeration
        if self._module.events:
            self._write_section("Event enumeration")
            self._write_enumeration(self._module.event_enum(),
                                    [ self._module.event_value(event) for event in self._module.events ])

        # DOM (struct) schema accessor declarations
        if self._module.schema.structs:
            self._write_section("DOM Schemas")
        for struct in self._module.schema.structs:
            self._write('''\
%s const HDK_XML_Schema* %s();
''' % (self._module.export_macro(), self._module.dom_schema_fn(struct)))

        # State declarations
        if self._module.states:

            self._write_section("ADI")
            self._write_enumeration(self._module.state_enum(),
                                    [ self._module.state_value(state) for state in self._module.states ],
                                    ix_start = 1)

            self._write_section("ADI sentinels")
            for state in self._module.states:

                # Determine get/set
                usages = {}
                for actionState in self._model.actionStates:
                    if state.uri in actionState.stateMembers:
                        stateMember = actionState.stateMembers[state.uri]
                        if stateMember.isGet:
                            usages["get"] = None
                        if stateMember.isSet:
                            usages["set"] = None

                for usage in sorted(usages.iterkeys()):
                    self._write('''\
#define %s
''' % (self._module.state_value_sentinel(state, usage)))

        # Module declaration
        if self._bGenerateModule:

            self._write_section("Module")
            self._write('''\
%s const HDK_MOD_Module* %s(void);
''' % (self._module.export_macro(), self._module.module_fn()))

            if self._bGenerateModuleDynamic:
                self._write('''\

/* Dynamic server module export */
%s const HDK_MOD_Module* HDK_SRV_Module(void);
''' % (self._module.export_macro()))

        # File footer
        self._write_footer_h()


    #
    # *.c code generator
    #

    def _generate_c(self):

        # File header
        self._write_header_c()
        self._write('''\

#include "%s"

#include <string.h>
''' % (self._module.filename('.h')))

        # Namespace table
        self._write_section("Namespaces")
        self._write('''\
static const HDK_XML_Namespace s_namespaces[] =
{
''')
        for namespace in self._module.schema.namespaces:
            self._write('''\
    /* %d */ "%s",
''' % (self._module.schema.namespace_index(namespace),
       namespace))
        self._write('''\
    HDK_XML_Schema_NamespacesEnd
};
''')

        # Elements table
        self._write_section("Elements")
        self._write('''\
static const HDK_XML_ElementNode s_elements[] =
{
''')
        for element in self._module.schema.elements:
            self._write('''\
    /* %s = %d */ { %d, "%s" },
''' % (self._module.element_value(element),
       self._module.schema.element_index(element),
       self._module.schema.namespace_index(element[0]),
       element[1]))
        self._write('''\
    HDK_XML_Schema_ElementsEnd
};
''')

        # Enumerations definitions
        for enum in self._module.schema.enums:

            # Enumeration string table
            self._write_section("Enumeration %s" % (enum.uri))
            self._write('''\
static const HDK_XML_EnumValue %s[] =
{
''' % (self._module.enum_type_strings_static(enum)))
            for value in enum.enumValues:
                self._write('''\
    "%s",
''' % (value))
            self._write('''\
    HDK_XML_Schema_EnumTypeValuesEnd
};
''')

        # Enum types array
        if self._module.schema.enums:

            self._write_section("Enumeration types array")
            self._write('''\
static const HDK_XML_EnumType s_enumTypes[] =
{
''')
            sep = ','
            for enum in self._module.schema.enums:
                if enum is self._module.schema.enums[-1]:
                    sep = ""
                self._write('''\
    %s%s
''' % (self._module.enum_type_strings_static(enum), sep))
            self._write('''\
};
''')

        # Action definitions
        for action in self._module.actions:

            self._write_section("Method %s" % (action.uri))

            # Input schema
            self._write_schema(self._module.schema_nodes_static(action, suffix = "_Input"),
                               self._module.schema_static(action, suffix = "_Input"),
                               self._module.schema.struct_nodes(action.inputMember, soap = True),
                               element_path_static = self._module.schema_element_path_static(action, suffix = "_Input"),
                               element_path = self._module.schema.struct_nodes_envelope(action.inputMember, soap = True))

            # Output schema
            self._write('''\

''')
            self._write_schema(self._module.schema_nodes_static(action, suffix = "_Output"),
                               self._module.schema_static(action, suffix = "_Output"),
                               self._module.schema.struct_nodes(action.outputMember, soap = True),
                               element_path_static = self._module.schema_element_path_static(action, suffix = "_Output"),
                               element_path = self._module.schema.struct_nodes_envelope(action.outputMember, soap = True))

        # Actions table
        if self._module.actions:

            self._write_section("Methods")

            # Actions table header
            self._write('''\
static const HDK_MOD_Method s_methods[] =
{
''')

            # Write each action node
            for action_loc in self._module.action_locations():
                self._write_action(action_loc)

            # Actions table footer
            self._write('''\
    HDK_MOD_MethodsEnd
};
''')

        # Event definitions
        for event in self._module.events:

            self._write_section("Event %s" % (event.uri))
            self._write_schema(self._module.schema_nodes_static(event),
                               self._module.schema_static(event),
                               self._module.schema.struct_nodes(event.member))

        # Events table
        if self._module.events:

            self._write_section("Events")

            # Events table header
            self._write('''\
static const HDK_MOD_Event s_events[] =
{
''')

            # Write each event node
            for event in self._module.events:
                self._write_event(event)

            # Events table footer
            self._write('''\
    HDK_MOD_EventsEnd
};
''')

        if self._bGenerateServices:

            # Services action tables
            self._write_section("Service Methods")
            for service in self._module.services:

                # Service actions header
                self._write('''\
static const HDK_MOD_Method* %s[] =
{
''' % (self._module.service_actions_name(service)))

                # Service actions
                for action_uri in service.actions:

                    action_loc = [a for a in self._module.action_locations() if a.action.uri == action_uri][0]
                    self._write('''\
    &s_methods[%s],
''' % (self._module.action_value(action_loc)))

                # Service actions footer
                self._write('''\
    0
};
''')

            # Services event tables
            self._write_section("Service Events")
            for service in self._module.services:

                # Service events header
                self._write('''\
static const HDK_MOD_Event* %s[] =
{
''' % (self._module.service_events_name(service)))

                # Service events
                for eventURI in service.events:

                    event = [e for e in self._module.events if e.uri == eventURI][0]
                    self._write('''\
    &s_events[%s],
''' % (self._module.event_value(event)))

                # Service events footer
                self._write('''\
    0
};
''')

            # Services table
            self._write_section("Services")

            # Services table header
            self._write('''\
static const HDK_MOD_Service s_services[] =
{
''')

            # Write each service node
            for service in self._module.services:
                self._write('''\
    {
        "%s",
        %s,
        %s
    },
''' % (service.uri,
       self._module.service_actions_name(service),
       self._module.service_events_name(service)))

            # Services table footer
            self._write('''\
    HDK_MOD_ServicesEnd
};
''')

        # State definitions
        if self._module.states:

            self._write_section("ADI")
            self._write_schema("s_schemaNodes_ADI", "s_schema_ADI",
                               self._module.schema.state_nodes())

        # Struct schema definitions
        for struct in self._module.schema.structs:
            self._write_section("Struct %s" % (struct.uri))
            self._write_schema(self._module.schema_nodes_static(struct),
                               self._module.schema_static(struct),
                               self._module.schema.struct_nodes(struct))
            self._write('''\

/* extern */ const HDK_XML_Schema* %s()
{
    return &%s;
}
''' % (self._module.dom_schema_fn(struct),
       self._module.schema_static(struct)))

        # Module definition
        if self._bGenerateModule:

            self._write_section("Module");

            # Network Object ID definition
            if self._noid is not None:
                self._write('''\
/* %s */
static const HDK_XML_UUID s_uuid_NOID =
{
    { %s }
};

''' % (self._noid, ", ".join(["0x%02x" % ord(byte) for byte in self._noid.bytes])))

            # Module definition and accessors
            self._write('''\
static const HDK_MOD_Module s_module =
{
    %s,
    %s,
    %s,
    %s,
    %s,
    %s
};

const HDK_MOD_Module* %s(void)
{
    return &s_module;
}
''' % (
    "&s_uuid_NOID" if self._noid else "0",
    '"' + self._friendlyName + '"' if self._friendlyName else "0",
    "s_services" if self._bGenerateServices else "0",
    "s_methods" if self._module.actions else "0",
    "s_events" if self._module.events else "0",
    "&s_schema_ADI" if self._module.states else "0",
    self._module.module_fn()
    ))

            if self._bGenerateModuleDynamic:
                self._write('''\

const HDK_MOD_Module* HDK_SRV_Module(void)
{
    return &s_module;
}
''')


    #
    # *_methods.c code generator
    #

    def _generate_methods_c(self):

        # File header
        self._write_header_c()
        self._write('''\

#include "%s"

#include "hdk_srv.h"


/* Helper method for HNAP results */
#define SetHNAPResult(pStruct, prefix, method, result) \\
    prefix##_Set_##method##Result(pStruct, prefix##_Element_##method##Result, prefix##_Enum_##method##Result_##result)
''' % (self._module.filename('.h')))

        # Action declarations
        for action in self._module.actions:

            self._write_section("Method %s" % (action.uri))
            self._write('''\
#ifdef %s

void %s(HDK_MOD_MethodContext* pMethodCtx, HDK_XML_Struct* pInput, HDK_XML_Struct* pOutput)
{
    /* Unused parameters */
    (void)pMethodCtx;
    (void)pInput;
    (void)pOutput;
}

#endif /* %s */
''' % (self._module.action_sentinel(action),
       self._module.action_fn(action),
       self._module.action_sentinel(action)))


    #
    # *_adi.txt report generator
    #

    def _generate_adi_txt(self):

        # ADI report title
        self._write('''\
======================================================================
ADI Report for the %s Module
======================================================================
''' % (self._baseName.upper()))

        # Output the ADI values used (show get/set)
        self._write('''\


======================================================================
ADI values
======================================================================
''')
        for state in self._model.referenced_states():

            # Determine get/set
            usages = {}
            for actionState in self._model.actionStates:
                if state.uri in actionState.stateMembers:
                    stateMember = actionState.stateMembers[state.uri]
                    if stateMember.isGet:
                        usages["get"] = None
                    if stateMember.isSet:
                        usages["set"] = None
            sUsages = ', '.join(sorted(usages.iterkeys()))

            # Get the base type
            type = state.type
            nArray = 0
            while type.isArray:
                nArray += 1
                type = type.arrayType
            sArray = "[]" * nArray

            # Get the type description
            sDesc = ""
            if type.isEnum:
                sDesc = "enum"
            elif type.isStruct:
                sDesc = "struct"

            # Get the type string
            if sDesc:
                sType = '%s%s (%s, "%s")' % (type.name, sArray, sDesc, type.namespace)
            else:
                sType = '%s%s' % (type.name, sArray)

            self._write('''\

%s

    Namespace: "%s"
         Name: "%s"
         Type: %s
        Usage: %s
''' % (self._module.state_value(state), state.namespace, state.name, sType, sUsages))

        # Output the ADI values used by action
        self._write('''\


======================================================================
ADI values by action
======================================================================
''')
        for actionState in self._model.actionStates:
            self._write('''\

%s

''' % (actionState.uri))
            for stateMember in sorted(actionState.stateMembers.itervalues()):

                # Determine usage
                usages = {}
                if stateMember.isGet:
                    usages["get"] = None
                if stateMember.isSet:
                    usages["set"] = None
                sUsages = ', '.join(sorted(usages.iterkeys()))

                self._write('''\
    [%s] %s
''' % (sUsages, self._module.state_value(stateMember.state)))


######################################################################

if __name__ == "__main__":
    main()
