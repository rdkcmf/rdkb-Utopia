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

#
# hnappost.py - Send an HNAP POST request
#

import base64
import httplib
import optparse
import re
import StringIO
import sys
import time
import traceback
from xml.dom import minidom


#
# Main
#
def main():

    # Command line options
    cmdParser = optparse.OptionParser("usage: %prog [options] host")
    cmdParser.add_option("-u", action = "store", dest = "user", default = "admin",
                         help = "Host user name", metavar = "USER")
    cmdParser.add_option("-p", action = "store", dest = "password", default = "admin",
                         help = "Host password", metavar = "PASSWORD")
    cmdParser.add_option("-s", action = "store_true", dest = "bSSL", default = False,
                         help = "Use SSL")
    cmdParser.add_option("-m", action = "store", dest = "method",
                         help = "Send simple HNAP request", metavar = "METHOD")
    cmdParser.add_option("-f", action = "store", dest = "file",
                         help = "Send HNAP request from file", metavar = "FILE")
    cmdParser.add_option("-o", action = "store_true", dest = "bOutput",
                         help = "Output request content")
    cmdParser.add_option("--did", action = "store", dest = "deviceID",
                         help = "Specify Device ID", metavar="UUID")
    cmdParser.add_option("--noid", action = "store", dest = "networkObjectID",
                         help = "Specify Network Object ID", metavar="UUID")
    cmdParser.add_option("--settings", action = "store_true", dest = "bGetDeviceSettings", default = False,
                         help = "Send GetDeviceSettings request")
    cmdParser.add_option("--reboot", action = "store_true", dest = "bReboot", default = False,
                         help = "Send Reboot request")
    cmdParser.add_option("--firmware", action = "store", dest = "fileFirmware",
                         help = "Send FirmwareUpload request", metavar = "FILE")
    cmdParser.add_option("--backup", action = "store", dest = "fileConfigBackup",
                         help = "Send GetConfigBlob request", metavar = "FILE")
    cmdParser.add_option("--restore", action = "store", dest = "fileConfigRestore",
                         help = "Send SetConfigBlob request", metavar = "FILE")
    cmdParser.add_option("--factory", action = "store_true", dest = "bRestoreFactoryDefaults", default = False,
                         help = "Send RestoreFactoryDefaults request")
    cmdParser.add_option("--split-count", action = "store", type = "int", dest = "split_count", default = "1",
                         help = "Split request in multiple chunks", metavar = "CHUNKS")
    cmdParser.add_option("--split-period", action = "store", type = "int", dest = "split_period", default = "1",
                         help = "Delay period between each request chunk", metavar = "SECONDS")
    cmdParser.add_option("--split-stop", action = "store", type = "int", dest = "split_stop", default = "0",
                         help = "Stop sending request prior to specified chunk", metavar = "CHUNK")
    (cmdOptions, cmdArgs) = cmdParser.parse_args()

    # Get the host name
    if len(cmdArgs) != 1 and not cmdOptions.bOutput:
        cmdParser.error("No host specified")

    # Get the POST content
    base64Element = None
    if cmdOptions.method is not None:

        (soapAction, content) = CreateSimpleHNAPRequest(cmdOptions.method)

    elif cmdOptions.file is not None:

        try:
            (soapAction, content) = CreateHNAPRequestFromFile(cmdOptions.file)
            if soapAction is None:
                cmdParser.error("No SOAPAction defined.")
        except:
            print >> sys.stderr, "Error: Couldn't open '%s'" % cmdOptions.file
            exit(1)

    elif cmdOptions.bGetDeviceSettings:

        (soapAction, content) = CreateSimpleHNAPRequest("GetDeviceSettings")

    elif cmdOptions.bReboot:

        (soapAction, content) = CreateSimpleHNAPRequest("Reboot")

    elif cmdOptions.fileFirmware:

        try:
            (soapAction, content) = CreateBase64HNAPRequest(cmdOptions.fileFirmware, "FirmwareUpload", "Base64Image")
        except:
            print >> sys.stderr, "Error: Couldn't open '%s'" % cmdOptions.fileFirmware
            exit(1)

    elif cmdOptions.fileConfigBackup:

        (soapAction, content) = CreateSimpleHNAPRequest("GetConfigBlob")
        base64Element = "ConfigBlob"

    elif cmdOptions.fileConfigRestore:

        try:
            (soapAction, content) = CreateBase64HNAPRequest(cmdOptions.fileConfigRestore, "SetConfigBlob", "ConfigBlob")
        except:
            print >> sys.stderr, "Error: Couldn't open '%s'" % cmdOptions.fileConfigRestore
            exit(1)

    elif cmdOptions.bRestoreFactoryDefaults:

        (soapAction, content) = CreateSimpleHNAPRequest("RestoreFactoryDefaults")

    else:

        cmdParser.error("No action specified")

    # Output request content, if requested
    if cmdOptions.bOutput:

        print "Content-Length: %d" % len(content)
        print "SOAPAction: " + soapAction
        print "Authorization: Basic " + base64.b64encode(cmdOptions.user + ":" + cmdOptions.password)
        print
        print content

    else:

        try:
            (respHeader, respBody, tRequest, tResponse, bStopped) = \
                SendHNAPRequest(cmdArgs[0], cmdOptions.deviceID, cmdOptions.networkObjectID, cmdOptions.bSSL, cmdOptions.user, cmdOptions.password,
                                soapAction, content, split_count = cmdOptions.split_count,
                                split_period = cmdOptions.split_period, split_stop = cmdOptions.split_stop)
            if not bStopped:
                print respHeader
                print respBody
                print "Request: %.02f ms" % (tRequest * 1000)
                print "Response: %.02f ms" % (tResponse * 1000)
        except Exception, e:
            print >> sys.stderr, "Error: Exception occurred sending request:", str(e)
            traceback.print_exc(e)
            exit(1)

    # Save the decoded ConfigBlob, if requested
    if base64Element:

        try:
            ParseBase64HNAPResponse(cmdOptions.fileConfigBackup, respBody, base64Element)
        except:
            print >> sys.stderr, "Error: Couldn't create '%s'" % cmdOptions.fileConfigBackup
            exit(1)


