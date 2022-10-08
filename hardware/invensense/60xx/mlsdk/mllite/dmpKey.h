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
#ifndef DMPKEY_H__
#define DMPKEY_H__

#define KEY_CFG_25  0
#define KEY_CFG_24                 (KEY_CFG_25+1)
#define KEY_CFG_26                 (KEY_CFG_24+1)
#define KEY_CFG_21                 (KEY_CFG_26+1)
#define KEY_CFG_20                 (KEY_CFG_21+1)
#define KEY_CFG_TAP4               (KEY_CFG_20+1)
#define KEY_CFG_TAP5               (KEY_CFG_TAP4+1)
#define KEY_CFG_TAP6               (KEY_CFG_TAP5+1)
#define KEY_CFG_TAP7               (KEY_CFG_TAP6+1)
#define KEY_CFG_TAP0               (KEY_CFG_TAP7+1)
#define KEY_CFG_TAP1               (KEY_CFG_TAP0+1)
#define KEY_CFG_TAP2               (KEY_CFG_TAP1+1)
#define KEY_CFG_TAP3               (KEY_CFG_TAP2+1)
#define KEY_CFG_TAP_QUANTIZE       (KEY_CFG_TAP3+1)
#define KEY_CFG_TAP_JERK           (KEY_CFG_TAP_QUANTIZE+1)
#define KEY_CFG_TAP_SAVE_ACCB      (KEY_CFG_TAP_JERK+1)
#define KEY_CFG_TAP_CLEAR_STICKY   (KEY_CFG_TAP_SAVE_ACCB+1)
#define KEY_FCFG_ACCEL_INPUT       (KEY_CFG_TAP_CLEAR_STICKY +1)
#define KEY_FCFG_ACCEL_INIT        (KEY_FCFG_ACCEL_INPUT+1)
#define KEY_CFG_23                 (KEY_FCFG_ACCEL_INIT+1)
#define KEY_FCFG_1                 (KEY_CFG_23+1)
#define KEY_FCFG_3                 (KEY_FCFG_1+1)
#define KEY_FCFG_2                 (KEY_FCFG_3+1)
#define KEY_CFG_3D                 (KEY_FCFG_2+1)
#define KEY_CFG_3B                 (KEY_CFG_3D+1)
#define KEY_CFG_3C                 (KEY_CFG_3B+1)
#define KEY_FCFG_5                 (KEY_CFG_3C+1)
#define KEY_FCFG_4                 (KEY_FCFG_5+1)
#define KEY_FCFG_7                 (KEY_FCFG_4+1)
#define KEY_FCFG_FSCALE            (KEY_FCFG_7+1)
#define KEY_FCFG_AZ                (KEY_FCFG_FSCALE+1)
#define KEY_FCFG_6                 (KEY_FCFG_AZ+1)
#define KEY_FCFG_LSB4              (KEY_FCFG_6+1)
#define KEY_CFG_12                 (KEY_FCFG_LSB4+1)
#define KEY_CFG_14                 (KEY_CFG_12+1)
#define KEY_CFG_15                 (KEY_CFG_14+1)
#define KEY_CFG_16                 (KEY_CFG_15+1)
#define KEY_CFG_18                 (KEY_CFG_16+1)
#define KEY_CFG_6                  (KEY_CFG_18 + 1)
#define KEY_CFG_7                  (KEY_CFG_6+1)
#define KEY_CFG_4                  (KEY_CFG_7+1)
#define KEY_CFG_5                  (KEY_CFG_4+1)
#define KEY_CFG_2                  (KEY_CFG_5+1)
#define KEY_CFG_3                  (KEY_CFG_2+1)
#define KEY_CFG_1                  (KEY_CFG_3+1)
#define KEY_CFG_EXTERNAL           (KEY_CFG_1+1)
#define KEY_CFG_8                  (KEY_CFG_EXTERNAL+1)
#define KEY_CFG_9                  (KEY_CFG_8+1)
#define KEY_CFG_ORIENT_3           (KEY_CFG_9 + 1)
#define KEY_CFG_ORIENT_2           (KEY_CFG_ORIENT_3 + 1)
#define KEY_CFG_ORIENT_1           (KEY_CFG_ORIENT_2 + 1)
#define KEY_CFG_GYRO_SOURCE        (KEY_CFG_ORIENT_1 + 1)
#define KEY_CFG_ORIENT_IRQ_1       (KEY_CFG_GYRO_SOURCE + 1)
#define KEY_CFG_ORIENT_IRQ_2       (KEY_CFG_ORIENT_IRQ_1 + 1)
#define KEY_CFG_ORIENT_IRQ_3       (KEY_CFG_ORIENT_IRQ_2 + 1)
#define KEY_FCFG_MAG_VAL           (KEY_CFG_ORIENT_IRQ_3 + 1)
#define KEY_FCFG_MAG_MOV           (KEY_FCFG_MAG_VAL + 1)

