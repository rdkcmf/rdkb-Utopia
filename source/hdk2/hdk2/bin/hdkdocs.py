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

import cgi
import optparse
import os
import re
import sys
import traceback
import base64

sys.path.append(os.path.join(os.path.dirname(sys.argv[0]), "lib"))
from hdk.hslparser import HSLParser


#
# Main
#
def main():

    # Command line options
    cmdParser = optparse.OptionParser("usage: %prog [options] hsl...")
    cmdParser.add_option("-o", action = "store", type = "string", dest = "outputFile",
                         help = "output file", metavar = "FILE")
    cmdParser.add_option("-I", action = "append", dest = "importDirs", default = [],
                         help = "add directory to import path", metavar = "DIR")

    # Parse command line options
    (cmdOptions, cmdArgs) = cmdParser.parse_args()
    if cmdArgs is None or not cmdArgs:
        cmdParser.error("Must specify at least one HSL file")
    if cmdOptions.outputFile is None:
        cmdParser.error("Must specify output file")

    # Parse HSL files
    parser = HSLParser(cmdOptions.importDirs)
    for file in cmdArgs:
        parser.parseFile(file)
    parser.parseDone()
    if parser.errors:
        for error in parser.errors:
            print >> sys.stderr, error
        sys.exit(1)

    # Generate HTML
    print >> sys.stderr, 'Generating "%s" ...' % cmdOptions.outputFile
    out = open(cmdOptions.outputFile, 'w')
    try:
        write_html(out.write, parser.getmodel())
    except SystemExit:
        raise
    except:
        print >> sys.stderr, "Error: An unexpected exception occurred generating code:"
        traceback.print_exc()
        sys.exit(1)
    finally:
        out.close()


######################################################################

def error(obj, text):
    sys.exit("%s:%d: %s" % (obj.file, obj.line, text))

# Parser for documentation sections.
def docsection_html(section):

    # hack to work around lack of "nonlocal" before Python 3.
    class Holder:
        pass
    out = Holder()
    out.out = ""

    # this is a list of tag types, to track context
    stack = []

    def write(s):
        out.out += s

    def popwritetag():
        tagtype = stack.pop()
        write('</' + tagtype + '>')
        return tagtype

    def pushtag(tagtype):
        stack.append(tagtype)

    def pushwritetag(tagtype):
        write('<' + tagtype + '>')
        pushtag(tagtype)

    def endtag(tagtype):
        if tagtype in stack:
            while not popwritetag() == tagtype:
                pass
        else:
            error(section, 'unmatched end tag "</%s>"' % tagtype)

    # make sure we're inside a <p> element
    def enterspan():
        if not 'p' in stack:
            pushwritetag('p')

    # make sure we're not inside a <p> element
    def exitspan():
        if 'p' in stack:
            while not popwritetag() == 'p':
                pass

    def stacktop():
        if len(stack) == 0:
            return None
        else:
            return stack[-1]

    # sort out element logic
    def beforestarttag(tagtype):
        if tagtype == 'p':
            exitspan()
        elif tagtype == 'ul':
            exitspan()
        elif tagtype == 'ol':
            exitspan()
        elif tagtype == 'table':
            exitspan()
        elif tagtype == 'li':
            exitspan()
            top = stacktop()
            if top == 'li':
                popwritetag()
            elif top == 'ul':
                pass
            elif top == 'ol':
                pass
            else:
                pushwritetag('ul')
        elif tagtype == 'tr':
            exitspan()
            while True:
                top = stacktop()
                if top == None:
                    pushwritetag('table')
                    break
                elif top == 'table':
                    break
                popwritetag()
        elif tagtype == 'td' or tagtype == 'th':
            exitspan()
            while True:
                top = stacktop()
                if top == None:
                    beforestarttag('tr')
                    pushwritetag('tr')
                    break
                elif top == 'table':
                    beforestarttag('tr')
                    pushwritetag('tr')
                    break
                elif top == 'tr':
                    break
                popwritetag()
        else:
            enterspan()

    def text(s):
        if not s.strip() == '':
            enterspan()
            write(s)

    def base64_file(filepath):
        fullpath = os.path.join(os.path.dirname(section.file), os.path.normpath(filepath))
        f = open(fullpath, 'r')
        s = f.read()
        f.close()
        return base64.b64encode(s)

    def mimetype(filename):
        if re.search(r"\.png$", filename):
            return 'image/png'
        elif re.search(r"\.jpeg$", filename):
            return 'image/jpeg'
        elif re.search(r"\.jpg$", filename):
            return 'image/jpeg'
        elif re.search(r"\.gif$", filename):
            return 'image/gif'
        else:
            error(section, 'unknown image type: ' + filename)

    def write_subst_file(tagname, s):
        if tagname == 'img':
            m = re.match("^(.*)@([^'\"]+)(.*)$", s)
            if m:
                write(m.group(1))
                write('data:' + mimetype(m.group(2)) + ';base64,')
                write(base64_file(m.group(2)))
                write(m.group(3))
            else:
                write(s)
        else:
            write(s)

    # run the parser
    for s in section.doctext:
        s = s.strip()
        if s:
            while True:
                m = re.match(r"(?P<pre>[^<]*)(?P<tag><\s*(?P<cslash>/?)(?P<tagname>\w+)[^/>]*(/[^>][^/>]*)*(?P<eslash>/?)>)(?P<rest>.*)$", s)
                if m:
                    text(m.group('pre'))
                    s = m.group('rest')
                    if m.group('cslash') == '/':
                        endtag(m.group('tagname'))
                    else:
                        beforestarttag(m.group('tagname'))
                        if not m.group('eslash') == '/':
                            pushtag(m.group('tagname'))
                        write_subst_file(m.group('tagname'), m.group('tag'))
                else:
                    break
            text(s + ' ')
        else:
            exitspan()
    exitspan()
    while len(stack) > 0:
        popwritetag()
    write('\n')
    return out.out


