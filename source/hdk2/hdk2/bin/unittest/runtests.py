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

import datetime
import optparse
import os
import platform
import re
import shutil
import subprocess
import sys
import unittest

# Import HDK modules
sys.path.append(os.path.join(os.path.dirname(sys.argv[0]), "..", "lib"))
from hdk.hslparser import HSLParser
from hdk.testutil import ActualDiffers


#
# Main
#
def main():

    # Command line options
    cmdParser = optparse.OptionParser()
    cmdParser.add_option("-t", action = "append", dest = "testsIncluded",
                         help = "Run specified tests", metavar = "test")
    cmdParser.add_option("-u", action = "store_true", dest = "bUpdateExpected",
                         help = "Update expected output")
    cmdParser.add_option("--code-release", action = "store_true", dest = "bCodeRelease",
                         help = "Code cleanliness release tests")
    (cmdOptions, cmdArgs) = cmdParser.parse_args()

    # The unittest directory
    unittestDir = os.path.dirname(sys.argv[0])
    if not unittestDir:
        unittestDir = '.'

    # Test suites...
    suites = unittest.TestSuite()

    # Code cleanliness tests
    dirHDK = os.path.join(unittestDir, "..", "..")
    codeSuite = CodeSuite(dirHDK, cmdOptions.testsIncluded, cmdOptions.bCodeRelease)
    suites.addTest(codeSuite)

    # HSLParser tests
    dirTestsParser = os.path.join(unittestDir, "tests-hsl")
    parserSuite = HSLParserSuite(dirTestsParser, cmdOptions.testsIncluded, cmdOptions.bUpdateExpected)
    suites.addTest(parserSuite)

    # Code generation tests
    dirTestsGen = os.path.join(unittestDir, "tests-gen")
    genSuite = CodeGenerationSuite(dirTestsGen, cmdOptions.testsIncluded, cmdOptions.bUpdateExpected)
    suites.addTest(genSuite)

    # Run the test suites
    runner = unittest.TextTestRunner(verbosity = 2)
    if not runner.run(suites).wasSuccessful():
        return 1

    # Success
    return 0


######################################################################
#
# Code cleanliness tests
#
######################################################################

class CodeSuite(unittest.TestSuite):

    def __init__(self, dirCode, testNames, bCodeRelease):

        unittest.TestSuite.__init__(self)

        # Add the code cleanliness test
        if not testNames or "code" in testNames:
            self.addTest(CodeTest(dirCode, bCodeRelease))


