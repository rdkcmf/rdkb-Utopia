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

import os
import platform
import re
import shutil
import subprocess
import unittest


#
# Helper function to update expected files
#
def ActualDiffers(actual, expected, bUpdateExpected,
                  bIgnoreMallocStats = False,
                  fnDiffIgnore = None, fnDiffError = None):

    if fnDiffIgnore is None:
        fnDiffIgnore = []

    # Ignore malloc stats in expected/actual, if requested
    if bIgnoreMallocStats:
        reIgnoreMallocStats = re.compile("^malloc_interposer\.c - [exit:|\*+]")
        fnDiffIgnore.append(lambda s: reIgnoreMallocStats.search(s) is not None)

    if fnDiffError is None:
        fnDiffError = []

    # Report errors for memory leaks
    reMemoryLeaks = re.compile("^malloc_interposer\.c - Memory leak:")
    fnDiffError.append(lambda s: reMemoryLeaks.search(s) is not None)

    # Compare actual and expected files
    expectedDir = os.path.dirname(expected)
    if not os.path.isfile(expected):
        if bUpdateExpected:
            if not os.path.isdir(expectedDir):
                os.makedirs(expectedDir)
            shutil.copy(actual, expected)
        else:
            return "%s does not exist" % (expected)
    else:
        if FilesDiffer(expected, actual, fnDiffIgnore, fnDiffError):
            if bUpdateExpected:
                if not os.path.isdir(expectedDir):
                    os.makedirs(expectedDir)
                shutil.copy(actual, expected)
            else:
                return 'Actual differs from expected: "%s" "%s"' % \
                    (os.path.abspath(actual), os.path.abspath(expected))

    return False


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
        errors = []
        for fn in fnErrorActual:
            errors.extend([ x for x in actualLines if fn(x) ])
        if errors:
            raise Exception("".join(errors))

    # Compare
    if fnIgnore is not None:
        for fn in fnIgnore:
            expectedLines = [ x for x in expectedLines if not fn(x) ]
            actualLines = [ x for x in actualLines if not fn(x) ]
    return expectedLines != actualLines


######################################################################
#
# Build Test Suite
#
######################################################################

class BuildSuite(unittest.TestSuite):

    def __init__(self, unittestDir, buildDirs, target, bClean, bDebug, bUpdateExpected):

        unittest.TestSuite.__init__(self)

        # Add the build test
        if bClean:
            self.addTest(BuildCleanTest(unittestDir))
        self.addTest(BuildTest(unittestDir, bDebug))

        # Add the export check tests
        for buildDir in buildDirs:

            self.addTest(CheckExportsTest(unittestDir, buildDir, target, bUpdateExpected))

            # Only update expected results with gold (first build dir)
            if bUpdateExpected:
                break


#
# Clean build test
#
class BuildCleanTest(unittest.TestCase):

    def __init__(self, unittestDir):

        unittest.TestCase.__init__(self)

        self.__unittestDir = unittestDir

    def __str__(self):

        return "Clean libraries and unittest clients"

    def runTest(self):

        # Make command
        if platform.system() == 'Windows':
            make = 'nmake /f Makefile.nmake clean'
        else:
            make = 'make clean'

        # Run make
        proc = subprocess.Popen(make, shell = True, cwd = self.__unittestDir,
                                stdout = subprocess.PIPE, stderr = subprocess.STDOUT)
        makeOutput = proc.communicate()[0]
        if proc.wait() != 0:
            self.fail(makeOutput)


#
# Build test (library and unittest client)
#
class BuildTest(unittest.TestCase):

    def __init__(self, unittestDir, bDebug):

        unittest.TestCase.__init__(self)

        self.__unittestDir = unittestDir
        self.__bDebug = bDebug

    def __str__(self):

        return "Build libraries and unittest clients"

    def runTest(self):

        # Make command
        if platform.system() == 'Windows':
            make = 'nmake /f Makefile.nmake'
        else:
            make = 'make'
        if self.__bDebug:
            make += " DEBUG=1"

        # Run make
        proc = subprocess.Popen(make, shell = True, cwd = self.__unittestDir,
                                stdout = subprocess.PIPE, stderr = subprocess.STDOUT)
        makeOutput = proc.communicate()[0]
        if proc.wait() != 0:
            self.fail(makeOutput)