def type_name(t):
    if t.isArray:
        return type_name(t.arrayType) + "[]"
    elif t.name == "datetime":
        return '<span class="type">DateTime</span>'
    elif t.name == "stream":
        return '<span class="type">string</span>'
    elif t.name == "stream64":
        return '<span class="type">blob</span>'
    elif t.isStruct and not t.isArray:
        return '<a href="#struct.%s"><span class="type">%s</span></a>' % (t.name, t.name)
    elif t.isEnum and not t.isBuiltin:
        return '<a href="#enum.%s"><span class="type">%s</span></a>' % (t.name, t.name)
    else:
        return '<span class="type">%s</span>' % (t.name)


def is_value_type(t):
    return t.isStruct

def member_type_name(m):
    s = type_name(m.type)
    if m.isUnbounded:
        s += '[]'
    elif m.isOptional:
        if is_value_type(m.type):
            s += '?'
        else:
            s += ' <span class="comment">/* may be null */</span>'
    return s

def result_type_name(action):
    for member in action.outputMember.type.members:
        if member.name == action.name + "Result":
            return member_type_name(member)
    return None

def ev_doc(e, ev):
    bResultAuto = e.name.endswith("Result") and not ev.doctext
    if bResultAuto and ev.name == "OK":
        return "<p>succeeded</p>"
    elif bResultAuto and ev.name == "REBOOT":
        return "<p>succeeded; now rebooting</p>"
    elif bResultAuto and ev.name == "ERROR":
        return "<p>failed</p>"
    else:
        return docsection_html(ev)

def anchor(t, name):
    return t + '.' + re.sub('[^A-Za-z0-9_]', '_', name)

