#!/bin/bash

set u

echo ">>>>>>>>>>>>>>>>>>>>>>>>"

# 确保在aosp根目录下
cd $ANDROID_BUILD_TOP

read line < ./RomVersion
ROM_VERSION=$line
tmp=`echo $ROM_VERSION | sed 's/[0-9]//g'`
if [ -n "$tmp" ]; then
    echo "The value of RomVersion is malformed \"$ROM_VERSION\""
    exit 1
fi

if [ "$TARGET_PRODUCT" == "aosp_arm" ]; then
    SUPPORT_ABIS="arm"
elif [ "$TARGET_PRODUCT" == "aosp_arm64" ]; then
    SUPPORT_ABIS="arm,arm64"
else 
    echo "Not support abis \"$TARGET_PRODUCT\"."
    exit 1
fi

if [ ! -n "$ANDROID_BUILD_TOP" ]; then
    echo "The \"ANDROID_BUILD_TOP\" environment variable is Null."
    echo "Use the \"lunch\" command to specify the current compile target."
    exit 1
fi

if [ ! -n "$OUT" ]; then
    echo "The \"OUT\" environment variable is Null."
    echo "Use the \"lunch\" command to specify the current compile target."
    exit 1
fi

echo "Start packing..."
# 打包阶段

ROOTFS_PATH=$OUT/vmos_rootfs

rm -rf $ROOTFS_PATH
mkdir -p $ROOTFS_PATH
mkdir -p $ROOTFS_PATH/system

touch $ROOTFS_PATH/guestOSInfo
echo "{" >> $ROOTFS_PATH/guestOSInfo
echo "    \"guestSystemVersion\":\"ANDROID_5\"," >> $ROOTFS_PATH/guestOSInfo
echo "    \"isMultiInstance\":true," >> $ROOTFS_PATH/guestOSInfo
echo "    \"supportAbis\":\"$SUPPORT_ABIS\"," >> $ROOTFS_PATH/guestOSInfo
echo "    \"hasGooglePlay\":false," >> $ROOTFS_PATH/guestOSInfo
echo "    \"hasRoot\":false," >> $ROOTFS_PATH/guestOSInfo
echo "    \"hasXposed\":false," >> $ROOTFS_PATH/guestOSInfo
echo "    \"supportGooglePlay\":true," >> $ROOTFS_PATH/guestOSInfo
echo "    \"supportRoot\":true," >> $ROOTFS_PATH/guestOSInfo
echo "    \"supportXposed\":true," >> $ROOTFS_PATH/guestOSInfo
echo "    \"useHostBattery1\":true," >> $ROOTFS_PATH/guestOSInfo
echo "    \"nsdk\": true," >> $ROOTFS_PATH/guestOSInfo
echo "    \"halver\": 1," >> $ROOTFS_PATH/guestOSInfo
echo "    \"requiredEngineType\":\"YLINKER\"," >> $ROOTFS_PATH/guestOSInfo
echo "    \"minEngineVersion\":211," >> $ROOTFS_PATH/guestOSInfo
echo "    \"romVersion\": $ROM_VERSION" >> $ROOTFS_PATH/guestOSInfo
echo "}" >> $ROOTFS_PATH/guestOSInfo


cp -rf $OUT/system/ $ROOTFS_PATH
cp -rf $OUT/root/proc $ROOTFS_PATH
cp -rf $OUT/root/sbin $ROOTFS_PATH
cp -rf $OUT/root/default.prop $ROOTFS_PATH
cp -rf $OUT/root/file_contexts $ROOTFS_PATH
cp -rf $OUT/root/fstab.goldfish $ROOTFS_PATH
cp -rf $OUT/root/init $ROOTFS_PATH
cp -rf $OUT/root/init_10 $ROOTFS_PATH
cp -rf $OUT/root/init.environ.rc $ROOTFS_PATH
cp -rf $OUT/root/init.goldfish.rc $ROOTFS_PATH
cp -rf $OUT/root/init.rc $ROOTFS_PATH
cp -rf $OUT/root/init.trace.rc $ROOTFS_PATH
cp -rf $OUT/root/init.usb.rc $ROOTFS_PATH
cp -rf $OUT/root/init.zygote32.rc $ROOTFS_PATH
cp -rf $OUT/root/init.zygote64_32.rc $ROOTFS_PATH > /dev/null 2>&1
cp -rf $OUT/root/property_contexts $ROOTFS_PATH
cp -rf $OUT/root/seapp_contexts $ROOTFS_PATH
cp -rf $OUT/root/service_contexts $ROOTFS_PATH
cp -rf $OUT/root/service_contexts $ROOTFS_PATH
cp -rf $OUT/root/ueventd.goldfish.rc $ROOTFS_PATH
cp -rf $OUT/root/ueventd.rc $ROOTFS_PATH
cp -rf $OUT/root/service_contexts $ROOTFS_PATH