#define KEY_D_0_22                 (KEY_FCFG_MAG_MOV + 1)
#define KEY_D_0_24                 (KEY_D_0_22+1)
#define KEY_D_0_36                 (KEY_D_0_24+1)
#define KEY_D_0_52                 (KEY_D_0_36+1)
#define KEY_D_0_96                 (KEY_D_0_52+1)
#define KEY_D_0_104                (KEY_D_0_96+1)
#define KEY_D_0_108                (KEY_D_0_104+1)
#define KEY_D_0_163                (KEY_D_0_108+1)
#define KEY_D_0_188                (KEY_D_0_163+1)
#define KEY_D_0_192                (KEY_D_0_188+1)
#define KEY_D_0_224                (KEY_D_0_192+1)
#define KEY_D_0_228                (KEY_D_0_224+1)
#define KEY_D_0_232                (KEY_D_0_228+1)
#define KEY_D_0_236                (KEY_D_0_232+1)

#define KEY_DMP_PREVPTAT           (KEY_D_0_236+1)
#define KEY_D_1_2                  (KEY_DMP_PREVPTAT+1)
#define KEY_D_1_4                  (KEY_D_1_2 + 1)
#define KEY_D_1_8                  (KEY_D_1_4 + 1)
#define KEY_D_1_10                 (KEY_D_1_8+1)
#define KEY_D_1_24                 (KEY_D_1_10+1)
#define KEY_D_1_28                 (KEY_D_1_24+1)
#define KEY_D_1_92                 (KEY_D_1_28+1)
#define KEY_D_1_96                 (KEY_D_1_92+1)
#define KEY_D_1_98                 (KEY_D_1_96+1)
#define KEY_D_1_106                (KEY_D_1_98+1)
#define KEY_D_1_108                (KEY_D_1_106+1)
#define KEY_D_1_112                (KEY_D_1_108+1)
#define KEY_D_1_128                (KEY_D_1_112+1)
#define KEY_D_1_152                (KEY_D_1_128+1)
#define KEY_D_1_168                (KEY_D_1_152+1)
#define KEY_D_1_175                (KEY_D_1_168+1)
#define KEY_D_1_178                (KEY_D_1_175+1)
#define KEY_D_1_179                (KEY_D_1_178+1)
#define KEY_D_1_236                (KEY_D_1_179+1)
#define KEY_D_1_244                (KEY_D_1_236+1)
#define KEY_D_2_12                 (KEY_D_1_244+1)
#define KEY_D_2_96                 (KEY_D_2_12+1)
#define KEY_D_2_108                (KEY_D_2_96+1)
#define KEY_D_2_244                (KEY_D_2_108+1)
#define KEY_D_2_248                (KEY_D_2_244+1)
#define KEY_D_2_252                (KEY_D_2_248+1)

// Compass Keys
#define KEY_CPASS_BIAS_X            (KEY_D_2_252+1)
#define KEY_CPASS_BIAS_Y            (KEY_CPASS_BIAS_X+1)
#define KEY_CPASS_BIAS_Z            (KEY_CPASS_BIAS_Y+1)
#define KEY_CPASS_MTX_00            (KEY_CPASS_BIAS_Z+1)
#define KEY_CPASS_MTX_01            (KEY_CPASS_MTX_00+1)
#define KEY_CPASS_MTX_02            (KEY_CPASS_MTX_01+1)
#define KEY_CPASS_MTX_10            (KEY_CPASS_MTX_02+1)
#define KEY_CPASS_MTX_11            (KEY_CPASS_MTX_10+1)
#define KEY_CPASS_MTX_12            (KEY_CPASS_MTX_11+1)
#define KEY_CPASS_MTX_20            (KEY_CPASS_MTX_12+1)
#define KEY_CPASS_MTX_21            (KEY_CPASS_MTX_20+1)
#define KEY_CPASS_MTX_22            (KEY_CPASS_MTX_21+1)

// Mantis Keys
#define KEY_CFG_MOTION_BIAS         (KEY_CPASS_MTX_22+1)

