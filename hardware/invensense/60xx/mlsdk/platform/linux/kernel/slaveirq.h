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

#ifndef __SLAVEIRQ__
#define __SLAVEIRQ__

#include <linux/mpu.h>
#include "mpuirq.h"

#define SLAVEIRQ_SET_TIMEOUT           _IOW(MPU_IOCTL, 0x50, unsigned long)
#define SLAVEIRQ_GET_INTERRUPT_CNT     _IOR(MPU_IOCTL, 0x51, unsigned long)
#define SLAVEIRQ_GET_IRQ_TIME          _IOR(MPU_IOCTL, 0x52, unsigned long)


void slaveirq_exit(struct ext_slave_platform_data *pdata);
int slaveirq_init(struct i2c_adapter *slave_adapter,
		  struct ext_slave_platform_data *pdata, char *name);


#endif
