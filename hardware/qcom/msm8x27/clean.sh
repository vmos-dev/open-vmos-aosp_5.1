#!/bin/bash

CLEAN_HEADER=../../../bionic/libc/kernel/tools/clean_header.py
ORIGINAL=original-kernel-headers
NEW=kernel-headers
OPTS=-k$ORIGINAL

HEADERS="linux/msm_ion.h linux/msm_mdp.h  linux/msm_rotator.h linux/msm_kgsl.h"

for HEADER in $HEADERS; do
    echo $HEADER
    $CLEAN_HEADER $OPTS $ORIGINAL/$HEADER > $NEW/$HEADER
done
