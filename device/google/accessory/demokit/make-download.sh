#!/bin/bash
# This script packages the ADK1 files into a downloadable zip
# by Joe Fernandez, June 2012
#
# creates a zip for downloading with the following structure:
# /app (demokit Android app)
# /arduino_libs
#   /AndroidAccessory
#     /examples/demokit (added here for ease of use with Arduino)
#   /USB_Host_Shield
# /hardware
# COPYING
# README


# Generic pause function
function pause {
    read -p " Press Enter to continue..."
}

# move up to accessory directory
cd ..

# Main execution
dateStamp=`date +"%Y%m%d"`

# create the directory structure
mkdir -p ADK_release_${dateStamp}/arduino_libs/AndroidAccessory/examples

# move the demokit firmware into the AndroidAccessory library, 
# so that it shows up in the Arduino IDE menus:
cp -r demokit/firmware/* ADK_release_${dateStamp}/arduino_libs/AndroidAccessory/examples

# copy in the app and hardware files
cp -r demokit/app ADK_release_${dateStamp}
cp -r demokit/hardware ADK_release_${dateStamp}

# copy in the README and license info
cp demokit/COPYING ADK_release_${dateStamp}
cp demokit/README ADK_release_${dateStamp}

# copy in the Arduino libraries and remove the make file
cp -r arduino/* ADK_release_${dateStamp}/arduino_libs
rm -f ADK_release_${dateStamp}/arduino_libs/Android.mk

echo "packaged directories assembled. Next: create zip"
#pause

# create the zip download
if [ -e ADK_release_${dateStamp}.zip ]; then
    rm -f ADK_release_${dateStamp}.zip
fi
zip -r ADK_release_${dateStamp}.zip ADK_release_${dateStamp}/*

echo "download zip assembled. Next: remove package directories"
#pause

rm -rf ADK_release_${dateStamp}



