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

#ifndef __TIMERIRQ__
#define __TIMERIRQ__

#include <linux/mpu.h>

#define TIMERIRQ_SET_TIMEOUT           _IOW(MPU_IOCTL, 0x60, unsigned long)
#define TIMERIRQ_GET_INTERRUPT_CNT     _IOW(MPU_IOCTL, 0x61, unsigned long)
#define TIMERIRQ_START                 _IOW(MPU_IOCTL, 0x62, unsigned long)
#define TIMERIRQ_STOP                  _IO(MPU_IOCTL, 0x63)

#endif
