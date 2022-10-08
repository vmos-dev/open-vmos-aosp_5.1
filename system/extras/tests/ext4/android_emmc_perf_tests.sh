#!/bin/bash

PERF="rand_emmc_perf"
PERF_LOC=/dev
STATS_FILE="/data/local/tmp/stats_test"
STATS_MODE=0
USERBUILD_MODE=0

if [ "$1" = "-s" ]
then
  STATS_MODE=1
elif [ "$1" = "-u" ]
then
  USERBUILD_MODE=1
fi

if [ ! -r "$PERF" ]
then
  echo "Cannot read $PERF test binary"
fi

if ! adb shell true >/dev/null 2>&1
then
  echo "No device detected over adb"
fi

HARDWARE=`adb shell getprop ro.hardware | tr -d "\r"`

case "$HARDWARE" in
  tuna | steelhead)
    CPUFREQ="/sys/devices/system/cpu/cpu0/cpufreq"
    CACHE="/dev/block/platform/omap/omap_hsmmc.0/by-name/cache"
    MMCDEV="mmcblk0"
    ;;

  stingray | wingray)
    CPUFREQ="/sys/devices/system/cpu/cpu0/cpufreq"
    CACHE="/dev/block/platform/sdhci-tegra.3/by-name/cache"
    MMCDEV="mmcblk0"
    ;;

  herring)
    echo "This test will wipe the userdata partition on $HARDWARE devices."
    read -p "Do you want to proceed? " ANSWER

    if [ "$ANSWER" != "yes" ]
    then
      echo "aborting test"
      exit 1
    fi

    CPUFREQ="/sys/devices/system/cpu/cpu0/cpufreq"
    CACHE="/dev/block/platform/s3c-sdhci.0/by-name/userdata"
    MMCDEV="mmcblk0"
    ;;

  grouper)
    CPUFREQ="/sys/devices/system/cpu/cpu0/cpufreq"
    CACHE="/dev/block/platform/sdhci-tegra.3/by-name/CAC"
    MMCDEV="mmcblk0"
    ;;

  manta)
    CPUFREQ="/sys/devices/system/cpu/cpu0/cpufreq"
    CACHE="/dev/block/platform/dw_mmc.0/by-name/cache"
    MMCDEV="mmcblk0"
    ;;

  flo)
    CPUFREQ="/sys/devices/system/cpu/cpu0/cpufreq"
    CACHE="dev/block/platform/msm_sdcc.1/by-name/cache"
    MMCDEV="mmcblk0"
    ;;

  *)
    echo "Unknown hardware $HARDWARE.  Exiting."
    exit 1
esac

# We cannot stop and unmount stuff in a user build, so don't even try.
if [ "$USERBUILD_MODE" -eq 0 ]
then
  # prepare the device
  adb root
  adb wait-for-device
  adb push "$PERF" /dev
  adb shell stop
  adb shell stop sdcard
  adb shell stop ril-daemon
  adb shell stop media
  adb shell stop drm
  adb shell stop keystore
  adb shell stop tf_daemon
  adb shell stop bluetoothd
  adb shell stop hciattach
  adb shell stop p2p_supplicant
  adb shell stop wpa_supplicant
  adb shell stop mobicore
  adb shell umount /sdcard >/dev/null 2>&1
  adb shell umount /mnt/sdcard >/dev/null 2>&1
  adb shell umount /mnt/shell/sdcard0 >/dev/null 2>&1
  adb shell umount /mnt/shell/emulated >/dev/null 2>&1
  adb shell umount /cache >/dev/null 2>&1
  if [ "$STATS_MODE" -ne 1 ]
  then
    adb shell umount /data >/dev/null 2>&1
  fi
else
  # For user builds, put the $PERF binary in /data/local/tmp,
  # and also setup CACHE to point to a file on /data/local/tmp,
  # and create that file
  PERF_LOC=/data/local/tmp
  adb push "$PERF" "$PERF_LOC"
  CACHE=/data/local/tmp/testfile
  echo "Creating testfile for user builds (can take up to 60 seconds)"
  adb shell dd if=/dev/zero of=$CACHE bs=1048576 count=512
fi

# Add more services here that other devices need to stop.
# So far, this list is sufficient for:
#   Prime

if [ "$USERBUILD_MODE" -eq 0 ]
then
  # At this point, the device is quiescent, need to crank up the cpu speed,
  # then run tests
  adb shell "cat $CPUFREQ/cpuinfo_max_freq > $CPUFREQ/scaling_max_freq"
  adb shell "cat $CPUFREQ/cpuinfo_max_freq > $CPUFREQ/scaling_min_freq"
fi

# Start the tests