#define KEY_DMP_TAPW_MIN           (KEY_CFG_MOTION_BIAS+1)
#define KEY_DMP_TAP_THR_X          (KEY_DMP_TAPW_MIN+1)
#define KEY_DMP_TAP_THR_Y          (KEY_DMP_TAP_THR_X+1)
#define KEY_DMP_TAP_THR_Z          (KEY_DMP_TAP_THR_Y+1)
#define KEY_DMP_SH_TH_Y            (KEY_DMP_TAP_THR_Z+1)
#define KEY_DMP_SH_TH_X            (KEY_DMP_SH_TH_Y+1)
#define KEY_DMP_SH_TH_Z            (KEY_DMP_SH_TH_X+1)
#define KEY_DMP_ORIENT             (KEY_DMP_SH_TH_Z+1)
#define KEY_D_ACT0                 (KEY_DMP_ORIENT+1)
#define KEY_D_ACSX                 (KEY_D_ACT0+1)
#define KEY_D_ACSY                 (KEY_D_ACSX+1)
#define KEY_D_ACSZ                 (KEY_D_ACSY+1)

// Pedometer Standalone only keys
#define KEY_D_PEDSTD_BP_B          (KEY_D_ACSZ+1)
#define KEY_D_PEDSTD_HP_A          (KEY_D_PEDSTD_BP_B+1)
#define KEY_D_PEDSTD_HP_B          (KEY_D_PEDSTD_HP_A+1)
#define KEY_D_PEDSTD_BP_A4         (KEY_D_PEDSTD_HP_B+1)
#define KEY_D_PEDSTD_BP_A3         (KEY_D_PEDSTD_BP_A4+1)
#define KEY_D_PEDSTD_BP_A2         (KEY_D_PEDSTD_BP_A3+1)
#define KEY_D_PEDSTD_BP_A1         (KEY_D_PEDSTD_BP_A2+1)
#define KEY_D_PEDSTD_INT_THRSH     (KEY_D_PEDSTD_BP_A1+1)
#define KEY_D_PEDSTD_CLIP          (KEY_D_PEDSTD_INT_THRSH+1)
#define KEY_D_PEDSTD_SB            (KEY_D_PEDSTD_CLIP+1)
#define KEY_D_PEDSTD_SB_TIME       (KEY_D_PEDSTD_SB+1)
#define KEY_D_PEDSTD_PEAKTHRSH     (KEY_D_PEDSTD_SB_TIME+1)
#define KEY_D_PEDSTD_TIML          (KEY_D_PEDSTD_PEAKTHRSH+1)
#define KEY_D_PEDSTD_TIMH          (KEY_D_PEDSTD_TIML+1)
#define KEY_D_PEDSTD_PEAK          (KEY_D_PEDSTD_TIMH+1)
#define KEY_D_PEDSTD_TIMECTR       (KEY_D_PEDSTD_PEAK+1)
#define KEY_D_PEDSTD_STEPCTR       (KEY_D_PEDSTD_TIMECTR+1)
#define KEY_D_PEDSTD_WALKTIME      (KEY_D_PEDSTD_STEPCTR+1)