class CodeTest(unittest.TestCase):

    def __init__(self, dirBase, bCodeRelease):

        unittest.TestCase.__init__(self)

        self.__dirBase = dirBase
        self.__dirOrig = os.getcwd()
        self.__bCodeRelease = bCodeRelease

    def __str__(self):

        return "Code Cleanliness"

    def setUp(self):

        os.chdir(self.__dirBase)

    def tearDown(self):

        os.chdir(self.__dirOrig)

    def runTest(self):

        errors = []

        # Regex
        reSource = re.compile("\.(c|cpp|h|py|hsl)$", re.IGNORECASE)
        reMakefile = re.compile("^Makefile[^~]*$", re.IGNORECASE)
        reUnittestFile = re.compile("bin[/\\\\]unittest[/\\\\]tests-.*?\.hsl", re.IGNORECASE)
        reRootIgnore = re.compile("([/\\\\]|\A)\.svn([/\\\\]|\Z)", re.IGNORECASE)

        # Find all source files
        for root, dirs, files in os.walk('.'):

            # Ignore files based on root?
            if reRootIgnore.search(root):
                continue

            for file in files:
                bSource = reSource.search(file)
                bMakefile = reMakefile.search(file)
                if bSource or bMakefile:
                    path = os.path.join(root, file)

                    # Check tabs?
                    bCheckTabs = not bMakefile

                    # Unittest source file?
                    bUnittestFile = reUnittestFile.search(path) is not None

                    # Check code cleanliness
                    errors.extend(self._checkCodeCleanliness(path, bCheckTabs, bUnittestFile))

        # Any errors?
        if errors:
            self.fail("\n".join(errors))

    def _checkCodeCleanliness(self, file, bCheckTabs, bUnittestFile):

        errors = []

        # Regex
        reWhitespaceEOL = re.compile("[ \t]+$")
        reTabs = re.compile("\t")
        reBlank = re.compile("^\s*$")
        reCopyright = re.compile("^( \*|//|#) Copyright\s+\(c\) (\d{4}-)?(?P<year>\d{4}) Cisco Systems, Inc.")
        reTodo = re.compile("\\btodo\\b", re.IGNORECASE)

        # Iterate each line in the file
        fh = open(file, "rb")
        try:
            ixLine = 0
            nBlank = 0
            nCopyright = 0
            for line in fh.readlines():
                ixLine += 1

                # Check whitespace at end-of-line
                if reWhitespaceEOL.search(line):
                    errors.append("%s:%d: End-of-line whitespace" % (file, ixLine))

                # Check for tabs
                if bCheckTabs and reTabs.search(line):
                    errors.append("%s:%d: Tab character" % (file, ixLine))

                # Check for non-ASCII characters
                try:
                    line.encode('ascii')
                except:
                    errors.append("%s:%d: Non-ASCII character" % (file, ixLine))

                # Count contiguous blank lines
                if reBlank.search(line):
                    nBlank += 1
                else:
                    nBlank = 0

                # Count copyright lines
                mCopyright = reCopyright.search(line)
                if mCopyright:
                    nCopyright += 1

                    # Check the year
                    if self.__bCodeRelease:
                        copyrightYear = int(mCopyright.group("year"))
                        currentYear = datetime.datetime.now().year
                        if copyrightYear != currentYear:
                            errors.append("%s:%d: Incorrect copyright year" % (file, ixLine))

                # Check for to-do's
                if self.__bCodeRelease and reTodo.search(line):
                    errors.append("%s:%d: To-do" % (file, ixLine))

            # Check for extra blank lines at end of file
            if nBlank > 0:
                errors.append("%s:%d: Extra blank lines at end of file" % (file, ixLine))

            # Check for copyright lines
            if not bUnittestFile and nCopyright < 1:
                errors.append("%s:%d: No copyright line found" % (file, ixLine))

        except:
            raise
        finally:
            fh.close()

        return errors


######################################################################
#
# HSL Specification Language (HSL) Parser Tests
#
######################################################################

class HSLParserSuite(unittest.TestSuite):

    def __init__(self, dirTests, testNames, bUpdateExpected):

        unittest.TestSuite.__init__(self)

        # Add the test directories
        for testName in sorted(os.listdir(dirTests)):

            dirTest = os.path.join(dirTests, testName)
            if os.path.isdir(dirTest) and testName[:1] != '.':

                if not testNames or testName in testNames:
                    self.addTest(HSLParserTest(dirTests, testName, bUpdateExpected))


class HSLParserTest(unittest.TestCase):

    def __init__(self, dirBase, dirTest, bUpdateExpected):

        unittest.TestCase.__init__(self)

        self.__dirOrig = os.getcwd()
        self.__dirBase = dirBase
        self.__dirTest = dirTest
        self.__bUpdateExpected = bUpdateExpected

    def __str__(self):

        return "HSLParser: " + self.__dirTest

    def setUp(self):

        os.chdir(self.__dirBase)

    def tearDown(self):

        os.chdir(self.__dirOrig)

    def runTest(self):

        # Parse the HSL files
        parser = LoadTestHSLParser(self.__dirTest)

        # Dump actions, types, and errors to actual file
        actual = os.path.join(self.__dirTest, "actual.txt")
        fh = open(actual, "w")
        try:
            DumpHSLParser(fh, parser)
        except:
            fh.close()
            raise
        fh.close()

        # Compare actual with expected
        expected = os.path.join(self.__dirTest, "expected.txt")
        result = ActualDiffers(actual, expected, self.__bUpdateExpected)
        if result:
            self.fail(result)


######################################################################
#
# Code Generation Tests
#
######################################################################