#
# Library exports test
#
class CheckExportsTest(unittest.TestCase):

    def __init__(self, unittestDir, buildDir, target, bUpdateExpected):

        unittest.TestCase.__init__(self)

        self.__unittestDir = unittestDir
        self.__buildDir = buildDir
        self.__target = target
        self.__bUpdateExpected = bUpdateExpected

    def __str__(self):

        return "Check exports for '%s'" % (os.path.join(self.__buildDir, self.__target))

    def runTest(self):

        if platform.system() == 'Windows':
            symbols = self.runLink()
            fileNameSuffix = "-Windows"
        elif platform.system() == 'Darwin':
            symbols = self.runNm()
            fileNameSuffix = "-Darwin"
        else:
            symbols = self.runReadElf()
            fileNameSuffix = ""

        # Actual and expected file locations
        actualDir = os.path.join(self.__unittestDir, os.path.join("actual", os.path.split(self.__buildDir)[1]))
        expectedDir = os.path.join(self.__unittestDir, "expected")
        actual = os.path.join(actualDir, "CheckExports%s.txt" % fileNameSuffix)
        expected = os.path.join(expectedDir, "CheckExports%s.txt" % fileNameSuffix)

        # Write the actual file
        if not os.path.isdir(actualDir):
            os.makedirs(actualDir)
        if os.path.isfile(actual):
            os.remove(actual)
        fh = open(actual, "w")
        for symbol in sorted(symbols.iterkeys()):
            print >>fh, symbol
        fh.close()

        # Compare the actual/expected files
        result = ActualDiffers(actual, expected, self.__bUpdateExpected)
        if result:
            self.fail(result)

    def runReadElf(self):

        # Read the global symbols using readelf
        proc = subprocess.Popen("readelf -s -W %s" % (os.path.join(self.__unittestDir, self.__buildDir, self.__target)),
                                shell = True, stdout = subprocess.PIPE, stderr = subprocess.STDOUT)
        readelfOutput = proc.communicate()[0]
        if proc.wait() != 0:
            self.fail(readelfOutput)

        # Get exported symbols
        symbols = {}
        reElf = re.compile("\s*\d+:\s+\w+\s+\d+\s+(?P<type>(OBJECT)|(FUNC))\s+GLOBAL\s+\w+\s+\d+\s+(?P<name>\w+)")
        for line in readelfOutput.splitlines():
            m = reElf.search(line)
            if m:
                symbols["%s (%s)" % (m.group("name"), m.group("type"))] = None

        return symbols

    def runNm(self):

        # Read the global symbols using nm
        proc = subprocess.Popen("nm -g %s" % (os.path.join(self.__unittestDir, self.__buildDir, self.__target)),
                                shell = True, stdout = subprocess.PIPE, stderr = subprocess.STDOUT)
        readelfOutput = proc.communicate()[0]
        if proc.wait() != 0:
            self.fail(readelfOutput)

        # Get exported symbols
        symbols = {}
        reElf = re.compile("[0-9A-Fa-f]+\s+T\s+(?P<name>\w+)")
        for line in readelfOutput.splitlines():
            m = reElf.search(line)
            if m:
                symbols["%s" % (m.group("name"))] = None

        return symbols

    def runLink(self):

        # Read the global symbols using link
        proc = subprocess.Popen("link -dump -exports %s" % (os.path.join(self.__unittestDir, self.__buildDir, self.__target)),
                                shell = True, stdout = subprocess.PIPE, stderr = subprocess.STDOUT)
        linkOutput = proc.communicate()[0]
        if proc.wait() != 0:
            self.fail(linkOutput)

        # Get exported symbols
        symbols = {}
        reLink = re.compile("\s*\d+\s+[0-9A-Fa-f]+\s+[0-9A-Fa-f]{8}\s+(?P<name>\w+)")
        for line in linkOutput.splitlines():
            m = reLink.search(line)
            if m:
                symbols["%s" % (m.group("name"))] = None

        return symbols

