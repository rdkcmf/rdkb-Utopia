#!/usr/bin/python

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

import optparse
import os
import re
import sys
import traceback
import datetime

sys.path.append(os.path.join(os.path.dirname(sys.argv[0]), "lib"))
from hdk.hslparser import HSLParser
from hdk.codeutil import makeGlobalSymbol
from hdk.modutil import HDKModule

#
# Main
#
def main():

    # Command line options
    cmdParser = optparse.OptionParser("usage: %prog [options] hnap...")
    cmdParser.add_option("-o", action = "store", type = "string", dest = "outputDir", default = ".",
                         help = "output directory", metavar = "DIR")
    cmdParser.add_option("-b", action = "store", type = "string", dest = "baseName", default = "hdkcli",
                         help = "base file name (e.g. 'hdk')", metavar = "NAME")
    cmdParser.add_option("--module-name", action = "store", type = "string", dest = "moduleName", default = "hdk",
                         help = "name for module (schema) files", metavar = "MODULE")
    cmdParser.add_option("-I", action = "append", dest = "importDirs", default = [],
                         help = "add directory to import path", metavar = "DIR")
    cmdParser.add_option("--action-include", action = "append", dest = "actionsInclusive",
                         help = "include only specified HNAP actions", metavar = "ACTION")
    cmdParser.add_option("--action-exclude", action = "append", dest = "actionsExclusive",
                         help = "exclude specified HNAP actions", metavar = "ACTION")
    cmdParser.add_option("--action-location", action = "store", type = "string", dest = "actionLocation", default = "/HNAP1",
                         help = "specify the module actions HTTP location", metavar = "PATH")
    cmdParser.add_option("--dom-schema", action = "store_true", dest = "domSchema", default = False,
                         help = "generate DOM schemas for user-defined structs")
    cmdParser.add_option("--no-inline", action = "store_true", dest = "noInline", default = False,
                         help = "do not generate inline code")

    # Parse command line options
    (cmdOptions, cmdArgs) = cmdParser.parse_args()
    if cmdArgs is None or not cmdArgs:
        cmdParser.error("Must specify at least one HNAP file")

    # Parse HNAP files
    parser = HSLParser(cmdOptions.importDirs)
    for file in cmdArgs:
        parser.parseFile(file)
    parser.parseDone()
    if parser.errors:
        for error in parser.errors:
            print >> sys.stderr, error
        sys.exit(1)

    # Include/exclude HNAP actions
    if cmdOptions.actionsExclusive is not None:
        parser.filterActions(cmdOptions.actionsExclusive, True)
    if not parser.errors and cmdOptions.actionsInclusive is not None:
        parser.filterActions(cmdOptions.actionsInclusive, False)
    if parser.errors:
        for error in parser.errors:
            print >> sys.stderr, error
        sys.exit(1)

    # Get the parsed actions, types and services
    model = parser.getmodel()
    if not model.actions and not cmdOptions.domSchema:
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
        gen = CppClientGenerator(model, cmdOptions.baseName, cmdOptions.moduleName,
                                 cmdOptions.actionLocation, cmdOptions.domSchema,
                                 not cmdOptions.noInline)
        gen.generate(cmdOptions.outputDir, sys.stdout)
    except:
        print >> sys.stderr, "Error: An unexpected exception occurred generating code:"
        traceback.print_exc()
        sys.exit(1)


class CppNamespaceScope:

    def __init__(self, ns):

        self._ns = ns
        self._refCount = 0

        self.increment()

    def increment(self):

        self._refCount = self._refCount + 1

    def decrement(self):

        self._refCount = self._refCount - 1

        return 0 == self._refCount

    @property
    def namespace(self):

        return self._ns

    @property
    def needs_closing(self):

        return 0 == self._refCount

