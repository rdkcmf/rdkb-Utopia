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

#
# unittest.py - UTCTX Library unit test runner
#

import optparse
import os
import platform
import re
import shutil
import subprocess
import sys
import tempfile
import unittest


#
# Main
#
def main():

    # Command line options
    cmdParser = optparse.OptionParser()
    cmdParser.add_option("-b", action = "store_true", dest = "bBuildOnly",
                         help = "Build unittest only (no run)")
    cmdParser.add_option("-r", action = "store_true", dest = "bRunOnly",
                         help = "Run unit tests only (no build)")
    cmdParser.add_option("-v", action = "store_true", dest = "bVerbose",
                         help = "Verbose output")
    cmdParser.add_option("-u", action = "store_true", dest = "bUpdateExpected",
                         help = "Update expected output")
    cmdParser.add_option("--debug", action = "store_true", dest = "bDebug",
                         help = "Build debug binaries")
    (cmdOptions, cmdArgs) = cmdParser.parse_args()

    # Test suites...
    suites = unittest.TestSuite()

    dirSrc = "src"
    dirTests = "tests"
    if not cmdOptions.bRunOnly:
        # Build test
        buildSuite = BuildTestSuite(dirSrc, cmdOptions.bDebug)
        suites.addTest(buildSuite)

    if not cmdOptions.bBuildOnly:
        # API test
        apiSuite = APITestSuite(dirSrc, dirTests, cmdOptions.bUpdateExpected)
        suites.addTest(apiSuite)

        # Transaction test
        transSuite = TransactionTestSuite(dirSrc, dirTests, cmdOptions.bUpdateExpected)
        suites.addTest(transSuite)

        # RWLock test
        rwlockSuite = RWLockTestSuite(dirSrc)
        suites.addTest(rwlockSuite)

    # Run the test suites
    runner = unittest.TextTestRunner()
    if cmdOptions.bVerbose:
        runner.verbosity = 2
    runner.run(suites)

#
# Helper function to determine if two files differ
#
def FilesDiffer(expected, actual, fnIgnore = None, fnErrorActual = None):

    # Ignore line endings on non-Linux systems
    bIgnoreLineEndings = (platform.system() != 'Linux')

    # Read the expected file
    fhExpected = open(expected, "r")
    try:
        expectedText = fhExpected.read()
    except:
        fhExpected.close()
        raise
    fhExpected.close()

    # Read the actual file
    fhActual = open(actual, "r")
    try:
        actualText = fhActual.read()
    except:
        fhActual.close()
        raise
    fhActual.close()

    # No reason split into lines?  If so, just compare the whole files...
    if fnIgnore is None and fnErrorActual is None and not bIgnoreLineEndings:
        return expectedText != actualText

    # Split the text into lines...
    expectedLines = expectedText.splitlines(not bIgnoreLineEndings)
    actualLines = actualText.splitlines(not bIgnoreLineEndings)

    # Check for errors in the actual text
    if fnErrorActual is not None:
        errors = [ x for x in actualLines if fnErrorActual(x) ]
        if errors:
            raise Exception("\n".join(errors))

    # Compare
    if fnIgnore is not None:
        expectedLines = [ x for x in expectedLines if not fnIgnore(x) ]
        actualLines = [ x for x in actualLines if not fnIgnore(x) ]
    return expectedLines != actualLines


######################################################################
#
# Build Test Suite
#
######################################################################


#
# Load the build test
#
class BuildTestSuite(unittest.TestSuite):

    def __init__(self, dirSrc, bDebug):
        unittest.TestSuite.__init__(self)

        for dirSrc, bDebug, cFlags in \
                [(dirSrc, bDebug, "-DUTCTX_LOG"),
                 (dirSrc, bDebug, "-DUTCTX_POSIX_SEM"),
                 (dirSrc, bDebug, "-DUTCTX_LOG -DUTCTX_POSIX_SEM"),
                 (dirSrc, bDebug, None)]:

            self.addTest(BuildTestCase(dirSrc, bDebug, cFlags))