if [ "$STATS_MODE" -eq 1 ]
then
  # This test looks for the average and max random write times for the emmc
  # chip.  It should be run with the emmc chip full for worst case numbers,
  # and after fstrim for best case numbers.  So first fill the chip, twice,
  # then run the test, then remove the large file, run fstrim, and run the
  # test again.

  # Remove the test file if it exists, then make it anew.
  echo "Filling userdata"
  adb shell rm  -f "$STATS_FILE"
  adb shell dd if=/dev/zero of="$STATS_FILE" bs=1048576
  adb shell sync

  # Do it again to make sure to fill up all the reserved blocks used for
  # wear levelling, plus any unused blocks in the other partitions.  Yes,
  # this is not precise, just a good heuristic.
  echo "Filling userdata again"
  adb shell rm "$STATS_FILE"
  adb shell sync
  adb shell dd if=/dev/zero of="$STATS_FILE" bs=1048576
  adb shell sync

  # Run the test
  echo "Running stats test after filling emmc chip"
  adb shell /dev/$PERF -w -o -s 20000 -f /dev/full_stats 400 "$CACHE"

  # Remove the file, and have vold do fstrim
  adb shell rm "$STATS_FILE"
  adb shell sync
  # Make sure fstrim knows there is work to do
  sleep 10

  # Get the current number of FSTRIM complete lines in thh logcat
  ORIGCNT=`adb shell logcat -d | grep -c "Finished fstrim work"`

  # Attempt to start fstrim
  OUT=`adb shell vdc fstrim dotrim | grep "Command not recognized"`

  if [ -z "$OUT" ]
  then
    # Wait till we see another fstrim finished line
    sleep 10
    let T=10
    NEWCNT=`adb shell logcat -d |grep -c "Finished fstrim work"`
    while [ "$NEWCNT" -eq "$ORIGCNT" ]
    do
      sleep 10
      let T=T+10
      if [ "$T" -ge 300 ]
      then
        echo "Error: FSTRIM did not complete in 300 seconds, continuing"
        break
      fi
      NEWCNT=`adb shell logcat -d |grep -c "Finished fstrim work"`
    done

    echo "FSTRIM took "$T" seconds"

    # Run the test again
    echo "Running test after fstrim"
    adb shell /dev/$PERF -w -o -s 20000 -f /dev/fstrimmed_stats 400 "$CACHE"

    # Retrieve the full data logs
    adb pull /dev/fstrimmed_stats $HARDWARE-fstrimmed_stats
    adb pull /dev/full_stats $HARDWARE-full_stats
  else
    echo "Device doesn't support fstrim, not running test a second time"
  fi

else

  # Sequential read test
  if [ "$USERBUILD_MODE" -eq 0 ]
  then
    # There is no point in running this in USERBUILD mode, because
    # we can't drop caches, and the numbers are ludicrously high
    for I in 1 2 3
    do
      adb shell "echo 3 > /proc/sys/vm/drop_caches"
      echo "Sequential read test $I"
      adb shell dd if="$CACHE" of=/dev/null bs=1048576 count=200
    done
  fi

  # Sequential write test
  for I in 1 2 3
  do
    echo "Sequential write test $I"
    # It's unclear if this test is useful on USERBUILDS, given the
    # caching on the filesystem
    adb shell dd if=/dev/zero conv=notrunc of="$CACHE" bs=1048576 count=200
  done

  if [ "$USERBUILD_MODE" -eq 0 ]
  then
    # Random read tests require that we read from a much larger range of offsets
    # into the emmc chip than the write test.  If we only read though 100 Megabytes
    # (and with a read-ahead of 128K), we quickly fill the buffer cache with 100
    # Megabytes of data, and subsequent reads are nearly instantaneous.  Since
    # reading is non-destructive, and we've never shipped a device with less than
    # 8 Gbytes, for this test we read from the raw emmc device, and randomly seek
    # in the first 6 Gbytes.  That is way more memory than any device we currently
    # have and it should keep the cache from being poluted with entries from
    # previous random reads.
    #
    # Also, test with the read-ahead set very low at 4K, and at the default

    # Random read test, 4K read-ahead
    ORIG_READAHEAD=`adb shell cat /sys/block/$MMCDEV/queue/read_ahead_kb | tr -d "\r"`
    adb shell "echo 4 > /sys/block/$MMCDEV/queue/read_ahead_kb"
    for I in 1 2 3
    do
      adb shell "echo 3 > /proc/sys/vm/drop_caches"
      echo "Random read (4K read-ahead) test $I"
      adb shell "$PERF_LOC"/"$PERF" -r 6000 "/dev/block/$MMCDEV"
    done

    # Random read test, default read-ahead
    adb shell "echo $ORIG_READAHEAD > /sys/block/$MMCDEV/queue/read_ahead_kb"
    for I in 1 2 3
    do
      adb shell "echo 3 > /proc/sys/vm/drop_caches"
      echo "Random read (default read-ahead of ${ORIG_READAHEAD}K) test $I"
      adb shell "$PERF_LOC"/"$PERF" -r 6000 "/dev/block/$MMCDEV"
    done
  fi

  # Random write test
  for I in 1 2 3
  do
    echo "Random write test $I"
    adb shell "$PERF_LOC"/"$PERF" -w 100 "$CACHE"
  done

  # Random write test with O_SYNC
  for I in 1 2 3
  do
    echo "Random write with o_sync test $I"
    adb shell "$PERF_LOC"/"$PERF" -w -o 100 "$CACHE"
  done
fi

# cleanup
if [ "$USERBUILD_MODE" -eq 0 ]
then
  # Make a new empty /cache filesystem
  adb shell make_ext4fs -w "$CACHE"
else
  adb shell rm -f "$CACHE" "$PERF_LOC"/"$PERF"
fi

