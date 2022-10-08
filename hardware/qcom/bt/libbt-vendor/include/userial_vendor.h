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
 *  Filename:      userial_vendor.h
 *
 *  Description:   Contains vendor-specific definitions used in serial port
 *                 controls
 *
 ******************************************************************************/

#ifndef USERIAL_VENDOR_H
#define USERIAL_VENDOR_H

/* Variables to identify the platform */
/*BT HS UART TTY DEVICE */
#define BT_HS_UART_DEVICE "/dev/ttyHS0"
/*BT RIVA-SMD CHANNELS */
#define APPS_RIVA_BT_ACL_CH  "/dev/smd2"
#define APPS_RIVA_BT_CMD_CH  "/dev/smd3"

typedef enum
{
  BT_HCI_UART,
  BT_HCI_SMD,
  BT_HCI_NONE = 0xFF
} bt_hci_transport_enum_type;

typedef struct
{
  /*transport type can be SMD/UART*/
  bt_hci_transport_enum_type type;

  /*hci cmd/event packet is required to carry the Packet indicator for UART interfaces only
    and not required for smd */
  int pkt_ind;

  /*transport device can be UART/SMD*/
  char *name;

} bt_hci_transport_device_type;

bt_hci_transport_device_type bt_hci_set_transport();

int bt_hci_init_transport ( int *pFd );

int bt_hci_deinit_transport(int *pFd);

#endif /* USERIAL_VENDOR_H */

