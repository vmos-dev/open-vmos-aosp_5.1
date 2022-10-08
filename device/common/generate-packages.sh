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

if [ $# != 6 ]
then
  echo Usage: $0 BUILD_ID BUILD ROOTDEVICE DEVICE MANUFACTURER PRODUCT
  echo Example: $0 1075408 KOT49Y mako mako lge occam
fi

ZIP_TYPE=target_files  # ota | target_files

ZIP=$6-$ZIP_TYPE-$1.zip
BUILD=$2

ROOTDEVICE=$3
DEVICE=$4
MANUFACTURER=$5

cd ../$MANUFACTURER/$ROOTDEVICE/self-extractors || echo Error change dir

EXTRACT_LIST_FILENAME=extract-lists.txt

for COMPANY in `grep "[a-z|A-Z])" $EXTRACT_LIST_FILENAME | cut -f1 -d')'`
do
  echo Processing files from $COMPANY
  rm -rf tmp
  FILEDIR=tmp/vendor/$COMPANY/$DEVICE/proprietary
  mkdir -p $FILEDIR
  mkdir -p tmp/vendor/$MANUFACTURER/$ROOTDEVICE

  TO_EXTRACT=`sed -n -e '/'"  $COMPANY"'/,/;;/ p' $EXTRACT_LIST_FILENAME | tail -n+3 | head -n-2 | sed -e 's/\\\//g'`

  # Check if TO_EXTRACT list has any APK files
  if [[ ${TO_EXTRACT} == *.apk* ]]
  then
    APK_MAKEFILE=${FILEDIR}/Android.mk
    echo "LOCAL_PATH := \$(call my-dir)" > ${APK_MAKEFILE}
    echo "" >> ${APK_MAKEFILE}
  fi

  echo \ \ Extracting files from OTA package
  for ONE_FILE in $TO_EXTRACT
  do
    if test ${ZIP_TYPE} = target_files
    then
      ONE_FILE=`echo $ONE_FILE | sed 's/system\//SYSTEM\//g'`
    fi

    if [[ $ONE_FILE == */lib64/* ]]
    then
      FILEDIR_NEW=$FILEDIR/lib64
    else
      FILEDIR_NEW=$FILEDIR
    fi

    # apk makefile
    if [[ ${ONE_FILE} == *.apk ]]
    then
      TMP_ONE_FILE_NAME=$(basename ${ONE_FILE} | sed 's/.apk//g')

      echo "include \$(CLEAR_VARS)" >> ${APK_MAKEFILE}

      echo "LOCAL_MODULE_SUFFIX := \$(COMMON_ANDROID_PACKAGE_SUFFIX)" >> ${APK_MAKEFILE}
      echo "LOCAL_MODULE := ${TMP_ONE_FILE_NAME}" >> ${APK_MAKEFILE}
      echo "LOCAL_MODULE_TAGS := optional" >> ${APK_MAKEFILE}
      echo "LOCAL_BUILT_MODULE_STEM := package.apk" >> ${APK_MAKEFILE}
      echo "LOCAL_MODULE_OWNER := ${COMPANY}" >> ${APK_MAKEFILE}
      echo "LOCAL_MODULE_CLASS := APPS" >> ${APK_MAKEFILE}
      echo "LOCAL_SRC_FILES := \$(LOCAL_MODULE).apk" >> ${APK_MAKEFILE}
      echo "LOCAL_CERTIFICATE := PRESIGNED" >> ${APK_MAKEFILE}

      if [[ ${TMP_ONE_FILE_NAME} == "LeanbackLauncher" ]]
      then
        echo "LOCAL_OVERRIDES_PACKAGES := Launcher2" >> ${APK_MAKEFILE}
      fi

      echo "include \$(BUILD_PREBUILT)" >> ${APK_MAKEFILE}
      echo "" >> ${APK_MAKEFILE}
    fi

    echo \ \ \ \ Extracting $ONE_FILE
    unzip -j -o $ZIP $ONE_FILE -d $FILEDIR_NEW> /dev/null || echo \ \ \ \ Error extracting $ONE_FILE
    if test ${ONE_FILE,,} = system/vendor/bin/gpsd -o ${ONE_FILE,,} = system/vendor/bin/pvrsrvinit -o ${ONE_FILE,,} = system/bin/fRom
    then
      chmod a+x $FILEDIR_NEW/$(basename $ONE_FILE) || echo \ \ \ \ Error chmoding $ONE_FILE
    fi

    ONE_FILE_BASE=$(basename $ONE_FILE)
    if [[ $ONE_FILE_BASE == *atmel-a432-*-shamu-p1.tdat ]]
    then
      ATMEL_FILE=$(ls $FILEDIR_NEW/$ONE_FILE_BASE | cut -f6 -d'/')
      sed -i "s/$ONE_FILE_BASE/$ATMEL_FILE/" moto/staging/device-partial.mk
    elif [[ $ONE_FILE_BASE == *atmel-a432-*-shamu.tdat ]]
    then
      ATMEL_FILE=$(ls $FILEDIR_NEW/$ONE_FILE_BASE | cut -f6 -d'/')
      sed -i "s/$ONE_FILE_BASE/$ATMEL_FILE/" moto/staging/device-partial.mk
    fi

  done
  echo \ \ Setting up $COMPANY-specific makefiles
  cp -R $COMPANY/staging/* tmp/vendor/$COMPANY/$DEVICE || echo \ \ \ \ Error copying makefiles
  echo \ \ Setting up shared makefiles
  cp -R root/* tmp/vendor/$MANUFACTURER/$ROOTDEVICE || echo \ \ \ \ Error copying makefiles
  echo \ \ Generating self-extracting script
  SCRIPT=extract-$COMPANY-$DEVICE.sh
  cat PROLOGUE > tmp/$SCRIPT || echo \ \ \ \ Error generating script
  cat $COMPANY/COPYRIGHT >> tmp/$SCRIPT || echo \ \ \ \ Error generating script
  cat PART1 >> tmp/$SCRIPT || echo \ \ \ \ Error generating script
  cat $COMPANY/LICENSE >> tmp/$SCRIPT || echo \ \ \ \ Error generating script
  cat PART2 >> tmp/$SCRIPT || echo \ \ \ \ Error generating script
  echo tail -n +$(expr 2 + $(cat PROLOGUE $COMPANY/COPYRIGHT PART1 $COMPANY/LICENSE PART2 PART3 | wc -l)) \$0 \| tar zxv >> tmp/$SCRIPT || echo \ \ \ \ Error generating script
  cat PART3 >> tmp/$SCRIPT || echo \ \ \ \ Error generating script
  (cd tmp ; tar zc --owner=root --group=root vendor/ >> $SCRIPT || echo \ \ \ \ Error generating embedded tgz)
  chmod a+x tmp/$SCRIPT || echo \ \ \ \ Error generating script
  ARCHIVE=$COMPANY-$DEVICE-$BUILD-$(md5sum < tmp/$SCRIPT | cut -b -8 | tr -d \\n).tgz
  rm -f $ARCHIVE
  echo \ \ Generating final archive
  (cd tmp ; tar --owner=root --group=root -z -c -f ../$ARCHIVE $SCRIPT || echo \ \ \ \ Error archiving script)
  rm -rf tmp
done
