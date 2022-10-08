/*
 * Copyright 2012 The Android Open Source Project
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

/******************************************************************************
 *
 *  Filename:      userial_vendor.c
 *
 *  Description:   Contains vendor-specific userial functions
 *
 ******************************************************************************/

#define LOG_TAG "bt_userial_vendor"

#include <utils/Log.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include "bt_vendor_qcom.h"
#include "userial_vendor.h"

bt_hci_transport_device_type bt_hci_set_transport()
{
    int ret;
    char transport_type[10] = {0,};
    bt_hci_transport_device_type bt_hci_transport_device;

    ret = property_get("ro.qualcomm.bt.hci_transport", transport_type, NULL);
    if(ret == 0)
        printf("ro.qualcomm.bt.hci_transport not set\n");
    else
        printf("ro.qualcomm.bt.hci_transport: %s \n", transport_type);

    if (!strcasecmp(transport_type, "smd"))
    {
        bt_hci_transport_device.type = BT_HCI_SMD;
        bt_hci_transport_device.name = APPS_RIVA_BT_CMD_CH;
        bt_hci_transport_device.pkt_ind = 1;
    }
    else{
        bt_hci_transport_device.type = BT_HCI_UART;
        bt_hci_transport_device.name = BT_HS_UART_DEVICE;
        bt_hci_transport_device.pkt_ind = 0;
    }

    return bt_hci_transport_device;
}

#define NUM_OF_DEVS 2
static char *s_pszDevSmd[] = {
    "/dev/smd3",
    "/dev/smd2"
};

int bt_hci_init_transport(int *pFd)
{
    int i = 0;
    int fd;
    for(i=0; i < NUM_OF_DEVS; i++){
       fd = bt_hci_init_transport_id(i);
       if(fd < 0 ){
          return -1;
       }
       pFd[i] = fd;
    }
    return 0;
}

int bt_hci_init_transport_id (int chId )
{
  struct termios   term;
  int fd = -1;
  int retry = 0;

  if(chId > 2 || chId <0)
     return -1;

  fd = open(s_pszDevSmd[chId], (O_RDWR | O_NOCTTY));

  while ((-1 == fd) && (retry < 7)) {
    ALOGE("init_transport: Cannot open %s: %s\n. Retry after 2 seconds",
        bt_hci_transport_device.name, strerror(errno));
    usleep(2000000);
    fd = open(bt_hci_transport_device.name, (O_RDWR | O_NOCTTY));
    retry++;
  }

  if (-1 == fd)
  {
    ALOGE("init_transport: Cannot open %s: %s\n",
        bt_hci_transport_device.name, strerror(errno));
    return -1;
  }

  /* Sleep (0.5sec) added giving time for the smd port to be successfully
     opened internally. Currently successful return from open doesn't
     ensure the smd port is successfully opened.
     TODO: Following sleep to be removed once SMD port is successfully
     opened immediately on return from the aforementioned open call */
  if(BT_HCI_SMD == bt_hci_transport_device.type)
     usleep(500000);

  if (tcflush(fd, TCIOFLUSH) < 0)
  {
    ALOGE("init_uart: Cannot flush %s\n", bt_hci_transport_device.name);
    close(fd);
    return -1;
  }

  if (tcgetattr(fd, &term) < 0)
  {
    ALOGE("init_uart: Error while getting attributes\n");
    close(fd);
    return -1;
  }

  cfmakeraw(&term);

  /* JN: Do I need to make flow control configurable, since 4020 cannot
   * disable it?
   */
  term.c_cflag |= (CRTSCTS | CLOCAL);

  if (tcsetattr(fd, TCSANOW, &term) < 0)
  {
    ALOGE("init_uart: Error while getting attributes\n");
    close(fd);
    return -1;
  }

  ALOGI("Done intiailizing UART\n");
  return fd;
}

int bt_hci_deinit_transport(int *pFd)
{
    close(pFd[0]);
    close(pFd[1]);
    return TRUE;
}