class CodeGenerationSuite(unittest.TestSuite):

    def __init__(self, dirTests, testNames, bUpdateExpected):

        unittest.TestSuite.__init__(self)

        # Add the test directories
        for testName in os.listdir(dirTests):
            dirTest = os.path.join(dirTests, testName)
            if os.path.isdir(dirTest) and testName[:1] != '.':
                if testNames is None or testName in testNames:
                    self.addTest(CodeGenerationTest(dirTests, testName, bUpdateExpected))


class CodeGenerationTest(unittest.TestCase):

    # Default test values
    __testSpec = "test.spec"

    def __init__(self, dirBase, dirTest, bUpdateExpected):

        unittest.TestCase.__init__(self)

        self.__dirOrig = os.getcwd()
        self.__dirBase = dirBase
        self.__dirTest = os.path.join(dirBase, dirTest)
        self.__bUpdateExpected = bUpdateExpected

        # Test state
        self.__testCases = []

    def __str__(self):

        return "Code Generation: " + self.__dirTest

    # Parse the test spec
    def __parse_test_spec(self):

        # Check for a .spec file first
        testSpec = os.path.join(self.__dirTest, self.__testSpec)
        if os.path.exists(testSpec):

            reComment = re.compile("^\s*(#.*)?$")
            reTestCase = re.compile("^test-case:\s*(\S+)\s*#*.*$")
            reCmd = re.compile('^\s+cmd:([^#]+)#*.*$')
            reDiff = re.compile("^\s+diff\s*#*.*$")
            reDiffFile = re.compile('^\s+(actual|expected):\s*(\S+)\s*#*.*$')

            # Parse test cases from spec file
            fhTestSpec = open(testSpec, "r")
            try:
                lineNo = 0
                for line in fhTestSpec:
                    lineNo = lineNo + 1

                    # Ignore comment lines
                    if reComment.search(line):
                        continue

                    # Handle a test-case match
                    elif reTestCase.search(line):

                        # Check for duplicate
                        testCaseName = reTestCase.search(line).group(1)
                        if [ testCase for testCase in self.__testCases if testCase["test-case"] == testCaseName ]:
                            self.fail("%s:%d: Duplicate test cases: %s" % (testSpec, line, testCaseName))

                        self.__testCases.append({"test-case": testCaseName})

                    # Handle diff set
                    elif reDiff.search(line):

                        if not self.__testCases:
                            self.fail("%s:%d: Attempting to set diff set to null testCase" % (testSpec, lineNo))

                        if not self.__testCases[-1].has_key("diff"):
                            self.__testCases[-1]["diff"] = []

                        self.__testCases[-1]["diff"].append({})

                    # Handle diff file
                    elif reDiffFile.search(line):

                        attrKey = reDiffFile.search(line).group(1)
                        attrFile = reDiffFile.search(line).group(2)

                        if not self.__testCases:
                            self.fail("%s:%d: Attempting to set diff file set to null testCase" % (testSpec, lineNo))

                        if self.__testCases[-1]["diff"] is None:
                            self.fail("%s:%d: Attempting to set %s file to null diff set %s" % (testSpec, lineNo, attrKey))

                        if self.__testCases[-1]["diff"][-1].has_key(attrKey):
                            self.fail("%s:%d: Redefinition of %s file value in diff set" % (testSpec, lineNo, attrKey))

                        self.__testCases[-1]["diff"][-1][attrKey] = attrFile

                    # Handle cmd match
                    elif reCmd.search(line):

                        cmd = reCmd.search(line).group(1)

                        # Check validity
                        if not self.__testCases:
                            self.fail("%s:%d: Attempting to set cmd to null testCase" % (testSpec, lineNo))

                        if not self.__testCases[-1].has_key("cmd"):
                            self.__testCases[-1]["cmd"] = cmd
                        else:
                            self.fail("%s:%d: Redefinition of test-case cmd" % (testSpec, lineNo))

                    # Syntax error
                    else:
                        self.fail("Syntax error in %s: %d" % (testSpec, lineNo))

            except:
                fhTestSpec.close()
                raise
            fhTestSpec.close()

        else: # No test.spec file

            self.fail("No test.spec specified (expected %s)" % os.path.abspath(testSpec))

        # Provide reasonable defaults
        for testCase in self.__testCases:
            if not testCase.has_key("cmd"):
                self.fail("test-case %s missing cmd value" % testCase["test-case"])
            if not testCase.has_key("diff"):
                testCase["diff"] = []

        # If there are no test cases to execute raise an exception
        if not self.__testCases:
            self.fail("%s: No test cases to execute" % (testSpec))

    def runTest(self):

        # Parse the test spec
        self.__parse_test_spec()

        # Run the test cases
        for testCase in self.__testCases:

            expectedOutput =  "expected_" + testCase["test-case"] + "_output.txt"
            actualOutput = "actual_" + testCase["test-case"] + "_output.txt"

            # Convert '/' to '\' on Windows systems
            if platform.system() == 'Windows':
                cmd = testCase["cmd"].replace('/', '\\')
            else:
                cmd = testCase["cmd"]

            # Execute the command
            proc = subprocess.Popen(cmd, shell = True, stdout = subprocess.PIPE, stderr = subprocess.STDOUT, cwd = self.__dirTest)
            output = proc.communicate()[0]
            result = proc.wait()

            # Replace '\' so directory path output is consistent across platforms
            output = output.replace('\\', '/')

            fh = open(os.path.join(self.__dirTest, actualOutput), "wb")
            try:
                print >>fh, "result = %d" % (result)
                print >>fh, output
            except:
                fh.close()
                raise
            fh.close()

            # Always include the tool output in the diff
            diffFileSet = [
                (os.path.join(self.__dirTest, actualOutput), os.path.join(self.__dirTest, expectedOutput))
            ]

            for diffDict in testCase["diff"]:
                actualFile = os.path.join(self.__dirTest, diffDict["actual"])
                expectedFile = os.path.join(self.__dirTest, diffDict["expected"])
                diffFileSet.append((actualFile, expectedFile))

            for (actual, expected) in diffFileSet:
                result = ActualDiffers(actual, expected, self.__bUpdateExpected)
                if result:
                    self.fail(result)


