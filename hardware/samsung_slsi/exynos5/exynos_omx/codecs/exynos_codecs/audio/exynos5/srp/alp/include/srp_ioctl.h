/*
 *
 * Copyright 2012 Samsung Electronics S.LSI Co. LTD
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * @file        srp_ioctl.h
 * @brief
 * @author      Yunji Kim (yunji.kim@samsung.com)
 * @version     1.1.0
 * @history
 *   2012.02.28 : Create
 */

#ifndef __SRP_IOCTL_H__
#define __SRP_IOCTL_H__

#ifdef __cplusplus
extern "C" {
#endif

#define SRP_INIT                             (0x10000)
#define SRP_DEINIT                           (0x10001)
#define SRP_GET_MMAP_SIZE                    (0x10002)
#define SRP_FLUSH                            (0x20002)
#define SRP_SEND_EOS                         (0x20005)
#define SRP_GET_IBUF_INFO                    (0x20007)
#define SRP_GET_OBUF_INFO                    (0x20008)
#define SRP_STOP_EOS_STATE                   (0x30007)
#define SRP_GET_DEC_INFO                     (0x30008)

#ifdef __cplusplus
}
#endif

#endif /* __SRP_IOCTL_H__ */