// EIS Keys
#define KEY_P_EIS_FIFO_FOOTER      (KEY_D_PEDSTD_WALKTIME+1)
#define KEY_P_EIS_FIFO_YSHIFT      (KEY_P_EIS_FIFO_FOOTER+1)
#define KEY_P_EIS_DATA_RATE        (KEY_P_EIS_FIFO_YSHIFT+1)
#define KEY_P_EIS_FIFO_XSHIFT      (KEY_P_EIS_DATA_RATE+1)
#define KEY_P_EIS_FIFO_SYNC        (KEY_P_EIS_FIFO_XSHIFT+1)
#define KEY_P_EIS_FIFO_ZSHIFT      (KEY_P_EIS_FIFO_SYNC+1)
#define KEY_P_EIS_FIFO_READY       (KEY_P_EIS_FIFO_ZSHIFT+1)
#define KEY_DMP_FOOTER             (KEY_P_EIS_FIFO_READY+1)
#define KEY_DMP_INTX_HC            (KEY_DMP_FOOTER+1)
#define KEY_DMP_INTX_PH            (KEY_DMP_INTX_HC+1)
#define KEY_DMP_INTX_SH            (KEY_DMP_INTX_PH+1)
#define KEY_DMP_AINV_SH            (KEY_DMP_INTX_SH +1)
#define KEY_DMP_A_INV_XH           (KEY_DMP_AINV_SH+1)
#define KEY_DMP_AINV_PH            (KEY_DMP_A_INV_XH+1)
#define KEY_DMP_CTHX_H             (KEY_DMP_AINV_PH+1)
#define KEY_DMP_CTHY_H             (KEY_DMP_CTHX_H+1)
#define KEY_DMP_CTHZ_H             (KEY_DMP_CTHY_H+1)
#define KEY_DMP_NCTHX_H            (KEY_DMP_CTHZ_H+1)
#define KEY_DMP_NCTHY_H            (KEY_DMP_NCTHX_H+1)
#define KEY_DMP_NCTHZ_H            (KEY_DMP_NCTHY_H+1)
#define KEY_DMP_CTSQ_XH            (KEY_DMP_NCTHZ_H+1)
#define KEY_DMP_CTSQ_YH            (KEY_DMP_CTSQ_XH+1)
#define KEY_DMP_CTSQ_ZH            (KEY_DMP_CTSQ_YH+1)
#define KEY_DMP_INTX_H             (KEY_DMP_CTSQ_ZH+1)
#define KEY_DMP_INTY_H             (KEY_DMP_INTX_H+1)
#define KEY_DMP_INTZ_H             (KEY_DMP_INTY_H+1)
#define KEY_DMP_HPX_H              (KEY_DMP_INTZ_H+1)
#define KEY_DMP_HPY_H              (KEY_DMP_HPX_H+1)
#define KEY_DMP_HPZ_H              (KEY_DMP_HPY_H+1)

// Stream Keys
#define KEY_STREAM_P_GYRO_Z        (KEY_DMP_HPZ_H + 1)
#define KEY_STREAM_P_GYRO_Y        (KEY_STREAM_P_GYRO_Z+1)
#define KEY_STREAM_P_GYRO_X        (KEY_STREAM_P_GYRO_Y+1)
#define KEY_STREAM_P_TEMP          (KEY_STREAM_P_GYRO_X+1)
#define KEY_STREAM_P_AUX_Y         (KEY_STREAM_P_TEMP+1)
#define KEY_STREAM_P_AUX_X         (KEY_STREAM_P_AUX_Y+1)
#define KEY_STREAM_P_AUX_Z         (KEY_STREAM_P_AUX_X+1)
#define KEY_STREAM_P_ACCEL_Y       (KEY_STREAM_P_AUX_Z+1)
#define KEY_STREAM_P_ACCEL_X       (KEY_STREAM_P_ACCEL_Y+1)
#define KEY_STREAM_P_FOOTER        (KEY_STREAM_P_ACCEL_X+1)
#define KEY_STREAM_P_ACCEL_Z       (KEY_STREAM_P_FOOTER+1)

#define NUM_KEYS (KEY_STREAM_P_ACCEL_Z+1)

    typedef struct {
        unsigned short key;
        unsigned short addr;
    } tKeyLabel;
    
#define DINA0A 0x0a
#define DINA22 0x22
#define DINA42 0x42
#define DINA5A 0x5a

#define DINA06 0x06
#define DINA0E 0x0e
#define DINA16 0x16
#define DINA1E 0x1e
#define DINA26 0x26
#define DINA2E 0x2e
#define DINA36 0x36
#define DINA3E 0x3e
#define DINA46 0x46
#define DINA4E 0x4e
#define DINA56 0x56
#define DINA5E 0x5e
#define DINA66 0x66
#define DINA6E 0x6e
#define DINA76 0x76
#define DINA7E 0x7e

#define DINA00 0x00
#define DINA08 0x08
#define DINA10 0x10
#define DINA18 0x18
#define DINA20 0x20
#define DINA28 0x28
#define DINA30 0x30
#define DINA38 0x38
#define DINA40 0x40
#define DINA48 0x48
#define DINA50 0x50
#define DINA58 0x58
#define DINA60 0x60
#define DINA68 0x68
#define DINA70 0x70
#define DINA78 0x78

#define DINA04 0x04
#define DINA0C 0x0c
#define DINA14 0x14
#define DINA1C 0x1C
#define DINA24 0x24
#define DINA2C 0x2c
#define DINA34 0x34
#define DINA3C 0x3c
#define DINA44 0x44
#define DINA4C 0x4c
#define DINA54 0x54
#define DINA5C 0x5c
#define DINA64 0x64
#define DINA6C 0x6c
#define DINA74 0x74
#define DINA7C 0x7c

