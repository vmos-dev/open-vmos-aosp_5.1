#!/usr/bin/env python
#
# Copyright (C) 2013 The Android Open Source Project
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

# diff_products.py product_mk_1 [product_mk_2]
# compare two product congifuraitons or analyze one product configuration.
# List PRODUCT_PACKAGES, PRODUCT_COPY_FILES, and etc.


import os
import sys


PRODUCT_KEYWORDS = [
    "PRODUCT_PACKAGES",
    "PRODUCT_COPY_FILES",
    "PRODUCT_PROPERTY_OVERRIDES" ]

# Top level data
# { "PRODUCT_PACKAGES": {...}}
# PRODCT_PACKAGES { "libstagefright": "path_to_the_mk_file" }

def removeTrailingParen(path):
    if path.endswith(")"):
        return path[:-1]
    else:
        return path

def substPathVars(path, parentPath):
    path_ = path.replace("$(SRC_TARGET_DIR)", "build/target")
    path__ = path_.replace("$(LOCAL_PATH)", os.path.dirname(parentPath))
    return path__


def parseLine(line, productData, productPath, overrideProperty = False):
    #print "parse:" + line
    words = line.split()
    if len(words) < 2:
        return
    if words[0] in PRODUCT_KEYWORDS:
        # Override only for include
        if overrideProperty and words[1] == ":=":
            if len(productData[words[0]]) != 0:
                print "** Warning: overriding property " + words[0] + " that was:" + \
                      productData[words[0]]
            productData[words[0]] = {}
        d = productData[words[0]]
        for word in words[2:]:
            # TODO: parsing those $( cases in better way
            if word.startswith("$(foreach"): # do not parse complex calls
                print "** Warning: parseLine too complex line in " + productPath + " : " + line
                return
            d[word] = productPath
    elif words[0] == "$(call" and words[1].startswith("inherit-product"):
        parseProduct(substPathVars(removeTrailingParen(words[2]), productPath), productData)
    elif words[0] == "include":
        parseProduct(substPathVars(words[1], productPath), productData, True)
    elif words[0] == "-include":
        parseProduct(substPathVars(words[1], productPath), productData, True)

def parseProduct(productPath, productData, overrideProperty = False):
    """parse given product mk file and add the result to productData dict"""
    if not os.path.exists(productPath):
        print "** Warning cannot find file " + productPath
        return

    for key in PRODUCT_KEYWORDS:
        if not key in productData:
            productData[key] = {}

    multiLineBuffer = [] #for storing multiple lines
    inMultiLine = False
    for line in open(productPath):
        line_ = line.strip()
        if inMultiLine:
            if line_.endswith("\\"):
                multiLineBuffer.append(line_[:-1])
            else:
                multiLineBuffer.append(line_)
                parseLine(" ".join(multiLineBuffer), productData, productPath)
                inMultiLine = False
        else:
            if line_.endswith("\\"):
                inMultiLine = True
                multiLineBuffer = []
                multiLineBuffer.append(line_[:-1])
            else:
                parseLine(line_, productData, productPath)
    #print productData

def printConf(confList):
    for key in PRODUCT_KEYWORDS:
        print " *" + key
        if key in confList:
            for (k, path) in confList[key]:
                print "  " + k + ": " + path

def diffTwoProducts(productL, productR):
    """compare two products and comapre in the order of common, left only, right only items.
       productL and productR are dictionary"""
    confCommon = {}
    confLOnly = {}
    confROnly = {}
    for key in PRODUCT_KEYWORDS:
        dL = productL[key]
        dR = productR[key]
        confCommon[key] = []
        confLOnly[key] = []
        confROnly[key] = []
        for keyL in sorted(dL.keys()):
            if keyL in dR:
                if dL[keyL] == dR[keyL]:
                    confCommon[key].append((keyL, dL[keyL]))
                else:
                    confCommon[key].append((keyL, dL[keyL] + "," + dR[keyL]))
            else:
                confLOnly[key].append((keyL, dL[keyL]))
        for keyR in sorted(dR.keys()):
            if not keyR in dL: # right only
                confROnly[key].append((keyR, dR[keyR]))
    print "==Common=="
    printConf(confCommon)
    print "==Left Only=="
    printConf(confLOnly)
    print "==Right Only=="
    printConf(confROnly)

def main(argv):
    if len(argv) < 2:
        print "diff_products.py product_mk_1 [product_mk_2]"
        print " compare two product mk files (or just list single product)"
        print " it must be executed from android source tree root."
        print " ex) diff_products.py device/asus/grouper/full_grouper.mk " + \
              " device/asus/tilapia/full_tilapia.mk"
        sys.exit(1)

    productLPath = argv[1]
    productRPath = None
    if len(argv) == 3:
        productRPath = argv[2]

    productL = {}
    productR = {}
    parseProduct(productLPath, productL)
    if productRPath is None:
        for key in PRODUCT_KEYWORDS:
            productR[key] = {}

    else:
        parseProduct(productRPath, productR)

    diffTwoProducts(productL, productR)


if __name__ == '__main__':
    main(sys.argv)