######################################################################
#
# C Unit Test Suite
#
######################################################################

class UnittestSuite(unittest.TestSuite):

    def __init__(self):

        unittest.TestSuite.__init__(self)

    def addTest(self, unittestDir, buildDir, target, bCheckMallocStats, bUpdateExpected):

        # Get list of tests
        if platform.system() == 'Windows':
            env = { 'SystemRoot': os.environ['SystemRoot'] } # SystemRoot is required for SxS c runtime
        elif platform.system() == 'Darwin':
            env = { 'DYLD_LIBRARY_PATH': '.' }
        else:
            env = { 'LD_LIBRARY_PATH': '.' }

        proc = subprocess.Popen(os.path.join(".", target), shell = True,
                                cwd = os.path.join(unittestDir, buildDir),
                                env = env,
                                stdout = subprocess.PIPE, stderr = subprocess.STDOUT)
        testNames = proc.communicate()[0].splitlines()

        # Create a test object for each test
        for testName in testNames:

            test = UnittestTest(unittestDir, buildDir, target, testName,
                                bCheckMallocStats,
                                bUpdateExpected)
            unittest.TestSuite.addTest(self, test)



class UnittestTest(unittest.TestCase):

    def __init__(self, unittestDir, buildDir, target, test, bCheckMallocStats, bUpdateExpected):

        unittest.TestCase.__init__(self)

        self.__unittestDir = unittestDir
        self.__buildDir = buildDir
        self.__target = target
        self.__test = test
        self.__bCheckMallocStats = bCheckMallocStats
        self.__bUpdateExpected = bUpdateExpected

    def __str__(self):

        return os.path.join(self.__buildDir, self.__target) + " " + self.__test

    def runTest(self):

        # Actual and expected file locations
        actualDir = os.path.join(self.__unittestDir, os.path.join("actual", os.path.split(self.__buildDir)[1]))
        expectedDir = os.path.join(self.__unittestDir, "expected")
        actual = os.path.join(actualDir, self.__test + ".txt")
        expected = os.path.join(expectedDir, self.__test + ".txt")

        if not os.path.isdir(actualDir):
            os.makedirs(actualDir)

        if platform.system() == 'Windows':
            env = { 'SystemRoot': os.environ['SystemRoot'] } # SystemRoot is required for SxS c runtime
        elif platform.system() == 'Darwin':
            env = { 'DYLD_LIBRARY_PATH': '.',
                    'DYLD_INSERT_LIBRARIES': os.path.join('.', 'malloc_interposer.dylib') }
        else:
            env = { 'LD_LIBRARY_PATH': '.',
                    'LD_PRELOAD': os.path.join('.', 'malloc_interposer.so') }

        # Write the log output to: <actualDir>/<test>.log
        logFile = os.path.abspath(os.path.join(actualDir, self.__test + ".log"))

        # Run the test
        proc = subprocess.Popen(os.path.join(".", self.__target) + " " + self.__test + " -l" + logFile, shell = True,
                                cwd = os.path.join(self.__unittestDir, self.__buildDir),
                                env = env,
                                stdout = subprocess.PIPE, stderr = subprocess.STDOUT)
        output = proc.communicate()[0]
        result = proc.wait()
        if result != 0:
            self.fail(output)

        # Write the actual file
        if os.path.isfile(actual):
            os.remove(actual)
        fh = open(actual, "w")
        fh.write(output)
        fh.close()

        # Compare actual and expected files
        actualResult = ActualDiffers(actual, expected, self.__bUpdateExpected,
                                     not self.__bCheckMallocStats)
        if actualResult:
            self.fail(actualResult)