#define DINA01 0x01
#define DINA09 0x09
#define DINA11 0x11
#define DINA19 0x19
#define DINA21 0x21
#define DINA29 0x29
#define DINA31 0x31
#define DINA39 0x39
#define DINA41 0x41
#define DINA49 0x49
#define DINA51 0x51
#define DINA59 0x59
#define DINA61 0x61
#define DINA69 0x69
#define DINA71 0x71
#define DINA79 0x79

#define DINA25 0x25
#define DINA2D 0x2d
#define DINA35 0x35
#define DINA3D 0x3d
#define DINA4D 0x4d
#define DINA55 0x55
#define DINA5D 0x5D
#define DINA6D 0x6d
#define DINA75 0x75
#define DINA7D 0x7d

#define DINC00 0x00
#define DINC01 0x01
#define DINC02 0x02
#define DINC03 0x03
#define DINC08 0x08
#define DINC09 0x09
#define DINC0A 0x0a
#define DINC0B 0x0b
#define DINC10 0x10
#define DINC11 0x11
#define DINC12 0x12
#define DINC13 0x13
#define DINC18 0x18
#define DINC19 0x19
#define DINC1A 0x1a
#define DINC1B 0x1b

#define DINC20 0x20
#define DINC21 0x21
#define DINC22 0x22
#define DINC23 0x23
#define DINC28 0x28
#define DINC29 0x29
#define DINC2A 0x2a
#define DINC2B 0x2b
#define DINC30 0x30
#define DINC31 0x31
#define DINC32 0x32
#define DINC33 0x33
#define DINC38 0x38
#define DINC39 0x39
#define DINC3A 0x3a
#define DINC3B 0x3b

#define DINC40 0x40
#define DINC41 0x41
#define DINC42 0x42
#define DINC43 0x43
#define DINC48 0x48
#define DINC49 0x49
#define DINC4A 0x4a
#define DINC4B 0x4b
#define DINC50 0x50
#define DINC51 0x51
#define DINC52 0x52
#define DINC53 0x53
#define DINC58 0x58
#define DINC59 0x59
#define DINC5A 0x5a
#define DINC5B 0x5b

#define DINC60 0x60
#define DINC61 0x61
#define DINC62 0x62
#define DINC63 0x63
#define DINC68 0x68
#define DINC69 0x69
#define DINC6A 0x6a
#define DINC6B 0x6b
#define DINC70 0x70
#define DINC71 0x71
#define DINC72 0x72
#define DINC73 0x73
#define DINC78 0x78
#define DINC79 0x79
#define DINC7A 0x7a
#define DINC7B 0x7b

#if defined CONFIG_MPU_SENSORS_MPU3050
#define DINA80 0x80
#define DINA90 0x90
#define DINAA0 0xa0
#define DINAC9 0xc9
#define DINACB 0xcb
#define DINACD 0xcd
#define DINACF 0xcf
#define DINAC8 0xc8
#define DINACA 0xca
#define DINACC 0xcc
#define DINACE 0xce
#define DINAD8 0xd8
#define DINADD 0xdd
#define DINAF8 0xf8
#define DINAFE 0xfe
#define DINAC0 0xc0
#define DINAC1 0xc1
#define DINAC2 0xc2
#define DINAC3 0xc3
#define DINAC4 0xc4
#define DINAC5 0xc5
#elif defined CONFIG_MPU_SENSORS_MPU6050A2 || \
	defined CONFIG_MPU_SENSORS_MPU6050B1

#define DINA80 0x80
#define DINA90 0x90
#define DINAA0 0xa0
#define DINAC9 0xc9
#define DINACB 0xcb
#define DINACD 0xcd
#define DINACF 0xcf
#define DINAC8 0xc8
#define DINACA 0xca
#define DINACC 0xcc
#define DINACE 0xce
#define DINAD8 0xd8
#define DINADD 0xdd
#define DINAF8 0xf0
#define DINAFE 0xfe

#define DINBF8 0xf8
#define DINAC0 0xb0
#define DINAC1 0xb1
#define DINAC2 0xb4
#define DINAC3 0xb5
#define DINAC4 0xb8
#define DINAC5 0xb9
#define DINBC0 0xc0
#define DINBC2 0xc2
#define DINBC4 0xc4
#define DINBC6 0xc6
#else
#error No CONFIG_MPU_SENSORS_xxxx has been defined.
#endif


#endif // DMPKEY_H__
