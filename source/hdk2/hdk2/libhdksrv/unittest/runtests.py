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
    buildDirs = (os.path.join("build", "libhdksrv"),
                 os.path.join("build", "libhdksrv-c++"),
                 os.path.join("build", "libhdksrv-nologging"))

    bWindowsPlatform = (platform.system() == 'Windows')
    bDarwinPlatform = (platform.system() == 'Darwin')

    # Build target (platform dependant)
    if bWindowsPlatform:
        buildTarget = "libhdksrv.dll"
    elif bDarwinPlatform:
        buildTarget = "libhdksrv.dylib"
    else:
        buildTarget = "libhdksrv.so"

    buildSuite = BuildSuite(unittestDir, buildDirs, buildTarget,
                            not cmdOptions.bNoClean, cmdOptions.bDebug, cmdOptions.bUpdateExpected)
    if not runner.run(buildSuite).wasSuccessful():
        return 1

    # C unit test suite
    unittestSuite = UnittestSuite()
    for buildDir in ((os.path.join("build", "unittest")),
                     (os.path.join("build", "unittest-c++"))):

        unittestSuite.addTest(unittestDir, buildDir, "unittest",
                              cmdOptions.bCheckMallocStats, cmdOptions.bUpdateExpected)

        # Only update expected results with gold (first build dir)
        if cmdOptions.bUpdateExpected:
            break

    if not runner.run(unittestSuite).wasSuccessful():
        return 1

    # Success
    return 0


######################################################################

if __name__ == "__main__":
    sys.exit(main())
