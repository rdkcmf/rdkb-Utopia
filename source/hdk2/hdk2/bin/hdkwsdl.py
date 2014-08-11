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
import sys
import traceback

sys.path.append(os.path.join(os.path.dirname(sys.argv[0]), "lib"))
from hdk.hslparser import HSLParser
from hdk.codeutil import makeGlobalSymbol


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
    cmdParser.add_option("-i", action = "append", dest = "actionsInclusive",
                         help = "include only specified actions", metavar = "ACTION")
    cmdParser.add_option("-x", action = "append", dest = "actionsExclusive",
                         help = "exclude specified actions", metavar = "ACTION")
    cmdParser.add_option("-I", action = "append", dest = "importDirs", default = [],
                         help = "add directory to import path", metavar = "DIR")

    # Parse command line options
    (cmdOptions, cmdArgs) = cmdParser.parse_args()
    if cmdArgs is None or not cmdArgs:
        cmdParser.error("Must specify at least one HSL file")

    # Parse HSL files
    parser = HSLParser(cmdOptions.importDirs)
    for file in cmdArgs:
        parser.parseFile(file)
    parser.parseDone()
    if parser.errors:
        for error in parser.errors:
            print >> sys.stderr, error
        sys.exit(1)

    # Include/exclude actions
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
    if not model.actions:
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
        gen = WSDLGenerator(model, cmdOptions.baseName)
        gen.generate(cmdOptions.outputDir, sys.stdout)
    except:
        print >> sys.stderr, "Error: An unexpected exception occurred generating code:"
        traceback.print_exc()
        sys.exit(1)


