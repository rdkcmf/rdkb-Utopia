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

import optparse
import os
import re
import subprocess
import sys
import unittest
import platform

# Import HDK modules
sys.path.append(os.path.join(os.path.dirname(sys.argv[0]), "..", "bin", "lib"))
from hdk.testutil import ActualDiffers


#
# Main
#
def main():

    # Command line options
    cmdParser = optparse.OptionParser("usage: %prog [options] <module> <tests-dir>")
    cmdParser.add_option("-p", action = "store", dest = "unittest",
                         help = "Unittest program")
    cmdParser.add_option("-l", action = "append", dest = "library_path",
                         help = "Library path", default = [])
    cmdParser.add_option("-m", action = "store_true", dest = "bCheckMallocStats",
                         help = "Check malloc statistics")
    cmdParser.add_option("-t", action = "append", dest = "testsIncluded",
                         help = "Run specified tests", metavar = "test")
    cmdParser.add_option("-u", action = "store_true", dest = "bUpdateExpected",
                         help = "Update expected output")
    (cmdOptions, cmdArgs) = cmdParser.parse_args()

    # Check the positional arguments
    if len(cmdArgs) != 2:
        cmdParser.print_help()
        sys.exit(-1)
    serverModule, dirTests = cmdArgs

    # Location defaults
    if cmdOptions.unittest is None:
        cmdOptions.unittest = os.path.join(os.path.dirname(sys.argv[0]), "..", "build", "unittest", "unittest")

    bWindowsPlatform = (platform.system() == 'Windows')
    bDarwinPlatform = (platform.system() == 'Darwin')

    if bWindowsPlatform:
        mallocInterposer = None
    elif bDarwinPlatform:
        mallocInterposer = os.path.join(os.path.dirname(cmdOptions.unittest), "malloc_interposer.dylib")
    else: # assume linux platform
        mallocInterposer = os.path.join(os.path.dirname(cmdOptions.unittest), "malloc_interposer.so")

    # Add the ADI tests
    suite = ADITestSuite(cmdOptions.unittest, mallocInterposer, cmdOptions.library_path,
                         serverModule, dirTests, cmdOptions.testsIncluded,
                         cmdOptions.bUpdateExpected, not cmdOptions.bCheckMallocStats)

    # Run the test suites
    runner = unittest.TextTestRunner(verbosity = 2)
    if not runner.run(suite).wasSuccessful():
        return 1

    # Success
    return 0


######################################################################
#
# ADI Test Suite
#
######################################################################

#
# ADI test suite
#
class ADITestSuite(unittest.TestSuite):

    def __init__(self, testProgram, mallocInterposer, libraryPath, serverModule,
                 dirTests, testNames, bUpdateExpected, bIgnoreMallocStats):

        unittest.TestSuite.__init__(self)

        # Get the test names (if not provided)
        if not testNames:

            testNames = []
            for testName in os.listdir(dirTests):
                dirTest = os.path.join(dirTests, testName)
                if os.path.isdir(dirTest) and testName[0] != '.':
                    testNames.append(testName)

        # Add the test cases
        for testName in sorted(testNames):

            dirTest = os.path.join(dirTests, testName)
            self.addTest(ADITestCase(testProgram, mallocInterposer, libraryPath, serverModule,
                                     dirTest, bUpdateExpected, bIgnoreMallocStats))


