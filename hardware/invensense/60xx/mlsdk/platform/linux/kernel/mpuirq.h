/*
 $License:
   Copyright 2011 InvenSense, Inc.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
  $
 */

#ifndef __MPUIRQ__
#define __MPUIRQ__

#ifdef __KERNEL__
#include <linux/time.h>
#include <linux/ioctl.h>
#include "mldl_cfg.h"
#else
#include <sys/ioctl.h>
#include <sys/time.h>
#endif

#define MPUIRQ_SET_TIMEOUT           _IOW(MPU_IOCTL, 0x40, unsigned long)
#define MPUIRQ_GET_INTERRUPT_CNT     _IOR(MPU_IOCTL, 0x41, unsigned long)
#define MPUIRQ_GET_IRQ_TIME          _IOR(MPU_IOCTL, 0x42, struct timeval)
#define MPUIRQ_SET_FREQUENCY_DIVIDER _IOW(MPU_IOCTL, 0x43, unsigned long)

#ifdef __KERNEL__
void mpuirq_exit(void);
int mpuirq_init(struct i2c_client *mpu_client, struct mldl_cfg *mldl_cfg);
#endif

#endif