touch $ROOTFS_PATH/proc/mounts
chmod 777 $ROOTFS_PATH/proc/mounts
echo "rootfs / rootfs ro,seclabel,relatime 0 0" >> $ROOTFS_PATH/proc/mounts
echo "tmpfs /dev tmpfs ro,seclabel,nosuid,relatime,mode=755 0 0" >> $ROOTFS_PATH/proc/mounts
echo "devpts /dev/pts devpts ro,seclabel,relatime,mode=600 0 0" >> $ROOTFS_PATH/proc/mounts
echo "proc /proc proc ro,relatime 0 0" >> $ROOTFS_PATH/proc/mounts
echo "sysfs /sys sysfs ro,seclabel,relatime 0 0" >> $ROOTFS_PATH/proc/mounts
echo "selinuxfs /sys/fs/selinux selinuxfs ro,relatime 0 0" >> $ROOTFS_PATH/proc/mounts
echo "debugfs /sys/kernel/debug debugfs ro,relatime 0 0" >> $ROOTFS_PATH/proc/mounts
echo "none /acct cgroup ro,relatime,cpuacct 0 0" >> $ROOTFS_PATH/proc/mounts
echo "none /sys/fs/cgroup tmpfs ro,seclabel,relatime,mode=750,gid=1000 0 0" >> $ROOTFS_PATH/proc/mounts
echo "tmpfs /mnt/asec tmpfs ro,seclabel,relatime,mode=755,gid=1000 0 0" >> $ROOTFS_PATH/proc/mounts
echo "tmpfs /mnt/obb tmpfs ro,seclabel,relatime,mode=755,gid=1000 0 0" >> $ROOTFS_PATH/proc/mounts
echo "none /dev/cpuctl cgroup ro,relatime,cpu 0 0" >> $ROOTFS_PATH/proc/mounts
echo "/dev/block/platform/msm_sdcc.1/by-name/system /system ext4 ro,seclabel,relatime,data=ordered 0 0" >> $ROOTFS_PATH/proc/mounts
echo "/dev/block/platform/msm_sdcc.1/by-name/userdata /data ext4 ro,seclabel,nosuid,nodev,noatime,nomblk_io_submit,noauto_da_alloc,errors=panic,data=ordered 0 0" >> $ROOTFS_PATH/proc/mounts
echo "/dev/block/platform/msm_sdcc.1/by-name/cache /cache ext4 ro,seclabel,nosuid,nodev,noatime,nomblk_io_submit,noauto_da_alloc,errors=panic,data=ordered 0 0" >> $ROOTFS_PATH/proc/mounts
echo "/dev/block/platform/msm_sdcc.1/by-name/persist /persist ext4 ro,seclabel,nosuid,nodev,relatime,nomblk_io_submit,nodelalloc,errors=panic,data=ordered 0 0" >> $ROOTFS_PATH/proc/mounts
echo "/dev/block/platform/msm_sdcc.1/by-name/modem /firmware vfat ro,context=u:object_r:firmware_file:s0,relatime,uid=1000,gid=1000,fmask=0337,dmask=0227,codepage=cp437,iocharset=iso8859-1,shortname=lower,errors=remount-rw 0 0" >> $ROOTFS_PATH/proc/mounts
echo "/dev/fuse /mnt/shell/emulated fuse ro,nosuid,nodev,noexec,relatime,user_id=1023,group_id=1023,default_permissions,allow_other 0 0" >> $ROOTFS_PATH/proc/mounts

touch $ROOTFS_PATH/proc/maps

# 压缩阶段
VMOS_DST_ZIP_FILE=$ANDROID_BUILD_TOP/vmos_51_$TARGET_PRODUCT-$TARGET_BUILD_VARIANT.zip

echo "Start compressing files..."
rm $VMOS_DST_ZIP_FILE > /dev/null 2>&1
cd $ROOTFS_PATH

ZIP_CMD=$(which zip)
if [ ! -n "$ZIP_CMD" ]; then
    echo "No \"zip\" command found, interrupt the compression program."
    exit 1
fi

$ZIP_CMD -r $VMOS_DST_ZIP_FILE * > /dev/null

if [ -f $VMOS_DST_ZIP_FILE ]; then
    echo "Packing success, The output file is \"$VMOS_DST_ZIP_FILE\"."
else
    echo "Packing failure, The system image package is not generated."
fi


echo "<<<<<<<<<<<<<<<<<<<<<<<<"