#
# ADI test case
#
class ADITestCase(unittest.TestCase):

    # Default test values
    __defMethod = "POST"
    __defPath = "/HNAP1/"
    __defStartState = os.path.join("..", "..", "state", "default.ds")
    __requestExt = ".request"
    __responseExt = ".response"
    __testSpec = "test.spec"
    __stateExt = ".ds"

    # Used by the parser to validate parsed attributes
    __setAttributeKeys = [ "method", "path", "request", "response",
                           "start-state", "end-state",
                           "diff-state", "diff-response" ]


    # Test friendly name
    def __str__(self):
        return os.path.basename(self.__dirTest)


    # Class initializer
    def __init__(self, testProgram, mallocInterposer, libraryPath, serverModule,
                 dirTest, bUpdateExpected, bIgnoreMallocStats):

        unittest.TestCase.__init__(self)

        # Provided state
        self.__testProgram = testProgram
        self.__mallocInterposer = mallocInterposer
        self.__libraryPath = libraryPath
        self.__serverModule = serverModule
        self.__dirTest = dirTest
        self.__bUpdateExpected = bUpdateExpected
        self.__bIgnoreMallocStats = bIgnoreMallocStats

        # Test state
        self.__dirExpected = os.path.join(self.__dirTest, "expected")
        self.__dirActual = os.path.join(self.__dirTest, "actual")
        self.__testCases = []


    # Parse the test case
    def __parse_test_case(self):

        # Check for a .spec file first
        testSpec = os.path.join(self.__dirTest, self.__testSpec)
        if os.path.exists(testSpec):

            reComment = re.compile("^\s*(#.*)?$")
            reTestCase = re.compile("^test-case:\s*(\S+)\s*#*.*$")
            reAttr = re.compile("^\s+(\S+):\s*(\S+)\s*#*.*$")

            # Parse test cases from spec file
            fhTestSpec = open(testSpec, "r")
            try:
                for line in fhTestSpec:

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

                    # Handle attribute match
                    elif reAttr.search(line):

                        attrKey = reAttr.search(line).group(1)
                        attrValue = reAttr.search(line).group(2)

                        # Check validity
                        if attrKey not in self.__setAttributeKeys:
                            self.fail("%s:%d: Invalid attribute key: %s" % (testSpec, line, attrKey))
                        elif not self.__testCases:
                            self.fail("%s:%d: Attempting to set attribute to null testCase" % (testSpec, line))

                        self.__testCases[-1][attrKey] = attrValue

                    # Syntax error
                    else:
                        self.fail("Syntax error in %s: %d" % (testSpec, line))

            except:
                fhTestSpec.close()
                raise
            fhTestSpec.close()

        else: # No test.spec file

            # Iterate over the .request files
            for fileRequest in os.listdir(self.__dirTest):

                testCaseName, testCaseExt = os.path.splitext(fileRequest)
                if testCaseExt == self.__requestExt:

                    self.__testCases.append({"test-case": testCaseName})

        # Provide reasonable defaults
        for testCase in self.__testCases:
            if not testCase.has_key("method"):
                testCase["method"] = self.__defMethod
            if not testCase.has_key("path"):
                testCase["path"] = self.__defPath
            if not testCase.has_key("request"):
                testCase["request"] = testCase["test-case"] + self.__requestExt
            if not testCase.has_key("response"):
                testCase["response"] = testCase["test-case"] + self.__responseExt
            if not testCase.has_key("start-state"):
                testCase["start-state"] = testCase["test-case"] + self.__stateExt
                if not os.path.exists(os.path.join(self.__dirTest, testCase["start-state"])):
                    testCase["start-state"] = self.__defStartState
            if not testCase.has_key("end-state"):
                testCase["end-state"] = testCase["test-case"] + self.__stateExt
            if not testCase.has_key("diff-state"):
                testCase["diff-state"] = "false"
            if not testCase.has_key("diff-response"):
                testCase["diff-response"] = "true"

        # If there are no test cases to execute raise an exception
        if not self.__testCases:
            self.fail("%s: No test cases to execute" % (testSpec))


    # Run the test
    def runTest(self):

        # Parse the test case
        self.__parse_test_case()

        # Run the test cases
        for testCase in self.__testCases:

            # Set up the file paths
            fileRequest = os.path.join(self.__dirTest, testCase["request"])
            fileActualResponse = os.path.join(self.__dirActual, testCase["response"])
            fileExpectedResponse = os.path.join(self.__dirExpected, testCase["response"])
            fileStartState = os.path.join(self.__dirTest, testCase["start-state"])
            fileActualEndState = os.path.join(self.__dirActual, testCase["end-state"])
            fileExpectedEndState = os.path.join(self.__dirExpected, testCase["end-state"])

            # Set up the command args
            args = [ os.path.abspath(self.__testProgram), os.path.abspath(self.__serverModule),
                     testCase["method"].upper(), testCase["path"] ]
            if testCase["method"].lower() == "post":
                if not os.path.exists(fileRequest):
                    self.fail("Request file %s does not exist" % (fileRequest))
                args.append(os.path.abspath(fileRequest))
            else:
                args.append("-")
            args.extend([ os.path.abspath(fileStartState), os.path.abspath(fileActualEndState) ])
            cmd = " ".join(args)

            # Ensure the actual directory exists
            if not os.path.isdir(self.__dirActual):
                os.makedirs(self.__dirActual)

            # Execute request
            fhResponse = open(fileActualResponse, "w")
            try:

                if platform.system() == 'Windows':
                    env = { 'SystemRoot': os.environ['SystemRoot'], # SystemRoot is required for SxS c runtime
                            'PATH': ";".join([os.path.abspath(p) for p in self.__libraryPath]) }
                elif platform.system() == 'Darwin':
                    env = { 'DYLD_INSERT_LIBRARIES': os.path.abspath(self.__mallocInterposer),
                            'DYLD_LIBRARY_PATH': ":".join([os.path.abspath(p) for p in self.__libraryPath]) }
                else:
                    env = { 'LD_PRELOAD': os.path.abspath(self.__mallocInterposer),
                            'LD_LIBRARY_PATH': ":".join([os.path.abspath(p) for p in self.__libraryPath]) }

                proc = subprocess.Popen(args = args, stdout = fhResponse, cwd = self.__dirActual,
                                        env = env)
                returnCode = proc.wait()
                if returnCode != 0:
                    self.fail("Test failed with return code %s!\n%s" % (returnCode, cmd))
            except:
                fhResponse.close()
                raise
            fhResponse.close()

            # Diff the response file
            if testCase["diff-response"].lower() == "true":
                result = ActualDiffers(fileActualResponse, fileExpectedResponse, self.__bUpdateExpected,
                                       self.__bIgnoreMallocStats)
                if result:
                    self.fail("\n".join((cmd, result)))

            # Diff the output state file
            if testCase["diff-state"].lower() == "true":
                result = ActualDiffers(fileActualEndState, fileExpectedEndState, self.__bUpdateExpected,
                                       self.__bIgnoreMallocStats)
                if result:
                    self.fail("\n".join((cmd, result)))


######################################################################

if __name__ == "__main__":
    sys.exit(main())
