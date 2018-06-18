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

#
# unittest_server.py - Scriptable server for HDK client unittesting
#

import collections
import sys
import os
import optparse
import signal
import SocketServer
import BaseHTTPServer
import threading
import platform
import re
import socket
import time
import shutil
import select
try:
    from cStringIO import StringIO
except ImportError:
    from StringIO import StringIO

__all__ = ["UnittestServer", "__main__"]

__version__ = "0.1"

##
#
# Script syntax
#
#
# Syntax:
#   '!' - Indicates the rest of the line should be sent to the client (as is)
#   '#' - Indicates a comment and is ignored by the server
#   'sleep:<TIME>' - sleep for TIME milliseconds before reading the next line of script
#   'close' - close the connection immediately
#
##

class ScriptActionBase:

    def do_action(self, fhOutputStream, server, request):

        raise Exception("Invalid action")

    def merge_action(self, action):

        raise Exception("Invalid action")

class ScriptActionReadHeaders(ScriptActionBase):

    # Override ScriptActionBase.do_action()
    def do_action(self, request_handler, fhLogStream):

       # Copy it to the output stream.
        fhLogStream.write("+++++++ Request Headers +++++++\n")

        # HTTP version and command.
        fhLogStream.write("%s %s %s\n" % (request_handler.request_version, request_handler.command, request_handler.path))

        # Write the relevant headers to the output file.
        for header in \
            [
                'Connection',
                'Content-Type',
                'Content-Length',
                'Authorization',
                'SOAPAction',
                'Host',
                'Expect',
                'X-DeviceID',
                'X-NetworkObjectID'
            ]:
            headerValue = request_handler.headers.getheader(header)
            if headerValue:
                fhLogStream.write(header + ": " + headerValue + "\n")
        fhLogStream.write("\n")

        return True

    # Override ScriptActionBase.merge_action()
    def merge_action(self, action):

        if isinstance(action, ScriptActionLogRequestHeaders):
            self._fh.write(action._fh.getvalue())
        else:
            raise Exception("Attempting to merge non-ScriptActionLogRequestHeaders: %s" % str(action))

class ScriptActionReadData(ScriptActionBase):

    def __init__(self, bytes = None):

        self._bytes = bytes

    # Override ScriptActionBase.do_action()
    def do_action(self, request_handler, fhLogStream):

        contentLength = request_handler.headers.getheader('Content-Length')
        if self._bytes is None and contentLength is not None:
            self._bytes = int(contentLength)

        if self._bytes is not None:
            # Write it to the log stream.
            fhLogStream.write("+++++++ Request Data +++++++\n")
            fhLogStream.write(request_handler.rfile.read(self._bytes))
            fhLogStream.write("\n")

        return True

    # Override ScriptActionBase.merge_action()
    def merge_action(self, action):

        if isinstance(action, ScriptActionLogRequestData):
            self._bytes = self._bytes + action._bytes
        else:
            raise Exception("Attempting to merge non-ScriptActionLogRequestData: %s" % str(action))

class ScriptActionOutput(ScriptActionBase):
    def __init__(self, string):

        self._fh = StringIO()
        self._fh.write(string)

    # Override ScriptActionBase.do_action()
    def do_action(self, request_handler, fhLogStream):

        fhLogStream.write("\n------- Response -------\n")

        # Write it to the log stream.
        fhLogStream.write(self._fh.getvalue())

        # Copy it to the output stream.
        request_handler.wfile.write(self._fh.getvalue())

        return True

    # Override ScriptActionBase.merge_action()
    def merge_action(self, action):

        if isinstance(action, ScriptActionOutput):
            self._fh.write(action._fh.getvalue())
        else:
            raise Exception("Attempting to merge non-ScriptActionOutput: %s" % str(action))

class ScriptActionSleep:
    def __init__(self, sleepTimeMsecs):

        self._sleepTimeMsecs = sleepTimeMsecs

    # Override ScriptActionBase.do_action()
    def do_action(self, request_handler, fhLogStream):

        fhLogStream.write("$sleep: %s\n" % str(self._sleepTimeMsecs))

        # Sleep for required time or until the server is shutting down.
        request_handler.server._shutdownRequested.wait(self._sleepTimeMsecs / 1000.0)

        return True

    # Override ScriptActionBase.merge_action()
    def merge_action(self, action):

        if isinstance(action, ScriptActionSleep):
            self._sleepTimeMsecs += action._sleepTimeMsecs
        else:
            raise Exception("Attempting to merge non-ScriptActionSleep")

class ScriptActionClose:

    # Override ScriptActionBase.do_action()
    def do_action(self, request_handler, fhLogStream):

        fhLogStream.write("$close\n")

        request_handler.server.close_request(request_handler.request)

        return False

    # Override ScriptActionBase.merge_action()
    def merge_action(self, action):

        if not isinstance(action, ScriptActionClose):
            raise Exception("Attempting to merge non-ScriptActionClose")