# Generate HTML
def write_html(write, model):

    def defn_eol():
        write("<br />\n")

    def defn_indent(indent, line, eol):
        write('&nbsp;&nbsp;&nbsp;&nbsp;' * indent)
        write(line)
        if eol:
            defn_eol()

    def defn_member(n, before, member, after, eol):
        if member.type.isArray:
            if member.type.members[0].name != member.type.arrayType.name:
                defn_indent(n, '[<span class="type">XmlArrayItem</span>(<span class="string">"' + member.type.members[0].name + '"</span>)]', True)
        defn_indent(n, before + member_type_name(member) + ' ' + member.name + after, eol)

    def defn_type(t):
        write('<p class="syntax"><tt>\n')
        if t.isStruct:
            write('<span class="keyword">struct</span> <span class="defined">' + t.name + '</span><br />\n{<br />\n')
            for member in t.members:
                defn_member(1, '', member, ';', True)
            write('}\n')
        elif t.isEnum:
            write('<span class="keyword">enum</span> <span class="defined">' + t.name + '</span><br />\n{<br />\n')
            first = True
            for ev in t.enumValuesEx:
                if first:
                    first = False
                else:
                    write(',<br />\n')
                write('&nbsp;&nbsp;&nbsp;&nbsp;"' + ev.name + '"')
            write('<br />\n')
            write('}\n')
        else:
            write('<span class="keyword">primitive</span> <span class="defined">' + t.name + '</span>\n')
        write('</tt></p>\n')

    def defn_action(action):
        write('<p class="syntax"><tt>\n')
        write(result_type_name(action) + ' <span class="defined">' + action.name + '</span><br />\n(<br />\n')
        first = True
        for member in action.inputMember.type.members:
            if first:
                first = False
            else:
                write(',')
                defn_eol()
            defn_member(1, '', member, '', False)
        for member in action.outputMember.type.members:
            if member.name != action.name + "Result":
                if first:
                    first = False
                else:
                    write(',')
                    defn_eol()
                defn_member(1, '<span class="keyword">out</span> ', member, '', False)
        if not first:
            defn_eol()
        write(');\n')
        write('</tt></p>\n')

    def member_tr(member):
        write('<tr>\n')
        write('<td><p><tt>' + member.name + '</tt></p></td>\n')
        write('<td><p><tt>' + member_type_name(member) + '</tt></p></td>\n')
        write('<td>' + docsection_html(member) + '</td>\n')
        write('</tr>\n')

    def show_type(t, no_uri = False):
        if not no_uri:
            write('<p class="fullname">' + t.uri + '</p>\n')
            write(docsection_html(t))
        defn_type(t)
        if t.isStruct:
            write('<table summary="members" class="params">\n')
            for member in t.members:
                member_tr(member)
            write('</table>\n')
        elif t.isEnum:
            write('<table summary="values" class="params">\n')
            for ev in t.enumValuesEx:
                write('<tr>\n')
                if ev.name == "":
                    write('<td><p><i>empty</i></p></td>\n')
                else:
                    write('<td><p><tt>' + ev.name + '</tt></p></td>\n')
                write('<td>' + ev_doc(t, ev) + '</td>\n')
                write('</tr>\n')
            write('</table>\n')
        else:
            write('<p>This is a primitive type.</p>\n')

    def show_type_h3(t):
        if t.isEnum:
            kind = 'enum'
        else:
            kind = 'struct'
        write('<h3><a name="%s">%s</a></h3>\n' % (anchor(kind, t.name), t.name))
        show_type(t)

    def sample_indent(indent, line):
        write('&nbsp;&nbsp;' * indent)
        write(line + "<br />\n")

    def sample_element(indent, name, t):
        if t.isStruct or t.isArray:
            sample_indent(indent, '&lt;' + name + '&gt;')
            for member in t.members:
                sample_element(indent + 1, member.name, member.type)
            sample_indent(indent, '&lt;/' + name + '&gt;')
        else:
            sample_indent(indent, '&lt;' + name + '&gt;<var>' + type_name(t) + ' value</var>&lt;/' + name + '&gt;')

    def show_action(action):
        write('<h3><a name="action.' + action.name + '">' + action.name + "</a></h3>\n")
        write('<p class="fullname">' + action.uri + '</p>\n')
        write(docsection_html(action))
        if action.isNoAuth:
            write('<p>NOTE: This call does not require authentication.</p>\n')

        # Syntax
        defn_action(action)

        # In Parameters
        if len(action.inputMember.type.members) > 0:
            write('<h4>In Parameters</h4>\n')
            write('<table summary="in parameters" class="params">\n')
            for member in action.inputMember.type.members:
                member_tr(member)
            write('</table>\n')

        # Out Parameters
        if len(action.outputMember.type.members) > 1:
            write('<h4>Out Parameters</h4>\n')
            write('<table summary="out parameters" class="params">\n')
            for member in action.outputMember.type.members:
                if member.name != action.name + "Result":
                    member_tr(member)
            write('</table>\n')

        # Returns
        write('<h4><a name="%s">Return Type</a></h4>\n' % anchor('enum', action.resultEnum.name))
        show_type(action.resultEnum)

        write('<h4>Sample Transaction</h4>\n')

        # Sample Request
        write('<p class="http">\n')
        write('POST <var>location</var> HTTP/1.1<br />\n')
        write('Host: 192.168.1.1<br />\n')
        write('Content-Type: text/xml; charset=utf-8<br />\n')
        write('Content-Length: <var>number of octets in body</var><br />\n')
        write('SOAPAction: "' + action.uri + '"<br />\n')
        write('<br />\n')
        sample_indent(0, '&lt;?xml version="1.0" encoding="utf-8"?&gt;')
        sample_indent(0, '&lt;Envelope xmlns="http://schemas.xmlsoap.org/soap/envelope/"&gt;')
        sample_indent(1, '&lt;Body&gt;')
        if len(action.inputMember.type.members) > 0:
            sample_indent(2, '&lt;' + action.name + ' xmlns="' + action.namespace + '">')
            for member in action.inputMember.type.members:
                sample_element(3, member.name, member.type)
            sample_indent(2, '&lt;/' + action.name + '>')
        else:
            sample_indent(2, '&lt;' + action.name + ' xmlns="' + action.namespace + '" />')
        sample_indent(1, '&lt;/Body&gt;')
        sample_indent(0, '&lt;/Envelope&gt;')
        write('</p>\n')

        # Sample Response
        write('<p class="http"><tt>\n')
        write('HTTP/1.1 200 OK<br />\n')
        write('Content-Type: text/xml; charset=utf-8<br />\n')
        write('Content-Length: <var>number of octets in body</var><br />\n')
        write('<br />\n')
        sample_indent(0, '&lt;?xml version="1.0" encoding="utf-8"?&gt;')
        sample_indent(0, '&lt;Envelope xmlns="http://schemas.xmlsoap.org/soap/envelope/"&gt;')
        sample_indent(1, '&lt;Body&gt;')
        sample_indent(2, '&lt;' + action.name + 'Response xmlns="' + action.namespace + '">')
        for member in action.outputMember.type.members:
            sample_element(3, member.name, member.type)
        sample_indent(2, '&lt;/' + action.name + 'Response>')
        sample_indent(1, '&lt;/Body&gt;')
        sample_indent(0, '&lt;/Envelope&gt;')
        write('</tt></p>\n')

    def show_event(event):

        write('<h3><a name="event.' + event.name + '">' + event.name + "</a></h3>\n")
        write('<p class="fullname">' + event.uri + '</p>\n')
        write(docsection_html(event))

        # Event payload
        if len(event.member.type.members) > 0:

            write('<h4>Event Payload</h4>\n')
            show_type(event.member.type, no_uri = True)

            write('<h4>Sample Event Payload</h4>\n')
            write('<p class="http">\n')
            sample_indent(0, '&lt;' + event.name + ' xmlns="' + event.namespace + '">')
            for member in event.member.type.members:
                sample_element(1, member.name, member.type)
            sample_indent(0, '&lt;/' + event.name + '>')
            write('</p>\n')

        else:

            write('<p>NOTE: This event does not have an event payload.</p>\n')

    def show_service(service):

        write('<h3><a name="service.' + service.name + '">' + service.name + "</a></h3>\n")
        write('<p class="fullname">' + service.uri + '</p>\n')
        write(docsection_html(service))

        if service.actions:
            write('<h4>Service Actions</h4>\n')
            for action in sorted(service.actions.itervalues()):
                write('&nbsp;&nbsp;&nbsp;&nbsp;<a href="#%s">%s</a><br/>\n' % (anchor('action', action.name), action.name))

        if service.events:
            write('<h4>Service Events</h4>\n')
            for event in sorted(service.events.itervalues()):
                write('&nbsp;&nbsp;&nbsp;&nbsp;<a href="#%s">%s</a><br/>\n' % (anchor('event', event.name), event.name))

    def getTitle():
        for section in model.docSections:
            if section.type == 'title':
                return section.title
        return "HNAP"

    # output DOCTYPE for strict XHTML 1.0
    write('<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">\n')

    # output document head
    write('<html>\n<head>\n')
    write('<title>' + getTitle() + '</title>\n')
    write('''
<style type="text/css">
body, td, th {background-color:white;font-family:"Helvetica";font-size:9pt;}
img.logo {padding:40px;}
tt {font-family:"Courier New";}
h1 {font-weight:normal;font-size:16pt;}
h2 {page-break-before:always;border-style: solid none; border-width: 2pt 0; padding: 8pt 0; margin: 30pt 0 10pt 0; font-size:14pt;}
h3 {font-size:12pt;}
h4 {font-size:9pt;margin:3ex 0 1ex 0;}
p {margin:1ex 0;padding:0}
table {margin:0;padding:0}
th,td {padding:0 1em;margin:0;text-align: left;vertical-align: baseline;background-color:#EEE;}
p.syntax {background-color:#EEE;padding:1ex 1em;}
p.http {background-color:#EEE;padding:1ex 1em;font-family:"Courier New";}
hr.before {page-break-before:always;margin-top:7ex;}
div.abstract h4 {border-style: solid none none none;border-width: 2pt 0 0 0;padding: 8pt 0 0 0;margin: 30pt 0 0 0; }
div.copyright p {font-style:italic;font-size:smaller;padding:0;margin:0;}
div.copyright h4 {font-style:italic;font-size:smaller;padding: 100pt 0 0 0;margin:0;}
p.version {font-style:italic;font-weight:bold;}
p.date {font-style:italic;}
p.fullname {font-family:"Courier New";}
var {background-color:#CFC;font-family:"Helvetica";font-size:8pt;font-style:normal;}
var:before {content:"["}
var:after {content:"]"}
.keyword {color: blue;}
.string {color: darkblue;}
.comment {color: darkgrey;}
.type {color: #800;}
.defined {font-weight:bold;}
table.params th {background-color:#EEE;}
table.params td {}
</style>
''')
    write('</head>\n<body>\n')

    # output front matter
    def topSection(sectype, title, text):
        write('<div class="' + sectype + '">')
        write(text)
        write('</div>\n\n')
    def h4Section(sectype, title, text):
        write('<div class="' + sectype + '">')
        write('<h4>' + title + '</h4>\n')
        write(text)
        write('</div>\n\n')
    def paraSection(sectype, title, text):
        write('<p class="' + sectype + '">' + title + '</p>\n\n')
    def titleSection(sectype, title, text):
        write('<h1 class="' + sectype + '">Home Network Administration Protocol (HNAP)<br/>for <b>' + title + '</b></h1>\n\n')
    def versionSection(sectype, title, text):
        paraSection(sectype, 'Version ' + title, text)
    def writeDocSection(sectype, ff):
        for section in model.docSections:
            if section.type == sectype:
                ff(section.type, section.title, docsection_html(section))
                break
    writeDocSection("top", topSection)
    writeDocSection("title", titleSection)
    writeDocSection("version", versionSection)
    writeDocSection("date", paraSection)
    writeDocSection("abstract", h4Section)
    writeDocSection("copyright", h4Section)

    # calculate structures and enumerations
    structs = []
    enums = []
    for type in model.referenced_types():
        if type in model.types:
            if type.isStruct and not type.isArray:
                structs.append(type)
            elif type.isEnum and not type.isBuiltin:
                enums.append(type)

    stuff = [
        ("Services", "service", model.services, show_service),
        ("Actions", "action", model.actions, show_action),
        ("Events", "event", model.events, show_event),
        ("Structures", "struct", structs, show_type_h3),
        ("Enumerations", "enum", enums, show_type_h3)
        ]

    def contents_line(indent, ref, name):
        write('&nbsp;&nbsp;&nbsp;&nbsp;' * indent)
        write('<a href="#%s">%s</a><br/>\n' % (ref, name))

    # output contents start
    write('<h2>Contents</h2>\n')
    write('<p class="contents">\n')

    # output the documentation section contents
    for section in model.docSections:
        m = re.match('^h([1-6])$', section.type)
        if m:
            indent = int(m.group(1)) - 2
            contents_line(indent, anchor('sec', section.title), section.title)

    # output the stuff contents
    for title, link, items, shower in stuff:
        if items:
            contents_line(0, link + 's', title)
            for item in items:
                contents_line(1, anchor(link, item.name), item.name)

    # output contents end
    write('</p>\n')

    # output documentation sections
    for section in model.docSections:
        if section.type in ("h2", "h3", "h4"):
            write('<%s><a name="%s">%s</a></%s>\n' % (section.type, anchor('sec', section.title), section.title, section.type))
            write(docsection_html(section))

    # output stuff
    for title, link, items, shower in stuff:
        if items:
            write('<h2><a name="%ss">%s</a></h2>\n' % (link, title))
            first = True
            for item in items:
                if first:
                    first = False
                else:
                    write('<hr class="before"/>\n')
                shower(item)

    # output document footer
    write('</body>\n</html>\n')


######################################################################

if __name__ == "__main__":
    main()