#
# Build test case
#
class BuildTestCase(unittest.TestCase):

    def __init__(self, dirSrc, bDebug, cFlags):
        unittest.TestCase.__init__(self)
        self.__dirSrc = dirSrc
        self.__bDebug = bDebug
        self.__cFlags = cFlags

    def __str__(self):
        if self.__cFlags is None:
            return 'Build: '
        else:
            return 'Build UNITTEST_CFLAGS="%s":' % self.__cFlags

    def runTest(self):
        # Set the target
        if self.__bDebug:
            target = "debug"
        else:
            target = "release"

        # Setup the make command
        make = 'make clean %s' % (target)

        # Append cflags
        if self.__cFlags is not None:
            make += ' UNITTEST_CFLAGS="%s"' % self.__cFlags

        # Run make
        proc = subprocess.Popen(make, shell = True, stdout = subprocess.PIPE,
                                stderr = subprocess.STDOUT, cwd = self.__dirSrc)
        makeOutput = proc.communicate()[0]

        if proc.wait() != 0:
            if makeOutput is not None:
                self.fail(makeOutput)


######################################################################
#
# API Test Suite
#
######################################################################


#
# Load the API test
#
class APITestSuite(unittest.TestSuite):

    def __init__(self, dirSrc, dirTests, bUpdateExpected):
        unittest.TestSuite.__init__(self)

        for strDesc, dirSrc, dirTests, strState, bUpdateExpected in \
                [("get", dirSrc, dirTests, "utctx_api.st", bUpdateExpected),
                 ("set", dirSrc, dirTests, "utctx_api.st", bUpdateExpected),
                 ("unset", dirSrc, dirTests, "utctx_api.st", bUpdateExpected),
                 ("event", dirSrc, dirTests, "utctx_api.st", bUpdateExpected),
                 ("utopia-values", dirSrc, dirTests, "utctx.st", bUpdateExpected)]:

            self.addTest(APITestCase(strDesc, dirSrc, dirTests, strState, bUpdateExpected))

#
# API test case
#
class APITestCase(unittest.TestCase):

    def __init__(self, strDesc, dirSrc, dirTests, strState, bUpdateExpected):
        unittest.TestCase.__init__(self)
        self.__strDesc = strDesc
        self.__dirSrc = dirSrc
        self.__fileState = os.path.join("state", strState)
        self.__dirTest = os.path.join(dirTests, "API")
        self.__testProgram = os.path.join(dirSrc, "unittest")
        self.__mallocInterposer = os.path.join(dirSrc, "malloc_interposer.so")
        self.__bUpdateExpected = bUpdateExpected

        # Ignore malloc stats in expected/actual, if requested
        reIgnoreMallocStats = re.compile("^malloc_interposer\.c - [exit:|\*+]")
        self.__fnDiffIgnore = lambda s: reIgnoreMallocStats.search(s) is not None

        # Report errors for memory leaks
        reMemoryLeaks = re.compile("^malloc_interposer\.c - Memory leak:")
        self.__fnDiffError = lambda s: reMemoryLeaks.search(s) is not None

    def __str__(self):
        return "API: %s" % self.__strDesc

    def runTest(self):
        # Copy the state file
        shutil.copy(self.__fileState, os.path.join(self.__dirSrc, "utctx.st"))

        # Setup the cmd args
        args = [os.path.abspath(self.__testProgram), self.__strDesc, "commit"]

        # Setup files
        fileActual = os.path.join(self.__dirTest, self.__strDesc, "actual.out")
        fileExpected = os.path.join(self.__dirTest, self.__strDesc, "expected.out")

        # Execute the test program
        cmdLine = " ".join(args)
        fhOutput = open(fileActual, "w")
        try:
            proc = subprocess.Popen(args = args, stdout = fhOutput, cwd = self.__dirSrc,
                                    env = { "LD_PRELOAD": os.path.abspath(self.__mallocInterposer) })
            returnCode = proc.wait()
            if returnCode != 0:
                self.fail("Test failed with return code %s!\n%s" % (returnCode, cmdLine))
        except:
            fhOutput.close()
            raise
        fhOutput.close()

        # Diff the actual output with expected
        if not os.path.exists(fileExpected):
            self.fail("Expected output file %s does not exist" % fileExpected)
        elif FilesDiffer(fileExpected, fileActual, self.__fnDiffIgnore, self.__fnDiffError):
            if self.__bUpdateExpected:
                shutil.copy(fileActual, fileExpected)
            else:
                self.fail('Actual output differs from expected: "%s" "%s"\n%s' %
                          (fileActual, fileExpected, cmdLine))


