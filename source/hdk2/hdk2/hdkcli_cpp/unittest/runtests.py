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
import sys
import unittest
import platform

# Import HDK modules
sys.path.append(os.path.join(os.path.dirname(sys.argv[0]), "..", "..", "bin", "lib"))
from hdk.testutil import BuildSuite, UnittestSuite


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
    buildDirs = []
    buildSuite = BuildSuite(unittestDir, buildDirs, None,
                            not cmdOptions.bNoClean, cmdOptions.bDebug, cmdOptions.bUpdateExpected)
    if not runner.run(buildSuite).wasSuccessful():
        return 1

    # C unit test suite
    unittestSuite = UnittestSuite()
    unittestSuite.addTest(unittestDir, os.path.join("build", "unittest"), "unittest",
                          cmdOptions.bCheckMallocStats, cmdOptions.bUpdateExpected)

    if not runner.run(unittestSuite).wasSuccessful():
        return 1

    # Success
    return 0


######################################################################

if __name__ == "__main__":
    sys.exit(main())
