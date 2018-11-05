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
#!/usr/bin/python

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
# unittest.py - HDK client library unit tests
#

import optparse
import os
import platform
import re
import shutil
import subprocess
import sys
import unittest
import threading

from unittest_server import UnittestServer
from urlparse import urlparse

# Import HDK modules
sys.path.append(os.path.join(os.path.dirname(sys.argv[0]), "..", "..", "bin", "lib"))
from hdk.testutil import BuildSuite, ActualDiffers


#
# Main
#
def main():

    # Command line options
    cmdParser = optparse.OptionParser()
    cmdParser.add_option("-r", action = "store_true", dest = "bNoClean",
                         help = "No clean build")
    cmdParser.add_option("-m", action = "store_true", dest = "bCheckMallocStats",
                         help = "Check malloc statistics")
    cmdParser.add_option("-t", action = "append", dest = "testsIncluded",
                         help = "Run specified tests", metavar = "test")
    cmdParser.add_option("-u", action = "store_true", dest = "bUpdateExpected",
                         help = "Update expected output")
    cmdParser.add_option("--debug", action = "store_true", dest = "bDebug",
                         help = "Build debug binaries")
    (cmdOptions, cmdArgs) = cmdParser.parse_args()

    # The unittest directory
    unittestDir = os.path.dirname(sys.argv[0])
    if not unittestDir:
        unittestDir = '.'

    # Create the test runner
    runner = unittest.TextTestRunner(verbosity = 2)

    # Build test suite
    buildDirs = (os.path.join("build", "libhdkcli"),
                 os.path.join("build", "libhdkcli-c++"),
                 os.path.join("build", "libhdkcli-logging"),
                 os.path.join("build", "libhdkcli-c++-logging"))

    bWindowsPlatform = (platform.system() == 'Windows')
    bDarwinPlatform = (platform.system() == 'Darwin')

    # Build target (platform dependant)
    if bWindowsPlatform:
        buildTarget = "libhdkcli.dll"
    elif bDarwinPlatform:
        buildTarget = "libhdkcli.dylib"
    else:
        buildTarget = "libhdkcli.so"

    buildSuite = BuildSuite(unittestDir, buildDirs, buildTarget,
                            not cmdOptions.bNoClean, cmdOptions.bDebug, cmdOptions.bUpdateExpected)
    if not runner.run(buildSuite).wasSuccessful():
        return 1

    # Location of the test definitions and parameters.
    dirTests = os.path.join(unittestDir, "tests")

    # Run the unittests for each target.
    unittestSuite = ClientUnittestSuite()
    for buildDir in ((os.path.join(unittestDir, "build", "unittest-logging")),
                     (os.path.join(unittestDir, "build", "unittest-c++"))):

        unittestSuite.addTest(dirTests, buildDir, "unittest",
                              cmdOptions.bCheckMallocStats,
                              cmdOptions.bUpdateExpected,
                              testsExplicit = cmdOptions.testsIncluded)

        # Only update expected results with gold (first build dir)
        if cmdOptions.bUpdateExpected:
            break

    if not runner.run(unittestSuite).wasSuccessful():
        return 1

    # Success
    return 0


######################################################################
#
# HDK client Unit Test Suite
#
######################################################################

class ClientUnittestSuite(unittest.TestSuite):

    def __init__(self):

        unittest.TestSuite.__init__(self)

    def addTest(self, dirTests, buildDir, target, bCheckMallocStats, bUpdateExpected, testsExplicit = None):

        testNames = sorted(os.listdir(dirTests))
        for testName in testNames:
            testDir = os.path.join(dirTests, testName)
            if os.path.isdir(testDir) and testName[0] != '.':
                if testsExplicit is None or testName in testsExplicit:
                    test = ClientUnittestTest(testName, testDir, buildDir, target,
                                              bCheckMallocStats,
                                              bUpdateExpected)
                    unittest.TestSuite.addTest(self, test)

