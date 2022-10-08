#!/usr/bin/env python
#
# Copyright (C) 2012 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the 'License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# prepare_pdk_tree.py target_dir [-m manifest] pdk_groups
# Ex: prepare_pdk_tree.py ../tmp/pdk grouper
# create mount_pdk.sh and umount_pdk.sh, which mounts/umounts pdk sources.

import os
import re
import sys
import subprocess


class ManifestHandler(object):

    def __init__(self):
        # current pattern
        self.current = 0
        self.patterns = [re.compile('path=\"([^\"]*)\".*groups=\"([^\"]*)\"'), \
                         re.compile('groups=\"([^\"]*)\".*path=\"([^\"]*)\"')]

    def getAttribs(self, line):
        attrib = [None, None] # list of path, groups
        m = self.patterns[self.current].search(line)
        # if match fails, try both pattens and change default one
        # if match founds
        if m is None:
            notCurrent = 1 - self.current
            mOther = self.patterns[notCurrent].search(line)
            if mOther is not None:
                # toggle
                self.current = notCurrent
                m = mOther
        if m is not None:
            if (self.current == 0):
                attrib[0] = m.group(1)
                attrib[1] = m.group(2)
            else:
                attrib[0] = m.group(2)
                attrib[1] = m.group(1)
        return attrib

def isInGroups(groupsAttrib, groups):
    if groupsAttrib is None:
        return False
    groupsAttribList = groupsAttrib.split(',')
    for group in groups:
        if group in groupsAttribList:
            return True
    return False

def getPDKDirs(manifest, groups):
    subdirs = []
    handler = ManifestHandler()
    f = open(manifest, 'r')
    for line in f:
        [path, groupsAttrib] = handler.getAttribs(line)
        if isInGroups(groupsAttrib, groups):
            subdirs.append(path)
    f.close()
    return subdirs

def create_symbolic_link(src_top, dest_top, dir_name):
    src_full = src_top + "/" + dir_name
    dest_full = dest_top + "/" + dir_name
    #print "create symbolic link from " + dest_full + " to " + src_full
    # remove existing link first to prevent recursive loop
    os.system("rm -rf " + dest_full)
    os.system("ln -s " + src_full + " " + dest_full)

# The only file not from manifest.
copy_files_list = [ "Makefile" ]
MOUNT_FILE = 'mount_pdk.sh'
UMOUNT_FILE = 'umount_pdk.sh'
SH_HEADER = "#!/bin/bash\n#Auto-generated file, do not edit!\n"

def main(argv):
    manifestFile = ".repo/manifest.xml"
    groups = ["pdk"]
    if len(argv) < 2:
        print "create_pdk_tree.py target_dir [-m manifest] [-a dir_to_add] pdk_groups"
        print " ex) create_pdk_tree.py ../tmp grouper"
        print " -a option is to include a directory which does not belong to specified group"
        print "   multiple -a options can be specified like -a frameworks/base -a external/aaa"
        print " Note that pdk group is included by default"
        print " Do not create target_dir under the current source tree. This will cause build error."
        sys.exit(1)
    targetDir = argv[1]
    argc = 2
    subdirs = []
    if len(argv) > 2:
        if argv[2] == "-m":
            manifestFile = argv[3]
            argc += 2
    while argc < len(argv):
        if argv[argc] == "-a":
            argc += 1
            subdirs.append(argv[argc])
        else:
            groups.append(argv[argc])
        argc += 1
    sourceDir = os.path.abspath('.')
    targetDir = os.path.abspath(targetDir)

    p = subprocess.Popen("mount", stdout = subprocess.PIPE)
    targetMounted = False
    for line in p.stdout:
        if targetDir in line:
            targetMounted = True
    p.stdout.close()

    if targetMounted:
        print "target dir already mounted"
        if os.path.exists(targetDir + '/' + UMOUNT_FILE):
            print "Use existing file", UMOUNT_FILE, "to unmount"
            sys.exit(1)
        else:
            print "Will create scripts, but may need manual unmount"

    subdirs += getPDKDirs(manifestFile, groups)
    print subdirs
    os.system("mkdir -p " + targetDir)
    mountf = open(targetDir + '/' + MOUNT_FILE, 'w+')
    mountf.write(SH_HEADER)
    umountf = open(targetDir + '/' + UMOUNT_FILE, 'w+')
    umountf.write(SH_HEADER)
    for subdir in subdirs:
        os.system("mkdir -p " + targetDir + '/' + subdir)
        mountf.write("mount --bind " + sourceDir + "/" + subdir + " " + targetDir + "/" + subdir + \
                        "\n")
        umountf.write("umount " + targetDir + "/" + subdir + "\n")
    for file_name in copy_files_list:
        create_symbolic_link(sourceDir, targetDir, file_name)
    mountf.close()
    umountf.close()
    os.system("chmod 700 " + targetDir + '/' + MOUNT_FILE)
    os.system("chmod 700 " + targetDir + '/' + UMOUNT_FILE)

if __name__ == '__main__':
    main(sys.argv)