#
# c++ client code generation
#
class CppClientGenerator:

    # Class initializer
    def __init__(self, model, baseName, moduleName, actionLocation, fDOM, fInline):

        self._baseName = baseName
        self._fDOM = fDOM
        self._fInline = fInline

        # Generator state
        self._out = None
        self._namespaceStack = []

        # The underlying module.
        self._module = HDKModule(model, moduleName, actionLocation, fDOM)

        # Type information
        structTypes = {}
        arrayTypes = {}
        enumTypes = {}
        for type in model.all_types(user_types = fDOM, state_types = False):
            if type.isArray:
                arrayTypes[type.uri] = type
            elif type.isStruct:
                structTypes[type.uri] = type
            elif type.isEnum:
                enumTypes[type.uri] = type

        self._structTypes = sorted(structTypes.values(), key = lambda x:x.uri)
        self._arrayTypes = sorted(arrayTypes.values(), key = lambda x:x.uri)
        self._enumTypes = sorted(enumTypes.values(), key = lambda x:x.uri)


    #
    # Generate code
    #

    def generate(self, dir, fhReport):

        # Generate c++ client code
        for file, fn in [(os.path.join(dir, self._filename('.h')), self._generate_h),
                         (os.path.join(dir, self._filename('.cpp')), self._generate_cpp)]:

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
    # Helpers
    #

    # Is the given type a "blob"
    @staticmethod
    def is_blob_type(type):

        return type.name == "blob"

    @staticmethod
    def is_bool_type(type):

        return type.name == "bool"

    @staticmethod
    def is_int_type(type):

        return type.name == "int"

    @staticmethod
    def is_long_type(type):

        return type.name == "long"

    @staticmethod
    def is_datetime_type(type):

        return type.name == "datetime"

    @staticmethod
    def is_ipaddress_type(type):

        return type.name == "IPAddress"

    @staticmethod
    def is_macaddress_type(type):

        return type.name == "MACAddress"

    @staticmethod
    def is_string_type(type):

        return type.name == "string"

    @staticmethod
    def is_uuid_type(type):

        return type.name == "UUID"

    @staticmethod
    def is_hnap_result_type(type):

        return type.name == "Result"

    # Does the given type support the "blank" value
    @staticmethod
    def supports_blank(type):

        return CppClientGenerator.is_ipaddress_type(type) or CppClientGenerator.is_macaddress_type(type)

    # Retrieve the HDK_XML_ function to use for the given type and access
    def _hdk_struct_accessor(self, type, access):

        if CppClientGenerator.is_datetime_type(type):
            method = "HDK_XML_" + access + "_DateTime"
        elif CppClientGenerator.is_macaddress_type(type) or \
                CppClientGenerator.is_ipaddress_type(type):
            method = "HDK_XML_" + access + "_" + type.name
        elif CppClientGenerator.is_uuid_type(type):
            method = "HDK_XML_" + access + "_UUID"
        elif type.isEnum:
            method = self._module.enum_accessor(type, access)
        elif type.isStruct:
            method = "HDK_XML_" + access + "_Struct"
        else:
            method = "HDK_XML_" + access + "_" + type.name.capitalize()

        return method

    # Retrieve the HDK type cast for the given type
    def _hdk_type_cast(self, type):

        if type.isEnum:
            return "(" + self._module.enum_enum(type) + ")"
        elif CppClientGenerator.is_bool_type(type):
            return "(int)"
        else:
            return ""

    #
    # Symbol helper methods
    #

    # Generated code filename
    def _filename(self, suffix):

        return self._baseName + suffix

    # c++ namespace
    def _namespace(self, uri):

        ns = re.sub("^\w+:/+(www\.)?", "", uri)
        ns = re.sub("\.com", "", ns)
        ns = re.sub("/+HNAPExt", "", ns)

        ns = re.sub("/+$", "", ns)
        if ns:
            ns = ns[0].upper() + ns[1:]

        return re.sub("[^\w]", "_", ns)

    # c++ class name
    def _class_class(self, type):

        assert(type.isStruct)

        if type.isArray:
            match = re.match("ArrayOf(.*)", type.name);
            return makeGlobalSymbol("", "", match.group(1) + "Array")
        else:
            return makeGlobalSymbol("", "", type.name + "Struct")

    def _class_class_iter(self, type):

        return self._class_class(type) + "Iter"

    # c++ class constructor
    def _class_constructor(self, type):

        assert(type.isStruct)
        return self._class_class(type)

    # c++ class set property method
    def _class_set_property_method(self, member):

        return "set_" + member.name

    # c++ class get property method
    def _class_get_property_method(self, member):

        return "get_" + member.name

    # c++ class from file method
    def _class_from_file_method(self):

        return "FromFile"

    # c++ class to file method
    def _class_to_file_method(self):

        return "ToFile"

    def _type_type(self, type, withNS = False):

        typeName = None

        if type.isBuiltin:
            if CppClientGenerator.is_bool_type(type):
               typeName = "bool"
            elif CppClientGenerator.is_int_type(type):
                typeName = "HDK_XML_Int"
            elif CppClientGenerator.is_long_type(type):
                typeName = "HDK_XML_Long"
            elif CppClientGenerator.is_string_type(type):
                typeName = "const char*"
            elif CppClientGenerator.is_datetime_type(type):
                typeName = "time_t"
            elif CppClientGenerator.is_ipaddress_type(type):
                typeName = "IPv4Address"
            elif CppClientGenerator.is_macaddress_type(type):
                typeName = "MACAddress"
            elif CppClientGenerator.is_blob_type(type):
                typeName = "Blob"
            elif CppClientGenerator.is_uuid_type(type):
                typeName = "UUID"
            elif CppClientGenerator.is_hnap_result_type(type):
                typeName = type.name
                if fWithNamespace and self._current_open_namespace() != type.namespace:
                    typeName = self._namespace(type.namespace) + "::" + typeName
            else:
                print>>sys.stderr, "Unhandled builtin type %s" % (type.name)
                typeName = type.name

        elif type.isStruct:
            typeName = self._class_class(type)
            namespace = self._namespace(type.namespace)

            if namespace and withNS and self._current_open_namespace() != type.namespace:
                typeName = namespace + "::" + typeName

        elif type.isEnum:
            typeName = self._enum_enum(type)
            if withNS and self._current_open_namespace() != type.namespace:
                typeName = self._namespace(type.namespace) + "::" + typeName
            typeName = "enum " + typeName

        return typeName

    # c++ type cast
    def _type_cast(self, type):

        if type.isEnum:
            return "(" + self._type_type(type, withNS = True)  + ")"
        elif CppClientGenerator.is_bool_type(type):
            return "(bool)"
        else:
            return ""

    # c++ enum type
    def _enum_enum(self, type):

        return makeGlobalSymbol("", "", type.name)

    # c++ enum value
    def _enum_value(self, type, value, bIsUnknown = False):

        if bIsUnknown:
            return makeGlobalSymbol(type.name, "", "Unknown")
        else:
            return makeGlobalSymbol(type.name, "", value)

    # c++ action
    def _action_fn(self, action_loc, withNS = False):

        (http_method, action) = (action_loc.http_method, action_loc.action)

        fn = action.name
        if withNS:
            fn = self._namespace(action.namespace) + "::" + fn

        # Non-post action?
        if http_method != "POST":
            suffix = "_" + http_method
        else:
            suffix = ""

        return fn + suffix

    #
    # Output helper methods
    #

    # Output
    def _write(self, s):

        self._out.write(s)

    def _write_newline(self):

        self._out.write('''
''')

    def _write_with_indent(self, s, cIndent, bIndentCPreprocessor = False):

        indent = cIndent * ' '
        for line in s.splitlines(True):
            if line[0] == '#' and not bIndentCPreprocessor:
                self._write(line)
            elif re.search("^\s*$", line):
                # Don't write unnecessary whitespace
                self._write(line)
            else:
                self._write(indent + line)

    def _write_with_ns_indent(self, s, extraIndent = 0):

        self._write_with_indent(s, self._namespace_indent() + extraIndent)

    def _write_copyright(self):

        self._write('''\
/*
 * Copyright (c) 2008-2010 Cisco Systems, Inc. All rights reserved.
 *
 * Cisco Systems, Inc. retains all right, title and interest (including all
 * intellectual property rights) in and to this computer program, which is
 * protected by applicable intellectual property laws.  Unless you have obtained
 * a separate written license from Cisco Systems, Inc., you are not authorized
 * to utilize all or a part of this computer program for any purpose (including
 * reproduction, distribution, modification, and compilation into object code),
 * and you must immediately destroy or return to Cisco Systems, Inc. all copies
 * of this computer program.  If you are licensed by Cisco Systems, Inc., your
 * rights to utilize this computer program are limited by the terms of that
 * license.  To obtain a license, please contact Cisco Systems, Inc.
 *
 * This computer program contains trade secrets owned by Cisco Systems, Inc.
 * and, unless unauthorized by Cisco Systems, Inc. in writing, you agree to
 * maintain the confidentiality of this computer program and related information
 * and to not disclose this computer program and related information to any
 * other person or entity.
 *
 * THIS COMPUTER PROGRAM IS PROVIDED AS IS WITHOUT ANY WARRANTIES, AND CISCO
 * SYSTEMS, INC. EXPRESSLY DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * INCLUDING THE WARRANTIES OF MERCHANTIBILITY, FITNESS FOR A PARTICULAR
 * PURPOSE, TITLE, AND NONINFRINGEMENT.
 */
''')

    # Write .cpp file header
    def _write_header_cpp(self, fileName = ""):

        self._write_copyright()

        self._write('''\

// %s - [Generated by hdkcli_cpp]
''' % (fileName))

    # Write .h file header
    def _write_header_h(self, fileName = ""):

        self._write_copyright()
        self._write('''\

#pragma once

// %s - [Generated by hdkcli_cpp]
''' % (fileName))

    # Write .h file footer
    def _write_footer_h(self):

        # Namespace stack should be empty now.
        assert(not self._namespaceStack)

    # Write the opening of a namespace
    def _write_open_namespace(self, uri, cIndent):

        indent = ' ' * cIndent
        namespace = self._namespace(uri)

        self._write('''\

%snamespace %s
%s{
''' % (indent, namespace,
       indent))

    # Write the closing of a namespace
    def _write_close_namespace(self, uri, cIndent):

        indent = ' ' * cIndent
        namespace = self._namespace(uri)

        self._write('''\
%s} // namespace %s
''' % (indent, namespace))

    # Generate a class forward declaration
    def _write_class_forward_declaration(self, type):

        self._write_with_ns_indent('''\
class %s;
''' % (self._class_class(type)))

    # Generate an enum definition
    def _write_enum(self, type, extraIndent = 0):

        assert(type.isEnum)

        indent = self._namespace_indent() + extraIndent

        self._write_doxygen_section_with_ns_indent_begin(extraIndent)
        self._write_doxygen_string_with_ns_indent('''\
\\enum %s
     <a>%s</a>
''' % (self._enum_enum(type),
       type.uri), extraIndent)
        self._write_doxygen_lines_with_ns_indent(type.doctext, extraIndent)
        self._write_doxygen_section_with_ns_indent_end(extraIndent)

        self._write_with_indent('''\
enum %s
{
    %s = %s /*<! Unknown value */,
''' % (self._enum_enum(type),
       self._enum_value(type, None, True), self._module.enum_value(type, None, True)), indent)

        for value in type.enumValues[0:-1]:
            self._write_with_indent('''\
    %s = %s /*!< %s */,
''' % (self._enum_value(type, value), self._module.enum_value(type, value), value), indent)

        if type.enumValues:
            value = type.enumValues[-1]
            self._write_with_indent('''\
    %s = %s /*!< %s */
''' % (self._enum_value(type, value), self._module.enum_value(type, value), value), indent)
        self._write_with_indent('''\
}; // enum %s

''' % (self._enum_enum(type)), indent)

    #
    # Doxygen-formatted output helpers
    #

    # Doxygen comment prefix.
    _doxygenPrefix = "///"

    def _write_doxygen_line_with_ns_indent(self, line, extraIndent = 0):

        doxygenFormattedLine = CppClientGenerator._doxygenPrefix
        if line and not re.search("^\s$", line):
            doxygenFormattedLine += " " + line

        if not doxygenFormattedLine.endswith("\n"):
            doxygenFormattedLine += "\n"

        self._write_with_indent(doxygenFormattedLine, extraIndent + self._namespace_indent())

    def _write_doxygen_lines_with_ns_indent(self, lines, extraIndent = 0):

        for line in lines:
            self._write_doxygen_line_with_ns_indent(line, extraIndent)

    def _write_doxygen_string_with_ns_indent(self, s, extraIndent = 0):

        for line in s.splitlines(True):
            self._write_doxygen_line_with_ns_indent(line, extraIndent)

    def _write_doxygen_section_with_ns_indent_begin(self, extraIndent = 0):

        self._write_doxygen_line_with_ns_indent(None, extraIndent)

    def _write_doxygen_section_with_ns_indent_end(self, extraIndent = 0):

        self._write_doxygen_line_with_ns_indent(None, extraIndent)

    def _write_doxygen_section_with_ns_indent(self, s, extraIndent = 0):

        self._write_doxygen_section_with_ns_indent_begin(extraIndent)

        self._write_doxygen_string_with_ns_indent(s, extraIndent)

        self._write_doxygen_section_with_ns_indent_end(extraIndent)

    #
    # Namespace scoping
    #

    # Update the current namespace (closing the previous one, if needed)
    def _push_namespace(self, ns):

        if self._namespaceStack:
            if self._namespaceStack[-1].namespace == ns:
                if self._namespaceStack[-1].needs_closing:
                    self._namespaceStack[-1].increment()
                    return
                #else: ns::ns. strange...
            else:
                if self._namespaceStack[-1].needs_closing:
                    self._write_close_namespace(self._namespaceStack[-1].namespace, (len(self._namespaceStack) - 1) * 4)
                    self._namespaceStack.pop()

        self._namespaceStack.append(CppNamespaceScope(ns))
        self._write_open_namespace(ns, (len(self._namespaceStack) - 1) * 4)

    def _pop_namespace(self, bAlwaysClose = False):

        if self._namespaceStack[-1].needs_closing:
            self._write_close_namespace(self._namespaceStack[-1].namespace, (len(self._namespaceStack) - 1) * 4)
            self._namespaceStack.pop()

        if self._namespaceStack:
            self._namespaceStack[-1].decrement()

            if bAlwaysClose and self._namespaceStack[-1].needs_closing:
                self._write_close_namespace(self._namespaceStack[-1].namespace, (len(self._namespaceStack) - 1) * 4)
                self._namespaceStack.pop()

    # Namespace indentation
    def _namespace_indent(self):

        return len(self._namespaceStack) * 4

    # Current open namespace
    def _current_open_namespace(self):

        if self._namespaceStack:
            return self._namespaceStack[-1].namespace
        else:
            return None

    #
    # Helpers for c++ class wrappers of HDK_XML_Structs
    #

    # Generate a default constructor
    def _write_class_default_constructor(self, struct, element = None, fImpl = False, classScope = None, extraIndent = 0):

        if element is None:
            element = "HDK_XML_BuiltinElement_Unknown"

        method = self._class_constructor(struct)
        if classScope is not None:
            method = self._type_type(classScope, withNS = True) + "::" + method

        self._write_with_ns_indent('''\
%s() throw()''' % (method), extraIndent)

        if fImpl:
            self._write(''' :''')
            self._write_with_ns_indent('''
    Struct(%s)
{
}
''' % (element), extraIndent)
        else: # not fImpl
            self._write(''';
''')

    # Generate a constructor with HDK_XML_Struct
    def _write_class_hdk_constructor(self, struct, fImpl = False, classScope = None, extraIndent = 0):

        method = self._class_constructor(struct)
        if classScope is not None:
            method = self._type_type(classScope, withNS = True) + "::" + method

        self._write_with_ns_indent('''\
%s(HDK_XML_Struct* phdkstruct) throw()''' % (method), extraIndent)

        if fImpl:
            self._write(''' :''')
            self._write_with_ns_indent('''
    Struct(phdkstruct)
{
}
''', extraIndent)
        else: # not fImpl
            self._write(''';
''')

    # Generate a get_ accessor implementation.
    def _write_class_get_accessor(self, member, fImpl = False, classScope = None, extraIndent = 0):

        element = self._module.element_value((member.namespace, member.name))

        method = self._class_get_property_method(member)
        if classScope is not None:
            method = self._type_type(classScope, withNS = True) + "::" + method

        self._write_with_ns_indent('''\
%s %s() const throw()''' % (self._type_type(member.type, withNS = True), method), extraIndent)

        if fImpl:
            self._write_with_ns_indent('''\

{
''', extraIndent)
            if member.type.isStruct:
                self._write_with_ns_indent('''\
    return %s(GetStruct(), %s);
''' % (self._hdk_struct_accessor(member.type, "Get"), element), extraIndent)
            else:
                # Blob type is special-cased.
                if CppClientGenerator.is_blob_type(member.type):
                    self._write_with_ns_indent('''\
    unsigned int cbBlob = 0;
    char* pbBlob = %s(GetStruct(), %s, &cbBlob);
    return Blob(pbBlob, cbBlob);
''' % (self._hdk_struct_accessor(member.type, "Get"), element), extraIndent)
                else:
                    # Determine the default value
                    if member.type.isEnum:
                        defaultValue = self._module.enum_value(member.type, None, is_unknown = True)
                    elif CppClientGenerator.is_ipaddress_type(member.type):
                        defaultValue = "HDK::IPv4Address::Blank()"
                    elif CppClientGenerator.is_macaddress_type(member.type):
                        defaultValue = "HDK::MACAddress::Blank()"
                    else:
                        defaultValue = "0"

                    # Determine the cast for the type.
                    cast = self._type_cast(member.type)
                    if CppClientGenerator.is_bool_type(member.type):
                        # Avoid Microsoft compiler warning C4800
                        cast = "0 != "
                    self._write_with_ns_indent('''\
    return %s%s(GetStruct(), %s, %s);
''' % (cast, self._hdk_struct_accessor(member.type, "GetEx"), element, defaultValue), extraIndent)
            self._write_with_ns_indent('''\
}
''', extraIndent)

        else: # not fImpl
            self._write(''';
''')

    # Generate a set_ accessor implementation.  If classScope is present, the method will be scoped in the class.
    def _write_class_set_accessor(self, member, fImpl = False, classScope = None, extraIndent = 0):

        element = self._module.element_value((member.namespace, member.name))

        if member.type.isStruct or \
                CppClientGenerator.is_blob_type(member.type) or \
                CppClientGenerator.is_ipaddress_type(member.type) or \
                CppClientGenerator.is_macaddress_type(member.type):

            argType = "const " + self._type_type(member.type, withNS = True) + "&"
        else:
            argType = self._type_type(member.type, withNS = True)

        method = self._class_set_property_method(member)
        if classScope is not None:
            method = self._type_type(classScope, withNS = True) + "::" + method

        self._write_with_ns_indent('''\
void %s(%s value) throw()''' % (method, argType), extraIndent)
        if fImpl:
            self._write_with_ns_indent('''\

{
''', extraIndent)

            if CppClientGenerator.supports_blank(member.type):
                self._write_with_ns_indent('''\
    if (value.IsBlank())
    {
        (void)HDK_XML_Set_Blank(GetStruct(), %s);
        return;
    }
''' % (element), extraIndent)

            # Handle non-standard value sets (struct and blob types)
            if member.type.isStruct:
                self._write_with_ns_indent('''\
    (void)%s(GetStruct(), %s, value);
''' % (self._hdk_struct_accessor(member.type, "SetEx"), element), extraIndent)

            elif CppClientGenerator.is_blob_type(member.type):
                self._write_with_ns_indent('''\
    (void)%s(GetStruct(), %s, value.get_Data(), value.get_Size());
''' % (self._hdk_struct_accessor(member.type, "Set"), element), extraIndent)

            else:
                self._write_with_ns_indent('''\
    (void)%s(GetStruct(), %s, %svalue);
''' % (self._hdk_struct_accessor(member.type, "Set"), element, self._hdk_type_cast(member.type)), extraIndent)

            self._write_with_ns_indent('''\
}
''', extraIndent)

        else: # not fImpl
            self._write(''';
''')

    # Generate a FromFile method implementation.
    def _write_class_method_from_file(self, type, fImpl = False, classScope = None, extraIndent = 0):

        method = self._class_from_file_method()
        if classScope is not None:
            method = self._type_type(classScope, withNS = True) + "::" + method

        self._write_with_ns_indent('''\
bool %s(const char* pszFile) throw()''' % (method), extraIndent)

        if fImpl:
            self._write_with_ns_indent('''\

{
    return HDK::Struct::DeserializeFromFile(%s(), pszFile);
}
''' % (self._module.dom_schema_fn(type)), extraIndent)
        else: # not fImpl
            self._write(''';
''')


    # Generate a ToFile method implementation.
    def _write_class_method_to_file(self, type, fImpl = False, classScope = None, extraIndent = 0):

        method = self._class_to_file_method()
        if classScope is not None:
            method = self._type_type(classScope, withNS = True) + "::" + method

        self._write_with_ns_indent('''\
bool %s(const char* pszFile) const throw()''' % (method), extraIndent)

        if fImpl:
            self._write_with_ns_indent('''\

{
    return HDK::Struct::SerializeToFile(%s(), pszFile, 0);
}
''' % (self._module.dom_schema_fn(type)), extraIndent)
        else: # not fImpl
            self._write(''';
''')

    # Generate a c++ class declaration from an HDK_XML_Struct
    def _write_class_declaration(self, struct,
                                 fGenerateSets = True, fConstructFromStruct = True,
                                 fInline = False, rootElement = None, fDOM = False):

        self._write_doxygen_section_with_ns_indent_begin()
        self._write_doxygen_string_with_ns_indent('''\
\\brief %s
     <a>%s</a>
''' % (struct.name, struct.uri))
        self._write_doxygen_lines_with_ns_indent(struct.doctext)
        self._write_doxygen_section_with_ns_indent_end()

        self._write_with_ns_indent('''\
class %s : public Struct
{
''' % (self._class_class(struct)))

        # Generate the class constructors and destructor.
        self._write_with_ns_indent('''\
public:
    //
    // Constructors/Destructor.
    //
''')
        self._write_class_default_constructor(struct, element = rootElement, fImpl = fInline, extraIndent = 4)
        self._write_newline()

        if fConstructFromStruct:
            self._write_class_hdk_constructor(struct, fImpl = fInline, extraIndent = 4)
            self._write_newline()

        # Write the accessor methods.
        for member in struct.members:

            self._write_doxygen_section_with_ns_indent_begin(4)
            self._write_doxygen_string_with_ns_indent('''\
\\brief Get the %s value.
''' % (member.name), 4)
            if member.doctext:
                self._write_doxygen_line_with_ns_indent('''\
     \\retval %s
''' % (member.doctext[0]), 4)

                self._write_doxygen_lines_with_ns_indent(member.doctext[1:], 4)
            self._write_doxygen_section_with_ns_indent_end(4)

            self._write_class_get_accessor(member, fImpl = fInline, extraIndent = 4)
            self._write_newline()

            if fGenerateSets:
                self._write_doxygen_section_with_ns_indent_begin(4)
                self._write_doxygen_string_with_ns_indent('''\
\\brief Set the %s value.
''' % (member.name), 4)
                if member.doctext:
                    self._write_doxygen_string_with_ns_indent('''\
     \\arg %s
''' % (member.doctext[0]), 4)
                    self._write_doxygen_lines_with_ns_indent(member.doctext[1:], 4)
                self._write_doxygen_section_with_ns_indent_end(4)

                self._write_class_set_accessor(member, fImpl = fInline, extraIndent = 4)
                self._write_newline()

        if fDOM:
            self._write_doxygen_section_with_ns_indent_begin(4)
            self._write_doxygen_string_with_ns_indent('''\
\\brief Serialize to/from an XML file.
''', 4)
            self._write_doxygen_section_with_ns_indent_end(4)

            self._write_class_method_from_file(struct, fImpl = fInline, extraIndent = 4)
            self._write_class_method_to_file(struct, fImpl = fInline, extraIndent = 4)
            self._write_newline()

        self._write_with_ns_indent('''\
}; // class %s : public Struct

''' % self._class_class(struct))

    # Generate a wrapper for an array class using the array template
    def _write_array_template_typedef(self, type, includeClassDeclaration = True):

        assert(type.isArray)

        self._write_doxygen_section_with_ns_indent('''\
\\class %s
     Wrapper class for accessing arrays of %s values.
''' % (self._class_class(type),
       type.arrayType.name))

        # element represents the XML element of each array item
        element = self._module.element_value((type.members[0].namespace, type.members[0].name))

        if type.arrayType.isStruct:
            if includeClassDeclaration:
                self._write_with_ns_indent('''\
class %s; // forward declaration
''' % (self._class_class(type.arrayType)))
            self._write_with_ns_indent('''\
typedef HDK::StructArray<%s, %s> %s;
typedef HDK::StructArray<%s, %s>::StructArrayIter %sIter;

''' % (self._type_type(type.arrayType, withNS = True), element, self._class_class(type),
       self._type_type(type.arrayType, withNS = True), element, self._class_class(type)))
        elif type.arrayType.isEnum:
            self._write_with_ns_indent('''\
typedef HDK::EnumArray<%s, %s, %s> %s;
typedef HDK::EnumArray<%s, %s, %s>::EnumArrayIter %sIter;

''' % (self._type_type(type.arrayType, withNS = True), self._module.enum_type_value(type.arrayType), element, self._class_class(type),
       self._type_type(type.arrayType, withNS = True), self._module.enum_type_value(type.arrayType), element, self._class_class(type)))
        elif type.arrayType.name == "datetime":
            self._write_with_ns_indent('''\
typedef HDK::DateTimeArray<%s> %s;
typedef HDK::DateTimeArray<%s>::DateTimeArrayIter %sIter;

''' % (element, self._class_class(type),
       element, self._class_class(type)))
        else:
            templateClass = type.arrayType.name.capitalize() + "Array"
            self._write_with_ns_indent('''\
typedef HDK::%s<%s> %s;
typedef HDK::%s<%s>::%sIter %sIter;

''' % (templateClass, element, self._class_class(type),
       templateClass, element, self._class_class(type), templateClass))

    #
    # Generate a c++ class definition from an HDK_XML_Struct
    #
    def _write_class_definition(self, struct,
                                fGenerateSets = True, fConstructFromStruct = True,
                                rootElement = None,
                                fDOM = False):

        assert(struct.isStruct)

        # Constructors.
        self._write_class_default_constructor(struct, element = rootElement, fImpl = True, classScope = struct)

        if fConstructFromStruct:
            self._write_newline()
            self._write_class_hdk_constructor(struct, fImpl = True, classScope = struct)

        for member in struct.members:

            element = self._module.element_value((member.namespace, member.name))

            # Get accessor
            self._write_newline()
            self._write_class_get_accessor(member, fImpl = True, classScope = struct)

            # Set accessor
            if fGenerateSets:
                self._write_newline()
                self._write_class_set_accessor(member, fImpl = True, classScope = struct)

        if fDOM:
            self._write_newline()
            self._write_class_method_from_file(struct, fImpl = True, classScope = struct)

            self._write_newline()
            self._write_class_method_to_file(struct, fImpl = True, classScope = struct)

    # Helper to generate the action method documentation
    def _write_action_method_documentation(self, action_loc, extraIndent = 0):

        (http_method, http_location, action, noauth) = \
            (action_loc.http_method, action_loc.http_location, action_loc.action, action_loc.noauth)

        bInput = (len(action.inputMember.type.members) > 0)
        bOutput = (len(action.outputMember.type.members) > 1)

        self._write_doxygen_section_with_ns_indent_begin(extraIndent)
        self._write_doxygen_string_with_ns_indent('''\
\\brief Call the %s method on a given device.
    <a>%s</a>
    This method uses HTTP method %s and location '%s'
''' % (action.name,
       action.uri,
       http_method, http_location), extraIndent)
        if noauth:
            self._write_doxygen_string_with_ns_indent('''\
    \\note This method does NOT require HTTP Basic Authorization.

''', extraIndent)
        self._write_doxygen_lines_with_ns_indent(action.doctext)
        self._write_doxygen_line_with_ns_indent("\n")

        # Write out the potential result values.
        if action.resultEnum.enumValues:
            self._write_doxygen_line_with_ns_indent('''\
    Possible result values:
''', extraIndent)
            for enumValue in action.resultEnum.enumValues:
                self._write_doxygen_line_with_ns_indent('''\
         - #%s
''' % (self._enum_value(action.resultEnum, enumValue)), extraIndent)
        self._write_doxygen_string_with_ns_indent('''\
    \\arg pTarget The target on which to call this method.
''', extraIndent)
        if bInput:
            self._write_doxygen_string_with_ns_indent('''\
    \\arg input The input argument data to the %s HNAP method.
''' % (action.name), extraIndent)

        if bOutput:
            self._write_doxygen_line_with_ns_indent('''\
    \\arg output The output argument data from the %s HNAP method.
''' % (action.name), extraIndent)

        self._write_doxygen_string_with_ns_indent('''\
    \\arg[optional] result The HNAP result of the %s HNAP method.
    \\arg timeoutSecs An optional timeout, in seconds, to use for the HNAP call.
    \\retval The result of the HNAP method call.
''' % (action.name), extraIndent)
        self._write_doxygen_section_with_ns_indent_end(extraIndent)

    # Helper to generate the action method declarations
    def _write_action_method(self, action_loc, fImpl = False, withNS = False, fNoDefaultArgs = False, extraIndent = 0):

        action = action_loc.action

        bInput = (len(action.inputMember.type.members) > 0)
        bOutput = (len(action.outputMember.type.members) > 1)

        self._write_with_ns_indent('''\
HDK::ClientError %s
(
    HDK::ITarget* pTarget,
''' % (self._action_fn(action_loc, withNS = withNS)), extraIndent)
        if bInput:
            self._write_with_ns_indent('''\
    const %s & input,
''' % (self._type_type(action.inputMember.type, withNS = True)), extraIndent)
        if bOutput:
            self._write_with_ns_indent('''\
    %s & output,
''' % (self._type_type(action.outputMember.type, withNS = True)), extraIndent)

        if fNoDefaultArgs:
            presultDefault = "/* = NULL */"
            timeoutSecsDefault = "/* = 0 */"
        else:
            presultDefault = "= NULL"
            timeoutSecsDefault = "= 0"
        self._write_with_ns_indent('''\
    %s* presult %s,
    unsigned int timeoutSecs %s
) throw()''' % (self._type_type(action.outputMember.type.members[0].type, withNS = True), presultDefault,
                timeoutSecsDefault), extraIndent)

        if fImpl:
            self._write_with_ns_indent('''\

{
    if (!pTarget)
    {
        return ClientError_InvalidArg;
    }
''', extraIndent)
            if not bInput:
                self._write_with_ns_indent('''\
    %s input;
''' % (self._type_type(action.inputMember.type, withNS = True)), extraIndent)
            if not bOutput:
                self._write_with_ns_indent('''\

    %s output;
''' % (self._type_type(action.outputMember.type, withNS = True)), extraIndent)
            self._write_with_ns_indent('''\

    ClientError error = pTarget->Request(timeoutSecs,
                                         %s(),
                                         %s,
                                         input,
                                         &output);
''' % (self._module.module_fn(),
       self._module.action_value(action_loc)), extraIndent)
            self._write_with_ns_indent('''\

    const HDK_MOD_Method* pMethod = HDK_MOD_GetMethod(%s(), %s);
''' % (self._module.module_fn(), self._module.action_value(action_loc)), extraIndent)
            self._write_with_ns_indent('''\

    // Get the result value.
    %s result = output.%s();
    if (NULL != presult)
    {
        *presult = result;
    }

    // Determine if there was an HNAP-result, and whether it was an error or not.
    if ((ClientError_OK == error) && (HDK_XML_BuiltinElement_Unknown != pMethod->hnapResultElement))
    {
        if ((pMethod->hnapResultOK != (int)result) && (pMethod->hnapResultREBOOT != (int)result))
        {
            // An HNAP error response.
            error = HDK::ClientError_HnapMethod;
        }
    }

    return error;
}
''' % (self._type_type(action.outputMember.type.members[0].type, withNS = True), \
           self._class_get_property_method(action.outputMember.type.members[0])), extraIndent)

        else: # not fImpl
            self._write(''';
''')

    # Helper to write HDK Init documentation
    def _write_init_function_documentation(self, extraIndent = 0):

        self._write_doxygen_section_with_ns_indent('''\
\\fn InitializeClient
     Initialize the HDK client library.  This should be called once per application instance.
     Each call to InitializeClient should be matched by a call to UninitializeClient.
     \\retval true if initialization was successful, false if not.
''', extraIndent)

    # Helper to write HDK Init method
    def _write_init_function(self, fImpl = False, withNS = False, extraIndent = 0):

        method = "InitializeClient"
        if withNS:
            method = "HDK::" + method

        self._write_with_ns_indent('''\
bool %s() throw()''' % (method), extraIndent)

        if fImpl:

            self._write_with_ns_indent('''\

{
    return !!HDK_CLI_Init();
}
''', extraIndent)

        else: # not fImpl

            self._write(''';
''')

    # Helper to write HDK uninit documentation
    def _write_uninit_function_documentation(self, extraIndent = 0):

        self._write_doxygen_section_with_ns_indent('''\
\\fn UninitializeClient
     Cleanup the HDK client library.  This should be called if true was returned from InitializeClient
     When the caller is done using the HDK client library.
''', extraIndent)

    # Helper to write HDK uninit method
    def _write_uninit_function(self, fImpl = False, withNS = False, extraIndent = 0):

        method = "UninitializeClient"
        if withNS:
            method = "HDK::" + method

        self._write_with_ns_indent('''\
void %s() throw()''' % (method), extraIndent)

        if fImpl:

            self._write_with_ns_indent('''\

{
    HDK_CLI_Cleanup();
}
''', extraIndent)

        else: # not fImpl

            self._write(''';
''')

    # Helper to write URI accessor macro documentation
    def _write_uri_accessor_macro_documentation(self, action_loc, extraIndent = 0):

        self._write_doxygen_section_with_ns_indent('''\
\\brief SOAP method URI for action %s
''' % (action_loc.action.name), extraIndent)

    # Helper to write URI accessor macro
    def _write_uri_accessor_macro(self, action_loc, extraIndent = 0):

        self._write_with_ns_indent('''\
#define %s (HDK_MOD_GetMethod(%s(), %s)->pszSOAPAction)
''' % (makeGlobalSymbol("", action_loc.action.namespace, action_loc.action.name, nameSuffix = "_URI"),
       self._module.module_fn(), self._module.action_value(action_loc)), extraIndent)

    #
    # .h code generator
    #
    def _generate_h(self):

        self._write_header_h(self._filename('.h'))

        self._write('''\

// Non-generated client code.
#include "hdk_cli_cpp.h"

// Underlying schema module.
#include "%s"

''' % (self._module.filename('.h')))

        self._push_namespace("HDK")

        # Write HDK global functions.
        if self._module.action_locations():
            self._write_init_function_documentation()
            self._write_init_function()
            self._write_newline()
            self._write_uninit_function_documentation()
            self._write_uninit_function()
            self._write_newline()

        # Generate c++-style enums
        for typeEnum in self._enumTypes:

            self._push_namespace(typeEnum.namespace)
            self._write_enum(typeEnum)
            self._pop_namespace()

        # Declare array types.
        for typeArray in self._arrayTypes:

            self._push_namespace(typeArray.namespace)
            self._write_array_template_typedef(typeArray)
            self._pop_namespace()

        # Generate c++ wrapper classes for each struct
        for typeStruct in self._structTypes:

            # Generate any required forward declarations.
            for member in typeStruct.members:
                # The types are sorted alphabetically, so anything greater than this one has yet to be defined.
                forwDeclType = member.type
                if forwDeclType.isArray:
                    forwDeclType = forwDeclType.arrayType
                if forwDeclType.isStruct and not forwDeclType.isArray:
                   if forwDeclType.name > typeStruct.name:
                       self._push_namespace(forwDeclType.namespace)
                       self._write_class_forward_declaration(forwDeclType)
                       self._pop_namespace()

            self._push_namespace(typeStruct.namespace)

            element = None
            if self._fDOM:
                element = self._module.element_value((typeStruct.namespace, typeStruct.name))
            self._write_class_declaration(typeStruct,
                                          fDOM = self._fDOM,
                                          fInline = self._fInline,
                                          rootElement = element)
            self._pop_namespace()

        # Generate class implementations for each action input and output structure.
        for action in self._module.actions:

            for (fGenerateSets, struct) in \
                    [(True, action.inputMember.type),
                     (False, action.outputMember.type)]:
                self._push_namespace(struct.namespace)
                self._write_class_declaration(struct,
                                              fGenerateSets, fConstructFromStruct = False,
                                              fInline = self._fInline,
                                              rootElement = self._module.element_value((action.namespace, action.name)))
                self._pop_namespace()

        # Generate the action method declarations.
        for action_loc in self._module.action_locations():
            self._push_namespace(action_loc.action.namespace)

            # Generate accessors for macros URIs.
            if action_loc.http_method == "POST":
                self._write_uri_accessor_macro_documentation(action_loc)
                self._write_uri_accessor_macro(action_loc)
                self._write_newline()

            self._write_action_method_documentation(action_loc)
            self._write_action_method(action_loc)
            self._write_newline()
            self._pop_namespace()

        # Make sure the previous namespace is closed.
        self._pop_namespace()

        # Should be in the HDK namespace scope
        assert(self._namespaceStack[-1].namespace == "HDK")

        # Close HDK namespace
        self._pop_namespace(True)

    #
    # .cpp code generator
    #
    def _generate_cpp(self):

        self._write_header_cpp(self._filename('.cpp'))

        self._write('''\

// Local header.
#include "%s"

using namespace HDK;
''' % (self._filename('.h')))

        if not self._fInline:
            # Generate class implementations for each structure type.
            for type in self._structTypes:
                element = None
                if self._fDOM:
                    element = self._module.element_value((type.namespace, type.name))

                self._write_newline()
                self._write_class_definition(type, fDOM = self._fDOM, rootElement = element)

            # Generate class implementations for each action input and output structure.
            for action in self._module.actions:
                for (fGenerateSets, member) in \
                        [(True, action.inputMember),
                         (False, action.outputMember)]:
                    self._write_newline()
                    self._write_class_definition(member.type,
                                                 fGenerateSets,
                                                 fConstructFromStruct = False,
                                                 rootElement = self._module.element_value((action.namespace, action.name)))

        # Write the action method definitions.
        if self._module.action_locations():
            # Write HDK global init/cleanup function definitions.
            self._write_newline()
            self._write_init_function(fImpl = True, withNS = True)
            self._write_newline()
            self._write_uninit_function(fImpl = True, withNS = True)

            for action_loc in self._module.action_locations():
                self._write_newline()
                self._write_action_method(action_loc, fImpl = True, withNS = True, fNoDefaultArgs = True)

######################################################################

if __name__ == "__main__":
    main()