class UnittestServerHandler(BaseHTTPServer.BaseHTTPRequestHandler):

    # Initialize the server version string (this is static).
    server_version = "HDK Unittest Server/" + __version__

    def __init__(self, request, client_address, server):
        BaseHTTPServer.BaseHTTPRequestHandler.__init__(self, request, client_address, server)

    # Handle any request.
    def handle_request(self):

        # Load the appropriate script file (removing leading '/')
        script = os.path.join(self.server.root_dir(), self.path[1:])

        # Run the script actions in order.
        for action in UnittestServer.parse_script(script):
            # Perform the action.
            if not action.do_action(self, self.server.log_file()):
                break;

            # Stop script execution is the server is shutting down.
            if self.server._shutdownRequested.isSet():
                break

    # Handle HTTP GET requests.
    def do_GET(self):
        self.handle_request()

    # Handle HTTP POST requests.
    def do_POST(self):
        self.handle_request()

class UnittestServer(BaseHTTPServer.HTTPServer):

    def __init__(self, rootDir, host = "localhost", port = 80, fhLogFile = sys.stdout):
        BaseHTTPServer.HTTPServer.__init__(self, (host, port), UnittestServerHandler)

        self._rootDir = rootDir
        self._fhLogFile = fhLogFile

        self._shutdownRequested = threading.Event()

        self._isShutDown = threading.Event()

    def log_file(self):

        return self._fhLogFile

    def root_dir(self):

        return self._rootDir

    # Override BaseServer.serve_forever()
    def serve_forever(self):

        self._isShutDown.clear()
        self._shutdownRequested.clear()
        while not self._shutdownRequested.isSet():
            r, w, e = select.select([self], [], [], 0)
            if r and not self._shutdownRequested.isSet():
                self.handle_request()
        self._isShutDown.set()

    # This overrides BaseServer.shutdown() in Python 2.6 and defines it in earlier versions
    def shutdown(self):

        self._shutdownRequested.set()

        # Connect to the socket in order to wake up the serve_forever thread
        s = socket.socket(self.socket.family, self.socket.type, self.socket.proto)

        try:
            s.connect(self.server_address)

            self._isShutDown.wait()
        finally:
            s.close()

    # Parse the given script file and return an order list of script action tuples (action, data)
    @staticmethod
    def parse_script(scriptFilePath):

        actionQueue = collections.deque()

        reComment = re.compile("^\$\s*(#.*)?$")
        reSleep = re.compile('^\$sleep:\s*(?P<sleep>\d+?)\s*(#.*)?$')
        reClose = re.compile('^\$close\s*(#.*)?$')
        reReadHeaders = re.compile('^\$read-headers\s*(#.*)?$')
        reReadData = re.compile('^\$read-data:\s*(?P<bytes>\d+?)\s*(#.*)?$')
        reReadContent = re.compile('^\$read-content\s*(#.*)?$')

        # Open in binary mode, so line endings are always '\n' only.
        fhScript = open(scriptFilePath, "rb")
        try:
            for line in fhScript:

                if '$' == line[0]:

                    # Don't care about whitespace or comment lines
                    if reComment.search(line):
                        continue

                    m = reSleep.match(line)
                    if m:
                        UnittestServer.append_action(actionQueue, ScriptActionSleep(float(m.group("sleep"))))
                        continue

                    m = reClose.match(line)
                    if m:
                        UnittestServer.append_action(actionQueue, ScriptActionClose())
                        continue

                    m = reReadHeaders.match(line)
                    if m:
                        UnittestServer.append_action(actionQueue, ScriptActionReadHeaders())
                        continue

                    m = reReadData.match(line)
                    if m:
                        UnittestServer.append_action(actionQueue, ScriptActionReadData(int(m.group("bytes"))))
                        continue

                    m = reReadContent.match(line)
                    if m:
                        UnittestServer.append_action(actionQueue, ScriptActionReadData())
                        continue

                    raise Exception("Invalid line '%s' in script file" % line)

                else:
                    # Any line that does not begin with a '$' is consider output

                    # Perforce changes '\n' line endings into '\r\n' for Windows clients.
                    # These must be changed back for WinHTTP to parse the headers on Windows 7.
                    if platform.system() == 'Windows':
                        line = line.replace('\r\n', '\n')
                    UnittestServer.append_action(actionQueue, ScriptActionOutput(line))

        finally:
            fhScript.close()

        return actionQueue

    @staticmethod
    def append_action(actionQueue, newAction):

        prevAction = None
        if actionQueue:
            prevAction = actionQueue[-1]
        # Add/merge the action into our queue of actions.
        if prevAction is not None and isinstance(newAction, prevAction.__class__):
            prevAction.merge_action(newAction)
        else:
            actionQueue.append(newAction)
#
# Main
#
def main():

    cmdParser = optparse.OptionParser("usage: %prog [options] root")
    cmdParser.add_option("--host", action = "store", type = "string", dest = "host", default = "localhost",
                         help = "server host", metavar = "host")
    cmdParser.add_option("--port", action = "store", type = "int", dest = "port", default = "8080",
                         help = "server port", metavar = "port")

    # Parse command line options
    (cmdOptions, cmdArgs) = cmdParser.parse_args()
    if cmdArgs is None or 1 != len(cmdArgs):
        cmdParser.error("Must specify root directory from which to serve files")

    rootDir = cmdArgs[0]

    if not os.path.exists(rootDir):
        cmdParser.error("Root directory '%s' does not exist" % rootDir)

    try:
        UnittestServer(rootDir, host = cmdOptions.host, port = cmdOptions.port).serve_forever()
    except KeyboardInterrupt:
        # Do nothing, this is expected.
        None


######################################################################

if __name__ == "__main__":
    main()