######################################################################
#
# Utilities
#
######################################################################

#
# Find the code generation test's HSL files
#
def GetHSLTestFiles(dirTest):

    # Iterate the test directory entries...
    reHSL = re.compile("(?!-noparse)\.hsl$")
    hslFiles = []
    importPath = []
    for dirEntry in os.listdir(dirTest):
        pathEntry = os.path.join(dirTest, dirEntry)

        # Parse only file with .hsl extension
        if os.path.isfile(pathEntry) and reHSL.search(pathEntry):
            hslFiles.append(pathEntry)

        # Add sub-directories to the import path
        if os.path.isdir(pathEntry):
            importPath.append(pathEntry)

    if not hslFiles:
        raise Exception("No HSL files found")

    return (sorted(hslFiles), sorted(importPath))

#
# Load a parser test object
#
def LoadTestHSLParser(dirTest):

    # Create the parser
    (hslFiles, importPath) = GetHSLTestFiles(dirTest)
    parser = HSLParser(importPath = importPath)

    # Parse the HSL files
    for hslFile in hslFiles:
        parser.parseFile(hslFile)
    parser.parseDone()

    return parser

#
# Load a parser test object
#
def ExecCodeGenTool(tool, args, dirTest, outputFile = None):

    # Format the command string
    (hslFiles, importPath) = GetHSLTestFiles(dirTest)
    cmd = " ".join((os.path.join(dirTest, "..", "..", "..", tool),
                    " ".join(args),
                    " ".join(["-I " + importDir for importDir in importPath ]),
                    " ".join(hslFiles)))

    # Execute the command
    proc = subprocess.Popen(cmd, shell = True, stdout = subprocess.PIPE, stderr = subprocess.STDOUT)
    output = proc.communicate()[0]
    result = proc.wait()

    # Replace '\' so directory path output is consistent across platforms
    output = output.replace('\\', '/')

    # Write the output
    if outputFile:
        fh = open(os.path.join(dirTest, outputFile), "wb")
        try:
            print >>fh, "result = %d" % (result)
            print >>fh, output
        except:
            fh.close()
            raise
        fh.close()

