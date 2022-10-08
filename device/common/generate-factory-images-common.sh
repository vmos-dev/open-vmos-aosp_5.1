# Copyright 2011 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Use the default values if they weren't explicitly set
if test "$XLOADERSRC" = ""
then
  XLOADERSRC=xloader.img
fi
if test "$BOOTLOADERSRC" = ""
then
  BOOTLOADERSRC=bootloader.img
fi
if test "$RADIOSRC" = ""
then
  RADIOSRC=radio.img
fi
if test "$SLEEPDURATION" = ""
then
  SLEEPDURATION=5
fi

# Prepare the staging directory
rm -rf tmp
mkdir -p tmp/$PRODUCT-$VERSION

# Extract the bootloader(s) and radio(s) as necessary
if test "$XLOADER" != ""
then
  unzip -d tmp ${SRCPREFIX}$PRODUCT-target_files-$BUILD.zip RADIO/$XLOADERSRC
fi
if test "$BOOTLOADERFILE" = ""
then
  unzip -d tmp ${SRCPREFIX}$PRODUCT-target_files-$BUILD.zip RADIO/$BOOTLOADERSRC
fi
if test "$RADIO" != "" -a "$RADIOFILE" = ""
then
  unzip -d tmp ${SRCPREFIX}$PRODUCT-target_files-$BUILD.zip RADIO/$RADIOSRC
fi
if test "$CDMARADIO" != "" -a "$CDMARADIOFILE" = ""
then
  unzip -d tmp ${SRCPREFIX}$PRODUCT-target_files-$BUILD.zip RADIO/radio-cdma.img
fi

# Copy the various images in their staging location
cp ${SRCPREFIX}$PRODUCT-img-$BUILD.zip tmp/$PRODUCT-$VERSION/image-$PRODUCT-$VERSION.zip
if test "$XLOADER" != ""
then
  cp tmp/RADIO/$XLOADERSRC tmp/$PRODUCT-$VERSION/xloader-$DEVICE-$XLOADER.img
fi
if test "$BOOTLOADERFILE" = ""
then
  cp tmp/RADIO/$BOOTLOADERSRC tmp/$PRODUCT-$VERSION/bootloader-$DEVICE-$BOOTLOADER.img
else
  cp $BOOTLOADERFILE tmp/$PRODUCT-$VERSION/bootloader-$DEVICE-$BOOTLOADER.img
fi
if test "$RADIO" != ""
then
  if test "$RADIOFILE" = ""
  then
    cp tmp/RADIO/$RADIOSRC tmp/$PRODUCT-$VERSION/radio-$DEVICE-$RADIO.img
  else
    cp $RADIOFILE tmp/$PRODUCT-$VERSION/radio-$DEVICE-$RADIO.img
  fi
fi
if test "$CDMARADIO" != ""
then
  if test "$CDMARADIOFILE" = ""
  then
    cp tmp/RADIO/radio-cdma.img tmp/$PRODUCT-$VERSION/radio-cdma-$DEVICE-$CDMARADIO.img
  else
    cp $CDMARADIOFILE tmp/$PRODUCT-$VERSION/radio-cdma-$DEVICE-$CDMARADIO.img
  fi
fi

# Write flash-all.sh
cat > tmp/$PRODUCT-$VERSION/flash-all.sh << EOF
#!/bin/sh

# Copyright 2012 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

EOF
if test "$UNLOCKBOOTLOADER" = "true"
then
cat >> tmp/$PRODUCT-$VERSION/flash-all.sh << EOF
fastboot oem unlock
EOF
fi
if test "$ERASEALL" = "true"
then
cat >> tmp/$PRODUCT-$VERSION/flash-all.sh << EOF
fastboot erase boot
fastboot erase cache
fastboot erase recovery
fastboot erase system
fastboot erase userdata
EOF
fi
if test "$XLOADER" != ""
then
cat >> tmp/$PRODUCT-$VERSION/flash-all.sh << EOF
fastboot flash xloader xloader-$DEVICE-$XLOADER.img
EOF
fi
cat >> tmp/$PRODUCT-$VERSION/flash-all.sh << EOF
fastboot flash bootloader bootloader-$DEVICE-$BOOTLOADER.img
EOF
if test "$TWINBOOTLOADERS" = "true"
then
cat >> tmp/$PRODUCT-$VERSION/flash-all.sh << EOF
fastboot flash bootloader2 bootloader-$DEVICE-$BOOTLOADER.img
EOF
fi
cat >> tmp/$PRODUCT-$VERSION/flash-all.sh << EOF
fastboot reboot-bootloader
sleep $SLEEPDURATION
EOF
if test "$RADIO" != ""
then
cat >> tmp/$PRODUCT-$VERSION/flash-all.sh << EOF
fastboot flash radio radio-$DEVICE-$RADIO.img
fastboot reboot-bootloader
sleep $SLEEPDURATION
EOF
fi
if test "$CDMARADIO" != ""
then
cat >> tmp/$PRODUCT-$VERSION/flash-all.sh << EOF
fastboot flash radio-cdma radio-cdma-$DEVICE-$CDMARADIO.img
fastboot reboot-bootloader
sleep $SLEEPDURATION
EOF
fi
cat >> tmp/$PRODUCT-$VERSION/flash-all.sh << EOF
fastboot -w update image-$PRODUCT-$VERSION.zip
EOF
chmod a+x tmp/$PRODUCT-$VERSION/flash-all.sh