class ClientUnittestTest(unittest.TestCase):

    __defaultHttpHost = "http://localhost:8080"

    def __init__(self, testName, testDir, buildDir, target, bCheckMallocStats, bUpdateExpected):

        unittest.TestCase.__init__(self)

        self.__testDir = testDir
        self.__buildDir = buildDir
        self.__target = target
        self.__test = testName
        self.__bCheckMallocStats = bCheckMallocStats
        self.__bUpdateExpected = bUpdateExpected

        self.__methodName = None # the SOAPAction for the method

        self.__networkObjectID = None
        self.__httpHost = ClientUnittestTest.__defaultHttpHost
        self.__httpUsername = None
        self.__httpPassword = None
        self.__httpTimeout = None
        self.__platformExpected = False

        # Input files are assumed to be named <Testname>.input.xml
        self.__hnapInputFile = None

        hnapInputFile = os.path.join(self.__testDir, (testName + ".input.xml"))
        if os.path.exists(hnapInputFile):
            self.__hnapInputFile = hnapInputFile

        # Output files are assumed to be named <Testname>.output
        self.__hnapOutputFile = None

        hnapOutputFile = os.path.join(self.__testDir, (testName + ".output"))
        if os.path.exists(hnapOutputFile):
            self.__hnapOutputFile = hnapOutputFile

        # Load the test parameters.
        fileTestSpec = os.path.join(self.__testDir, (testName + ".test"))
        if not os.path.exists(fileTestSpec):
            self.fail("Test parameters do not exist for test '%s'.  File '%s' does not exist." % (testName, fileTestSpec))

        self.parseTestSpec(fileTestSpec)

        if self.__methodName is None:
            self.fail("No method specified for test '%s' in test file '%s'" % (testName, fileTestSpec))

    def __str__(self):

        return os.path.join(self.__buildDir, self.__target) + " " + self.__test

    def parseTestSpec(self, testSpecFilePath):

        # Parse test cases from spec file
        fhTestSpec = open(testSpecFilePath, "rb")

        reMethodNone = re.compile('^method:\s*(#.*)?$')
        reMethod = re.compile('^method:\s*"?\s*(?P<method>.+?)\s*"?\s*(#.*)?$')
        reHttpHost = re.compile('^host:\s*"?\s*(?P<host>.+?)\s*"?\s*(#.*)?$')
        reNetworkObjectID = re.compile('^network-object-id:\s*"?\s*(?P<id>.+?)\s*"?\s*(#.*)?$')
        reHttpUsername = re.compile('^username:\s*"?\s*(?P<username>.+?)\s*"?\s*(#.*)?$')
        reHttpPassword = re.compile('^password:\s*"?\s*(?P<password>.+?)\s*"?\s*(#.*)?$')
        reHttpTimeout = re.compile('^timeout:\s*"?\s*(?P<timeout>.+?)\s*"?\s*(#.*)?$')
        reHttpPlatformExpected = re.compile('^platform-expected\s*(#.*)?$')
        reComment = re.compile("^\s*(#.*)?$")

        try:
            for line in fhTestSpec:
                # Don't care about whitespace or comment lines
                if reComment.search(line):
                    continue

                # Handle a method
                m = reMethodNone.match(line)
                if m:
                    self.__methodName = ""
                    continue

                m = reMethod.match(line)
                if m:
                    self.__methodName = m.group("method")
                    continue

                m = reNetworkObjectID.match(line)
                if m:
                    self.__networkObjectID = m.group("id")
                    continue

                m = reHttpHost.match(line)
                if m:
                    self.__httpHost = m.group("host")
                    continue

                m = reHttpUsername.match(line)
                if m:
                    self.__httpUsername = m.group("username")
                    continue

                m = reHttpPassword.match(line)
                if m:
                    self.__httpPassword = m.group("password")
                    continue

                m = reHttpTimeout.match(line)
                if m:
                    self.__httpTimeout = m.group("timeout")
                    continue

                m = reHttpPlatformExpected.match(line)
                if m:
                    self.__platformExpected = True
                    continue

                # Raise an exception if we don't match the line
                else:
                    raise Exception("Invalid line '%s' in test file '%s'" % (line, testSpecFilePath))
        finally:
            fhTestSpec.close()

    def runTest(self):

        # Actual and expected file locations
        actualDir = os.path.join(self.__testDir, os.path.join("actual", os.path.split(self.__buildDir)[1]))
        expectedDir = os.path.join(self.__testDir, "expected")
        actual = os.path.join(actualDir, self.__test + ".txt")

        expectedSuffix = ""

        if self.__platformExpected:
            if platform.system() == 'Windows':
                expectedSuffix = "-Windows"

        expected = os.path.join(expectedDir, self.__test + expectedSuffix + ".txt")

        # Generate the command line arguments for the unittest harness binary.
        cmdLineArgs = "\"" + self.__methodName + "\" \"" + self.__httpHost + "\""

        if self.__networkObjectID is not None:
            cmdLineArgs = cmdLineArgs + " -o" + self.__networkObjectID

        if self.__httpUsername is not None:
            cmdLineArgs = cmdLineArgs + " -u" + self.__httpUsername

        if self.__httpPassword is not None:
            cmdLineArgs = cmdLineArgs + " -p" + self.__httpPassword

        if self.__httpTimeout is not None:
            cmdLineArgs = cmdLineArgs + " -t" + self.__httpTimeout

        # Write logging output to <Testname>.log
        cmdLineArgs = cmdLineArgs + " -l\"" + os.path.join(actualDir, self.__test + ".log\"")

        if self.__hnapInputFile is not None:
            cmdLineArgs = cmdLineArgs + " " + self.__hnapInputFile

        # Write the actual file
        if not os.path.isdir(actualDir):
            os.makedirs(actualDir)
        if os.path.isfile(actual):
            os.remove(actual)
        fhActual = open(actual, "wb")

        try:
            server = None

            # Parse the host and port from the http host URL
            url = urlparse(self.__httpHost)
            host = url.netloc
            port = url.port
            if port is None:
                if url.scheme == "https":
                    port = 443
                else:
                    port = 80
            else:
                # Must remove the port from the host
                host = re.search('(?P<host>.+?):\d', host).group("host")

            if host:
                server = UnittestServer(self.__testDir, host = host, port = port, fhLogFile = fhActual)

                # Use serve_forever so we can easily shutdown the server in the event of some exception trying to run the client code.
                serverThread = threading.Thread(target = server.serve_forever)
                serverThread.start()

            try:

                if platform.system() == 'Windows':
                    env = { 'SystemRoot': os.environ['SystemRoot'] } # SystemRoot is required for SxS c runtime
                elif platform.system() == 'Darwin':
                    env = { 'DYLD_LIBRARY_PATH': self.__buildDir,
                            'DYLD_INSERT_LIBRARIES': os.path.join(self.__buildDir, 'malloc_interposer.dylib') }
                else:
                    env = { 'LD_LIBRARY_PATH': self.__buildDir,
                            'LD_PRELOAD': os.path.join(self.__buildDir, "malloc_interposer.so") }

                # Run the test
                unittestProcess = subprocess.Popen(os.path.join(self.__buildDir, self.__target) + " " + cmdLineArgs, shell = True,
                                                   env = env,
                                                   stdout = subprocess.PIPE, stderr = subprocess.STDOUT)
                output = unittestProcess.communicate()[0]
                result = unittestProcess.wait()
                if result != 0:
                    self.fail(output)
                fhActual.write("\n******* Client Result ******\n")
                fhActual.write(output)

            finally:
                if server is not None:
                    # Wait until the server thread exits.
                    server.shutdown()
                    serverThread.join()

        finally:
            fhActual.close()

        fnDiffIgnore = []

        # Compare actual and expected files
        actualResult = ActualDiffers(actual, expected, self.__bUpdateExpected,
                                     bIgnoreMallocStats = not self.__bCheckMallocStats,
                                     fnDiffIgnore = fnDiffIgnore)
        if actualResult:
            self.fail(actualResult)


######################################################################

if __name__ == "__main__":
    sys.exit(main())