#
# WSDL code generator
#
class WSDLGenerator:

    # Class initializer
    def __init__(self, model, baseName):

        self._actions = model.actions
        self._types = model.referenced_types()
        self._baseName = baseName

        # Namespaces
        self._nsSoap = "http://schemas.xmlsoap.org/wsdl/soap/"
        self._nsWSDL = "http://schemas.xmlsoap.org/wsdl/"
        self._nsSchema = "http://www.w3.org/2001/XMLSchema"

        # Determine the schema namespaces
        self._schemaNamespaces = {}
        for type in self._types:
            if type.isStruct or type.isEnum:
                self._schemaNamespaces[type.namespace] = None
        for action in self._actions:
            self._schemaNamespaces[action.namespace] = None

        # Build type namespace => prefix mapping
        self._nsPrefixes = { self._nsSoap: "soap",
                             self._nsWSDL: "wsdl",
                             self._nsSchema: "s",
                             HSLParser.HNAPNamespace: "tns" }
        cTypeNS = 0
        for schemaNamespace in sorted(self._schemaNamespaces.iterkeys()):
            if schemaNamespace not in self._nsPrefixes:
                cTypeNS += 1
                self._nsPrefixes[schemaNamespace] = "t%d" % cTypeNS

        # Name collision dictionary
        self._actionNames = {}
        self._typeNames = {}
        for action in self._actions:
            if action.name in self._actionNames:
                self._actionNames[action.name] += 1
            else:
                self._actionNames[action.name] = 1

    # Helper method for action names
    def _actionName(self, action):
        if self._actionNames[action.name] == 1:
            return action.name
        else:
            return makeGlobalSymbol("", action.namespace, action.name)

    # Helper method for struct names
    def _typeName(self, type):
        return type.name

    # Helper method to determine a member's effective type
    def _effectiveType(self, type):
        if type.isEnum:
            return (type.namespace, self._typeName(type))
        elif type.isEnum:
            return (self._nsSchema, "string")
        elif type.isBuiltin and type.namespace != self._nsSchema:
            return (self._nsSchema, "string")
        else:
            return (type.namespace, type.schemaName)

    # Helper method to output a complex type
    def _outputStruct(self, out, type, bElement = False):
        if bElement:
            typeIndent = '  '
            typeAttr = ''
        else:
            typeIndent = ''
            typeAttr = ' name="%s"' % self._typeName(type)
        if not type.members:
            out.write('''\
%s      <s:complexType%s />
''' % (typeIndent, typeAttr))
        else:
            out.write('''\
%s      <s:complexType%s>
%s        <s:sequence>
''' % (typeIndent, typeAttr, typeIndent))
            for member in type.members:
                if member.isOptional:
                    minOccurs = "0"
                else:
                    minOccurs = "1"
                if type.isArray or member.isUnbounded:
                    maxOccurs = "unbounded"
                else:
                    maxOccurs = "1"
                typeNS, typeName = self._effectiveType(member.type)
                out.write('''\
%s          <s:element minOccurs="%s" maxOccurs="%s" name="%s" type="%s:%s" />
''' % (typeIndent, minOccurs, maxOccurs, member.name, self._nsPrefixes[typeNS], typeName))
            out.write('''\
%s        </s:sequence>
%s      </s:complexType>
''' % (typeIndent, typeIndent))

    #
    # Generate code
    #
    def generate(self, dir, fhReport):

        for file, fn, bGenerate in [(os.path.join(dir, self._baseName + '.wsdl'), self._generate_wsdl, True)]:

            if bGenerate:

                if fhReport is not None:
                    print >>fhReport, 'Generating "%s" ...' % file

                out = open(file, "w")
                try:
                    fn(out)
                except:
                    raise
                finally:
                    out.close()

    #
    # Generate WSDL
    #
    def _generate_wsdl(self, out):

        # WSDL header
        out.write('''\
<?xml version="1.0" encoding="utf-8"?>
<wsdl:definitions
''')
        for ns, nsPrefix in sorted(self._nsPrefixes.iteritems(), key = lambda x: x[1]):
            out.write('''\
    xmlns:%s="%s"
''' % (nsPrefix, ns))
        out.write('''\
    targetNamespace="%s">
''' % (HSLParser.HNAPNamespace))

        # WSDL types
        out.write('''\
  <wsdl:types>
''')
        for schemaNamespace in sorted(self._schemaNamespaces.iterkeys()):

            # Schema header
            out.write('''\
    <s:schema elementFormDefault="qualified" targetNamespace="%s">
''' % (schemaNamespace))

            # Import all other schema namespaces
            for schemaNamespaceOther in sorted(self._schemaNamespaces.iterkeys()):
                if schemaNamespaceOther != schemaNamespace:
                    out.write('''\
      <s:import namespace="%s" />
''' % (schemaNamespaceOther))

            # Schema enum types
            for type in self._types:
                if type.namespace == schemaNamespace:
                    if type.isEnum:
                        out.write('''\
      <s:simpleType name="%s">
        <s:restriction base="s:string">
''' % (self._typeName(type)))
                        for enumValue in type.enumValues:
                            out.write('''\
          <s:enumeration value="%s" />
''' % (enumValue))
                        out.write('''\
        </s:restriction>
      </s:simpleType>
''')

            # Schema struct types
            for type in self._types:
                if type.namespace == schemaNamespace:
                    if type.isStruct:
                        self._outputStruct(out, type)

            # Input/output elements
            for action in self._actions:
                if action.namespace == schemaNamespace:
                    for actionMember in (action.inputMember, action.outputMember):
                        out.write('''\
      <s:element name="%s">
''' % (actionMember.name))
                        self._outputStruct(out, actionMember.type, bElement = True)
                        out.write('''\
      </s:element>
''')

            # Schema footer
            out.write('''\
    </s:schema>
''')
        out.write('''\
  </wsdl:types>
''')

        # WSDL Messages
        for action in self._actions:
            out.write('''\
  <wsdl:message name="%s">
    <wsdl:part name="%s" element="%s:%s" />
  </wsdl:message>
  <wsdl:message name="%sResponse">
    <wsdl:part name="%sResponse" element="%s:%s" />
  </wsdl:message>
''' % (self._actionName(action),
       action.inputMember.name, self._nsPrefixes[action.inputMember.namespace], action.inputMember.name,
       self._actionName(action),
       action.inputMember.name, self._nsPrefixes[action.outputMember.namespace], action.outputMember.name))

        # WSDL Ports
        out.write('''\
  <wsdl:portType name="HNAP">
''')
        for action in self._actions:
            out.write('''\
    <wsdl:operation name="%s">
      <wsdl:input message="tns:%s" />
      <wsdl:output message="tns:%sResponse" />
    </wsdl:operation>
''' % (self._actionName(action), self._actionName(action), self._actionName(action)))
        out.write('''\
  </wsdl:portType>
''')

        # WSDL Bindings
        out.write('''\
  <wsdl:binding name="HNAP" type="tns:HNAP">
    <soap:binding transport="http://schemas.xmlsoap.org/soap/http" />
''')
        for action in self._actions:
            out.write('''\
    <wsdl:operation name="%s">
      <soap:operation soapAction="%s" style="document" />
      <wsdl:input>
        <soap:body use="literal" />
      </wsdl:input>
      <wsdl:output>
        <soap:body use="literal" />
      </wsdl:output>
    </wsdl:operation>
''' % (self._actionName(action), action.uri))
        out.write('''\
  </wsdl:binding>
''')

        # WSDL Services
        out.write('''\
  <wsdl:service name="HNAP">
    <wsdl:port name="HNAP" binding="tns:HNAP">
      <soap:address location="" />
    </wsdl:port>
  </wsdl:service>
''')

        # WSDL footer
        out.write('''\
</wsdl:definitions>
''')


######################################################################

if __name__ == "__main__":
    main()