#
# Dump HSL parser state
#
def DumpHSLParser(fh, parser):

    model = parser.getmodel()

    # Merge types and all_types
    types = {}
    all_types = {}
    for type in model.types:
        types[type.uri] = type
        all_types[type.uri] = type
    for type in model.all_types(user_types = True, state_types = True):
        all_types[type.uri] = type

    if model.actions:
        print >>fh, "Actions:"
        for action in model.actions:
            DumpHSLAction(fh, action, 2)

    if all_types:
        print >>fh
        print >>fh, "Types:"
        for type in sorted(all_types.itervalues()):
            DumpHSLType(fh, type, 2, types)

    if model.states:
        print >>fh
        print >>fh, "States:"
        for state in model.states:
            DumpHSLState(fh, state, 2)

    if model.actionStates:
        print >>fh
        print >>fh, "ActionStates:"
        for state in model.actionStates:
            DumpHSLActionState(fh, state, 2)

    if model.services:
        print >>fh
        print >>fh, "Services:"
        for service in model.services:
            DumpHSLService(fh, service, 2)

    if model.docSections:
        print >>fh
        print >>fh, "DocSections:"
        for docSection in model.docSections:
            DumpHSLDocSection(fh, docSection, 2)

    if parser.errors:
        print >>fh
        print >>fh, "Errors:"
        print >>fh
        for error in parser.errors:
            print >>fh, "  " + error

#
# Dump HSL action object state
#
def DumpHSLAction(fh, action, cIndent = 0):
    indent = " " * cIndent
    print >>fh
    print >>fh, indent + action.uri
    print >>fh
    print >>fh, indent + "  namespace:", action.namespace
    print >>fh, indent + "  name:", action.name
    DumpHSLDoctext(fh, action, cIndent)
    print >>fh, indent + "  resultMember:", action.resultMember.uri
    print >>fh, indent + "  inputMember:"
    DumpHSLMember(fh, action.inputMember, cIndent + 4)
    print >>fh
    print >>fh, indent + "  inputMember.type:"
    DumpHSLType(fh, action.inputMember.type, cIndent + 4)
    print >>fh
    print >>fh, indent + "  outputMember:"
    DumpHSLMember(fh, action.outputMember, cIndent + 4)
    print >>fh
    print >>fh, indent + "  outputMember.type:"
    DumpHSLType(fh, action.outputMember.type, cIndent + 4)

#
# Dump HSL state object state
#
def DumpHSLState(fh, state, cIndent = 0):
    indent = " " * cIndent
    print >>fh
    print >>fh, indent + state.uri
    print >>fh
    print >>fh, indent + "  namespace:", state.namespace
    print >>fh, indent + "  name:", state.name
    DumpHSLDoctext(fh, state, cIndent)
    print >>fh, indent + "  type:", state.type.uri

#
# Dump HSL action_state object state
#
def DumpHSLActionState(fh, actionState, cIndent = 0):
    indent = " " * cIndent
    print >>fh
    print >>fh, indent + actionState.uri
    print >>fh
    print >>fh, indent + "  namespace:", actionState.namespace
    print >>fh, indent + "  name:", actionState.name
    DumpHSLDoctext(fh, actionState, cIndent)
    if actionState.stateMembers:
        print >>fh, indent + "  stateMembers:"
        for state in sorted(actionState.stateMembers.itervalues()):
            DumpHSLStateMember(fh, state, cIndent + 4)

#
# Dump HSL state member object state
#
def DumpHSLStateMember(fh, stateMember, cIndent = 0):
    indent = " " * cIndent
    print >>fh
    print >>fh, indent + stateMember.uri
    print >>fh
    print >>fh, indent + "  namespace:", stateMember.namespace
    print >>fh, indent + "  name:", stateMember.name
    DumpHSLDoctext(fh, stateMember, cIndent)
    print >>fh, indent + "  state:", stateMember.state.uri
    print >>fh, indent + "  isGet:", stateMember.isGet
    print >>fh, indent + "  isSet:", stateMember.isSet