######################################################################
#
# Utility functions
#
######################################################################

#
# Create a simple HNAP request given a method name or URI
#
def CreateSimpleHNAPRequest(actionName):

    # Is this a fully-qualified action URI?
    ixNSDelim = actionName.rfind("/")
    if ixNSDelim != -1:
        actionURI = actionName
        actionNS = actionURI[0 : ixNSDelim + 1]
        actionName = actionURI[ixNSDelim + 1 : ]
    else:
        actionNS = "http://purenetworks.com/HNAP1/"
        actionURI = actionNS + actionName

    # Generate the request content
    content = '''\
<?xml version="1.0" encoding="utf-8"?>
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" soap:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
<soap:Body>
<%s xmlns="%s">
</%s>
</soap:Body>
</soap:Envelope>''' % (actionName, actionNS, actionName)

    return (actionURI, content)

#
# Create an HNAP request from a file
#
def CreateHNAPRequestFromFile(file):

    soapAction = None
    content = ''

    # Read the request file - skip HTTP headers
    fh = open(file, "rb")
    try:
        bContent = False
        reHeaderEnd = re.compile('^\r?\n$')
        reHeader = re.compile('^\s*(.+?)\s*:\s*"?(.+?)"?\s*$')
        for line in fh:
            if bContent:
                content += line
            elif reHeaderEnd.search(line):
                bContent = True
            else:
                m = reHeader.search(line)
                if m:
                    header = m.group(1)
                    headerValue = m.group(2)
                    if header == "SOAPAction":
                        soapAction = headerValue
    except:
        raise
    finally:
        fh.close()

    return (soapAction, content)

#
# Create a Base64 HNAP request from a file
#
def CreateBase64HNAPRequest(file, actionName, elementName):

    # Add the request header
    content = StringIO.StringIO()
    content.write('''\
<?xml version="1.0" encoding="utf-8"?>
<soap:Envelope xmlns:soap="http://schemas.xmlsoap.org/soap/envelope/" soap:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
<soap:Body>
<%s xmlns="http://purenetworks.com/HNAP1/">
<%s>
''' % (actionName, elementName))

    # Base64 encode the file blob
    fh = open(file, "rb")
    try:
        blob = fh.read()
        content.write(base64.standard_b64encode(blob))
    except:
        raise
    finally:
        fh.close()

    # Add the footer
    content.write('''\
</%s>
</%s>
</soap:Body>
</soap:Envelope>''' % (elementName, actionName))

    # Return the request
    soapAction = "http://purenetworks.com/HNAP1/%s" % actionName
    return (soapAction, content.getvalue())

#
# Parse a Base64 HNAP response and decode to a file
#
def ParseBase64HNAPResponse(file, respBody, elementName):

    # Find the ConfigBlob node
    xmldoc = minidom.parseString(respBody)
    blob = xmldoc.getElementsByTagName(elementName)[0].firstChild.data

    # Decode and save config blob
    fh = open(file, "wb")
    try:
        fh.write(base64.b64decode(blob))
    except:
        raise
    finally:
        fh.close()

#
# Send an HNAP request
#
def SendHNAPRequest(host, deviceID, networkObjectID, bSSL, username, password, soapAction, content,
                    split_count = 1, split_period = 0, split_stop = 0):

    # Create the HTTP connection object
    if bSSL:
        conn = httplib.HTTPSConnection(host, strict = 0)
    else:
        conn = httplib.HTTPConnection(host, strict = 0)

    # Send the request
    try:
        headers = {
            "SOAPAction": soapAction,
            "Authorization": "Basic " + base64.b64encode(username + ":" + password),
            "Content-type": 'text/xml; charset="UTF-8"',
            "Connection": "close"
            }

        # Add network object reference headers
        if deviceID:
            headers["X-DeviceID"] = deviceID
        if networkObjectID:
            headers["X-NetworkObjectID"] = networkObjectID

        tStart = time.time()
        bStopped = False
        if split_count < 2:
            conn.request("POST", "/HNAP1/", content, headers)
        else:
            conn.putrequest("POST", "/HNAP1/")
            for header in headers.iterkeys():
                conn.putheader(header, headers[header])
            conn.putheader("Content-Length", str(len(content)))
            conn.endheaders()
            for i in xrange(0, split_count):
                if split_stop == i + 1:
                    print "Stopping before chunk #%d" % (i + 1)
                    bStopped = True
                    break
                i1 = i * len(content) / split_count
                i2 = (i + 1) * len(content) / split_count
                print "Sending chunk #%d (%d, %d)..." % (i + 1, i1, i2)
                conn.send(content[i1:i2])
                if split_period > 0 and i < split_count - 1:
                    time.sleep(split_period)
        tRequest = time.time() - tStart

        # Read the response
        tStart = time.time()
        if not bStopped:
            resp = conn.getresponse()
        tResponse = time.time() - tStart

        # Return the response with timing
        if not bStopped:
            respHeader = "HTTP/1." + str(resp.version - 10) + ' ' + str(resp.status) + \
                ' ' + resp.reason + '\n' + str(resp.msg)
            respBody = resp.read()
        else:
            respHeader = None
            respBody = None
        return (respHeader, respBody, tRequest, tResponse, bStopped)

    except:
        raise
    finally:
        conn.close()


######################################################################

if __name__ == "__main__":
    main()