# Write flash-all.bat
cat > tmp/$PRODUCT-$VERSION/flash-all.bat << EOF
@ECHO OFF
:: Copyright 2012 The Android Open Source Project
::
:: Licensed under the Apache License, Version 2.0 (the "License");
:: you may not use this file except in compliance with the License.
:: You may obtain a copy of the License at
::
::      http://www.apache.org/licenses/LICENSE-2.0
::
:: Unless required by applicable law or agreed to in writing, software
:: distributed under the License is distributed on an "AS IS" BASIS,
:: WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
:: See the License for the specific language governing permissions and
:: limitations under the License.

PATH=%PATH%;"%SYSTEMROOT%\System32"
EOF
if test "$UNLOCKBOOTLOADER" = "true"
then
cat >> tmp/$PRODUCT-$VERSION/flash-all.bat << EOF
fastboot oem unlock
EOF
fi
if test "$ERASEALL" = "true"
then
cat >> tmp/$PRODUCT-$VERSION/flash-all.bat << EOF
fastboot erase boot
fastboot erase cache
fastboot erase recovery
fastboot erase system
fastboot erase userdata
EOF
fi
if test "$XLOADER" != ""
then
cat >> tmp/$PRODUCT-$VERSION/flash-all.bat << EOF
fastboot flash xloader xloader-$DEVICE-$XLOADER.img
EOF
fi
cat >> tmp/$PRODUCT-$VERSION/flash-all.bat << EOF
fastboot flash bootloader bootloader-$DEVICE-$BOOTLOADER.img
EOF
if test "$TWINBOOTLOADERS" = "true"
then
cat >> tmp/$PRODUCT-$VERSION/flash-all.bat << EOF
fastboot flash bootloader2 bootloader-$DEVICE-$BOOTLOADER.img
EOF
fi
cat >> tmp/$PRODUCT-$VERSION/flash-all.bat << EOF
fastboot reboot-bootloader
ping -n $SLEEPDURATION 127.0.0.1 >nul
EOF
if test "$RADIO" != ""
then
cat >> tmp/$PRODUCT-$VERSION/flash-all.bat << EOF
fastboot flash radio radio-$DEVICE-$RADIO.img
fastboot reboot-bootloader
ping -n $SLEEPDURATION 127.0.0.1 >nul
EOF
fi
if test "$CDMARADIO" != ""
then
cat >> tmp/$PRODUCT-$VERSION/flash-all.bat << EOF
fastboot flash radio-cdma radio-cdma-$DEVICE-$CDMARADIO.img
fastboot reboot-bootloader
ping -n $SLEEPDURATION 127.0.0.1 >nul
EOF
fi
cat >> tmp/$PRODUCT-$VERSION/flash-all.bat << EOF
fastboot -w update image-$PRODUCT-$VERSION.zip

echo Press any key to exit...
pause >nul
exit
EOF

# Write flash-base.sh
cat > tmp/$PRODUCT-$VERSION/flash-base.sh << EOF
#!/bin/sh

# Copyright 2012 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

EOF
if test "$XLOADER" != ""
then
cat >> tmp/$PRODUCT-$VERSION/flash-base.sh << EOF
fastboot flash xloader xloader-$DEVICE-$XLOADER.img
EOF
fi
cat >> tmp/$PRODUCT-$VERSION/flash-base.sh << EOF
fastboot flash bootloader bootloader-$DEVICE-$BOOTLOADER.img
EOF
if test "$TWINBOOTLOADERS" = "true"
then
cat >> tmp/$PRODUCT-$VERSION/flash-base.sh << EOF
fastboot flash bootloader2 bootloader-$DEVICE-$BOOTLOADER.img
EOF
fi
cat >> tmp/$PRODUCT-$VERSION/flash-base.sh << EOF
fastboot reboot-bootloader
sleep $SLEEPDURATION
EOF
if test "$RADIO" != ""
then
cat >> tmp/$PRODUCT-$VERSION/flash-base.sh << EOF
fastboot flash radio radio-$DEVICE-$RADIO.img
fastboot reboot-bootloader
sleep $SLEEPDURATION
EOF
fi
if test "$CDMARADIO" != ""
then
cat >> tmp/$PRODUCT-$VERSION/flash-base.sh << EOF
fastboot flash radio-cdma radio-cdma-$DEVICE-$CDMARADIO.img
fastboot reboot-bootloader
sleep $SLEEPDURATION
EOF
fi
chmod a+x tmp/$PRODUCT-$VERSION/flash-base.sh

# Create the distributable package
(cd tmp ; tar zcvf ../$PRODUCT-$VERSION-factory.tgz $PRODUCT-$VERSION)
mv $PRODUCT-$VERSION-factory.tgz $PRODUCT-$VERSION-factory-$(sha1sum < $PRODUCT-$VERSION-factory.tgz | cut -b -8).tgz

# Clean up
rm -rf tmp