#
# Dump HSL service state
#
def DumpHSLService(fh, service, cIndent = 0):
    indent = " " * cIndent
    print >>fh
    print >>fh, indent + service.uri
    print >>fh
    print >>fh, indent + "  namespace:", service.namespace
    print >>fh, indent + "  name:", service.name
    DumpHSLDoctext(fh, service, cIndent)

    serviceActions = [action for action in service.actions.itervalues() if action is not None]
    if serviceActions:
        print >>fh
        print >>fh, indent + "  actions:"
        for action in sorted(serviceActions):
            DumpHSLServiceObject(fh, action, cIndent + 4)

    serviceEvents = [event for event in service.events.itervalues() if event is not None]
    if serviceEvents:
        print >>fh
        print >>fh, indent + "  events:"
        for event in sorted(serviceEvents):
            DumpHSLServiceObject(fh, event, cIndent + 4)

#
# Dump HSL service object state
#
def DumpHSLServiceObject(fh, object, cIndent = 0):
    indent = " " * cIndent
    if object is not None:
        print >>fh, indent + object.uri

#
# Dump HSL type object state
#
def DumpHSLType(fh, type, cIndent = 0, types = None):
    indent = " " * cIndent

    if types is not None and type.uri not in types:
        all_types = " (all_types)"
    else:
        all_types = ""

    print >>fh
    print >>fh, indent + type.uri + all_types
    print >>fh
    print >>fh, indent + "  namespace:", type.namespace
    print >>fh, indent + "  name:", type.name
    DumpHSLDoctext(fh, type, cIndent)
    print >>fh, indent + "  schemaName:", type.schemaName
    print >>fh, indent + "  parserOrder:", type.parserOrder
    print >>fh, indent + "  isBuiltin:", type.isBuiltin
    print >>fh, indent + "  isArray:", type.isArray
    print >>fh, indent + "  isStruct:", type.isStruct
    print >>fh, indent + "  isEnum:", type.isEnum
    if type.isArray:
        print >>fh, indent + "  arrayType:", type.arrayType.uri
    if type.isStruct:
        print >>fh, indent + "  members:"
        for member in type.members:
            DumpHSLMember(fh, member, cIndent + 4)
    if type.isEnum:
        print >>fh, indent + "  enumValues:"
        print >>fh
        for enumValue in type.enumValues:
            print >>fh, indent + "    \"" + enumValue + "\""

        # Any enum doctext?
        if [ev for ev in type.enumValuesEx if ev.doctext]:
            print >>fh
            print >>fh, indent + "  enumValuesEx:"
            print >>fh
            for enumValueEx in type.enumValuesEx:
                print >>fh, indent + "    \"" + enumValueEx.name + "\""
                DumpHSLDoctext(fh, enumValueEx, cIndent + 4)
#
# Dump HSL member object state
#
def DumpHSLMember(fh, member, cIndent = 0):
    indent = " " * cIndent
    print >>fh
    print >>fh, indent + member.uri
    print >>fh
    print >>fh, indent + "  name:", member.name
    print >>fh, indent + "  namespace:", member.namespace
    DumpHSLDoctext(fh, member, cIndent)
    print >>fh, indent + "  type:", member.type.uri
    print >>fh, indent + "  isUnbounded:", member.isUnbounded
    print >>fh, indent + "  isOptional:", member.isOptional
    print >>fh, indent + "  isCSV:", member.isCSV
    print >>fh, indent + "  isErrorOutput:", member.isErrorOutput

#
# Dump the HSL model's doc sections
#
def DumpHSLDocSection(fh, docSection, cIndent = 0):
    indent = " " * cIndent
    print >>fh
    print >>fh, indent + "  type:", docSection.type
    print >>fh, indent + "  title:", docSection.title
    DumpHSLDoctext(fh, docSection, cIndent)

#
# Dump an HSL object's doctext state
#
def DumpHSLDoctext(fh, hslobj, cIndent = 0):
    if hslobj.doctext:
        indent = " " * cIndent
        print >>fh, indent + "  doctext:"
        for line in hslobj.doctext:
            print >>fh, indent + "    " + line


######################################################################

if __name__ == "__main__":
    sys.exit(main())
