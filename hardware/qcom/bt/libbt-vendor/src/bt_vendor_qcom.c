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
 *  Filename:      bt_vendor_brcm.c
 *
 *  Description:   Broadcom vendor specific library implementation
 *
 ******************************************************************************/

#define LOG_TAG "bt_vendor"

#include <utils/Log.h>
#include <fcntl.h>
#include <termios.h>
#include "bt_vendor_qcom.h"
#include "userial_vendor.h"

/******************************************************************************
**  Externs
******************************************************************************/
extern int hw_config(int nState);

extern int is_hw_ready();

/******************************************************************************
**  Variables
******************************************************************************/
int pFd[2] = {0,};
bt_hci_transport_device_type bt_hci_transport_device;

bt_vendor_callbacks_t *bt_vendor_cbacks = NULL;
uint8_t vnd_local_bd_addr[6]={0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

#if (HW_NEED_END_WITH_HCI_RESET == TRUE)
void hw_epilog_process(void);
#endif


/******************************************************************************
**  Local type definitions
******************************************************************************/


/******************************************************************************
**  Functions
******************************************************************************/

/*****************************************************************************
**
**   BLUETOOTH VENDOR INTERFACE LIBRARY FUNCTIONS
**
*****************************************************************************/

static int init(const bt_vendor_callbacks_t* p_cb, unsigned char *local_bdaddr)
{
    ALOGI("bt-vendor : init");

    if (p_cb == NULL)
    {
        ALOGE("init failed with no user callbacks!");
        return -1;
    }

    //userial_vendor_init();
    //upio_init();

    //vnd_load_conf(VENDOR_LIB_CONF_FILE);

    /* store reference to user callbacks */
    bt_vendor_cbacks = (bt_vendor_callbacks_t *) p_cb;

    /* This is handed over from the stack */
    memcpy(vnd_local_bd_addr, local_bdaddr, 6);

    return 0;
}


/** Requested operations */
static int op(bt_vendor_opcode_t opcode, void *param)
{
    int retval = 0;
    int nCnt = 0;
    int nState = -1;

    ALOGV("bt-vendor : op for %d", opcode);

    switch(opcode)
    {
        case BT_VND_OP_POWER_CTRL:
            {
                nState = *(int *) param;
                retval = hw_config(nState);
                if(nState == BT_VND_PWR_ON
                   && retval == 0
                   && is_hw_ready() == TRUE){
                    retval = 0;
                }
                else {
                    retval = -1;
                }
            }
            break;

        case BT_VND_OP_FW_CFG:
            {
                // call hciattach to initalize the stack
                if(bt_vendor_cbacks){
                   ALOGI("Bluetooth Firmware and smd is initialized");
                   bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_SUCCESS);
                }
                else{
                   ALOGE("Error : hci, smd initialization Error");
                   bt_vendor_cbacks->fwcfg_cb(BT_VND_OP_RESULT_FAIL);
                }
            }
            break;

        case BT_VND_OP_SCO_CFG:
            {
                bt_vendor_cbacks->scocfg_cb(BT_VND_OP_RESULT_SUCCESS); //dummy
            }
            break;

        case BT_VND_OP_USERIAL_OPEN:
            {
                if(bt_hci_init_transport(pFd) != -1){
                    int (*fd_array)[] = (int (*) []) param;

                        (*fd_array)[CH_CMD] = pFd[0];
                        (*fd_array)[CH_EVT] = pFd[0];
                        (*fd_array)[CH_ACL_OUT] = pFd[1];
                        (*fd_array)[CH_ACL_IN] = pFd[1];
                }
                else {
                    retval = -1;
                    break;
                }
                retval = 2;
            }
            break;

        case BT_VND_OP_USERIAL_CLOSE:
            {
                 bt_hci_deinit_transport(pFd);
            }
            break;

        case BT_VND_OP_GET_LPM_IDLE_TIMEOUT:
            break;

        case BT_VND_OP_LPM_SET_MODE:
            {
                bt_vendor_cbacks->lpm_cb(BT_VND_OP_RESULT_SUCCESS); //dummy
            }
            break;

        case BT_VND_OP_LPM_WAKE_SET_STATE:
            break;
        case BT_VND_OP_EPILOG:
            {
#if (HW_NEED_END_WITH_HCI_RESET == FALSE)
                if (bt_vendor_cbacks)
                {
                    bt_vendor_cbacks->epilog_cb(BT_VND_OP_RESULT_SUCCESS);
                }
#else
                hw_epilog_process();
#endif
            }
            break;
    }

    return retval;
}

/** Closes the interface */
static void cleanup( void )
{
    ALOGI("cleanup");

    //upio_cleanup();

    bt_vendor_cbacks = NULL;
}

// Entry point of DLib
const bt_vendor_interface_t BLUETOOTH_VENDOR_LIB_INTERFACE = {
    sizeof(bt_vendor_interface_t),
    init,
    op,
    cleanup
};