######################################################################
#
# Transaction Test Suite
#
######################################################################


#
# Load the Transaction test
#
class TransactionTestSuite(unittest.TestSuite):

    def __init__(self, dirSrc, dirTests, bUpdateExpected):
        unittest.TestSuite.__init__(self)

        self.addTest(TransactionTestCase("commit", dirSrc, dirTests, True, bUpdateExpected))
        self.addTest(TransactionTestCase("rollback", dirSrc, dirTests, False, bUpdateExpected))

#
# Transaction test case
#
class TransactionTestCase(unittest.TestCase):

    def __init__(self, strDesc, dirSrc, dirTests, bCommit, bUpdateExpected):
        unittest.TestCase.__init__(self)
        self.__strDesc = strDesc
        self.__dirSrc = dirSrc
        self.__dirTest = os.path.join(dirTests, "Transaction")
        self.__testProgram = os.path.join(dirSrc, "unittest")
        self.__fileState = os.path.join("state", "utctx_api.st")
        self.__fileEndState = os.path.join(dirSrc, "utctx.st")
        self.__bCommit = bCommit
        self.__bUpdateExpected = bUpdateExpected

    def __str__(self):
        return "Transaction: %s" % self.__strDesc

    def runTest(self):
        # Init the end state file with the state
        shutil.copy(self.__fileState, self.__fileEndState)

        # Setup the cmd args
        args = [os.path.abspath(self.__testProgram), "set"]

        if self.__bCommit:
            args.extend(["commit"])

        # Execute the test program
        cmdLine = " ".join(args)
        fhOutput = tempfile.TemporaryFile()
        try:
            proc = subprocess.Popen(args = args, stdout = fhOutput, cwd = self.__dirSrc)
            returnCode = proc.wait()
        except:
            fhOutput.close()
            raise
        fhOutput.close()

        if returnCode != 0:
            self.fail("Test failed with return code %s!\n%s" % (returnCode, cmdLine))

        # Setup diff files
        fileActual = os.path.join(self.__dirTest, self.__strDesc, "actual.st")
        fileExpected = os.path.join(self.__dirTest, self.__strDesc, "expected.st")

        # Copy the end state to the actual state file
        shutil.copy(self.__fileEndState, fileActual)

        # Diff the actual output with expected
        if not os.path.exists(fileExpected):
            self.fail("Expected state file %s does not exist" % fileExpected)
        elif FilesDiffer(fileExpected, fileActual):
            if self.__bUpdateExpected:
                shutil.copy(fileActual, fileExpected)
            else:
                self.fail('Actual state differs from expected: "%s" "%s"\n%s' %
                          (fileActual, fileExpected, cmdLine))


######################################################################
#
# RWLock Test Suite
#
######################################################################


#
# Load the RWLock test
#
class RWLockTestSuite(unittest.TestSuite):

    def __init__(self, dirSrc):
        unittest.TestSuite.__init__(self)

        self.addTest(RWLockTestCase(dirSrc))

#
# RWLock test case
#
class RWLockTestCase(unittest.TestCase):

    def __init__(self, dirSrc):
        unittest.TestCase.__init__(self)
        self.__dirSrc = dirSrc
        self.__testProgram = os.path.join(dirSrc, "rwlock_unittest")

    def __str__(self):
        return "RWLock:"

    def runTest(self):
        fhOutput = tempfile.TemporaryFile()
        args = os.path.abspath(self.__testProgram)

        proc = subprocess.Popen(args, shell = True, stdout = subprocess.PIPE, stderr = subprocess.STDOUT, cwd = self.__dirSrc)
        makeOutput = proc.communicate()[0]

        if proc.wait() != 0:
            if makeOutput is not None:
                self.fail(makeOutput)


######################################################################

if __name__ == "__main__":
    main()
