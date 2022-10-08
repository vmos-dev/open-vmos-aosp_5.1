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

/******************************************************************************
 *
 * $Id:$
 *
 *****************************************************************************/

#include "mlSetGyroBias.h"
#include "mlFIFO.h"
#include "ml.h"
#include <string.h>
#include "mldl.h"
#include "mlMathFunc.h"

typedef struct {
    int needToSetBias;
    short currentBias[3];
    int mode;
    int motion;
} tSGB;

tSGB sgb;

/** Records a motion event that may cause a callback when the priority for this
 * feature is met.
 */
void inv_set_motion_state(int motion)
{
    sgb.motion = motion;
}

/** Converts from internal DMP gyro bias registers to external hardware gyro bias by
* applying scaling and transformation.
*/
void inv_convert_bias(const unsigned char *regs, short *bias)
{
    long biasTmp2[3], biasTmp[3], biasPrev[3];
    int i;
    int sf;
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();

    if (mldl_cfg->gyro_sens_trim != 0) {
        sf = 2000 * 131 / mldl_cfg->gyro_sens_trim;
    } else {
        sf = 2000;
    }
    for (i = 0; i < 3; i++) {
        biasTmp2[i] = inv_big8_to_int32(&regs[i * 4]);
    }
    // Rotate bias vector by the transpose of the orientation matrix
    for (i = 0; i < 3; ++i) {
        biasTmp[i] = inv_q30_mult(biasTmp2[0], inv_obj.gyro_orient[i]) +
            inv_q30_mult(biasTmp2[1], inv_obj.gyro_orient[i + 3]) +
            inv_q30_mult(biasTmp2[2], inv_obj.gyro_orient[i + 6]);
    }

    for (i = 0; i < GYRO_NUM_AXES; i++) {
        biasTmp[i] = (long)(biasTmp[i] * 1.39882274201861f / sf);
        biasPrev[i] = (long)mldl_cfg->offset[i];
        if (biasPrev[i] > 32767)
            biasPrev[i] -= 65536L;
    }

    for (i = 0; i < GYRO_NUM_AXES; i++) {
        bias[i] = (short)(biasPrev[i] - biasTmp[i]);
    }
}

/** Records hardware biases in format as used by hardware gyro registers.
* Note, the hardware will add this value to the measured gyro data.
*/
inv_error_t inv_set_gyro_bias_in_hw_unit(const short *bias, int mode)
{
    if (sgb.currentBias[0] != bias[0])
        sgb.needToSetBias = 1;
    if (sgb.currentBias[1] != bias[1])
        sgb.needToSetBias = 1;
    if (sgb.currentBias[2] != bias[2])
        sgb.needToSetBias = 1;
    if (sgb.needToSetBias) {
        memcpy(sgb.currentBias, bias, sizeof(sgb.currentBias));
        sgb.mode = mode;
    }
    return INV_SUCCESS;
}

/** Records gyro biases
* @param[in] bias Bias where 1dps is 2^16. In chip frame.
*/
inv_error_t inv_set_gyro_bias_in_dps(const long *bias, int mode)
{
    struct mldl_cfg *mldl_cfg = inv_get_dl_config();
    int sf, i;
    long biasTmp;
    short offset[3];
    inv_error_t result;

    if (mldl_cfg->gyro_sens_trim != 0) {
        sf = 2000 * 131 / mldl_cfg->gyro_sens_trim;
    } else {
        sf = 2000;
    }

    for (i = 0; i < GYRO_NUM_AXES; i++) {
        biasTmp = -bias[i] / sf;
        if (biasTmp < 0)
            biasTmp += 65536L;
        offset[i] = (short)biasTmp;
    }
    result = inv_set_gyro_bias_in_hw_unit(offset, mode);
    return result;
}

inv_error_t inv_set_gyro_bias_in_dps_float(const float *bias, int mode)
{
    long biasL[3];
    inv_error_t result;

    biasL[0] = (long)(bias[0] * (1L << 16));
    biasL[1] = (long)(bias[1] * (1L << 16));
    biasL[2] = (long)(bias[2] * (1L << 16));
    result = inv_set_gyro_bias_in_dps(biasL, mode);
    return result;
}

inv_error_t MLSetGyroBiasCB(struct inv_obj_t * inv_obj)
{
    inv_error_t result = INV_SUCCESS;
    if (sgb.needToSetBias) {
        result = inv_set_offset(sgb.currentBias);
        sgb.needToSetBias = 0;
    }

    // Check if motion state has changed
    if (sgb.motion == INV_MOTION) {
        // We are moving
        if (inv_obj->motion_state == INV_NO_MOTION) {
            //Trigger motion callback
            inv_obj->motion_state = INV_MOTION;
            inv_obj->flags[INV_MOTION_STATE_CHANGE] = INV_MOTION;
            if (inv_params_obj.motion_cb_func) {
                inv_params_obj.motion_cb_func(INV_MOTION);
            }
        }
    } else if (sgb.motion == INV_NO_MOTION){
        // We are not moving
        if (inv_obj->motion_state == INV_MOTION) {
            //Trigger no motion callback
            inv_obj->motion_state = INV_NO_MOTION;
            inv_obj->got_no_motion_bias = TRUE;
            inv_obj->flags[INV_MOTION_STATE_CHANGE] = INV_NO_MOTION;
            if (inv_params_obj.motion_cb_func) {
                inv_params_obj.motion_cb_func(INV_NO_MOTION);
            }
        }
    }

    return result;
}

inv_error_t inv_enable_set_bias(void)
{
    inv_error_t result;
    memset(&sgb, 0, sizeof(sgb));

    sgb.motion = inv_obj.motion_state;

    result =
        inv_register_fifo_rate_process(MLSetGyroBiasCB,
                                       INV_PRIORITY_SET_GYRO_BIASES);
    if (result == INV_ERROR_INVALID_PARAMETER)
        result = INV_SUCCESS;    /* We already registered this */
    return result;
}

inv_error_t inv_disable_set_bias(void)
{
    inv_error_t result;
    result = inv_unregister_fifo_rate_process(MLSetGyroBiasCB);
    return INV_SUCCESS;          // FIXME need to disable
}
