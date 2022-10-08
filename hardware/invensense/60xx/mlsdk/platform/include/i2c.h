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
/*******************************************************************************
 *
 * $Id: i2c.h 5629 2011-06-11 03:13:08Z mcaramello $
 * 
 *******************************************************************************/

#ifndef _I2C_H
#define _I2C_H

#include "mltypes.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------ */
/* - Defines. - */
/* ------------ */

/* - Error Codes. - */

#define I2C_SUCCESS 0
#define I2C_ERROR   1

/* ---------- */
/* - Enums. - */
/* ---------- */

/* --------------- */
/* - Structures. - */
/* --------------- */

#define I2C_RBUFF_MAX    128
#define I2C_RBUFF_SIZE    17

#ifdef BB_I2C
#define I2C_RBUFF_DYNAMIC  0
#else
#define I2C_RBUFF_DYNAMIC  1
#endif

typedef struct {

	HANDLE  i2cHndl;
	HANDLE  hDevice;                    // handle to the drive to be examined 

    MLU8      readBuffer[1024];
	MLU8      writeBuffer[1024];

    MLU16     rBuffRIndex;
	MLU16     rBuffWIndex;
#if !I2C_RBUFF_DYNAMIC 
    MLU8      rBuff[I2C_RBUFF_MAX][I2C_RBUFF_SIZE];
#else
    MLU8     *rBuff;
#endif
    MLU16     rBuffMax;
	MLU16     rBuffNumBytes;

    MLU8      runThread;
	MLU8      autoProcess;

} I2C_Vars_t;

/* --------------------- */
/* - Function p-types. - */
/* --------------------- */

#if (defined(BB_I2C))
void set_i2c_open_bind_cb(int (*func)(unsigned int i2c_slave_addr));
void set_i2c_open_cb(int (*func)(const char *dev, int rw));
void set_i2c_close_cb(int (*func)(int fd));
void set_i2c_lltransfer_cb(int (*func)(int fd, int client_addr, const char *write_buf, unsigned int write_len,
                                                                      char *read_buf,  unsigned int read_len ));
void set_i2c_write_register_cb(int (*func)(int fd, int client_addr, unsigned char reg, unsigned char value));
void set_i2c_read_register_cb(unsigned char (*func)(int fd, int client_addr, unsigned char reg));
void set_i2c_dump_register_cb(int (*func)(int fd, int client_addr, unsigned char start_reg, unsigned char *buff, int len));

int           _i2c_write_register(int fd, int client_addr, unsigned char reg, unsigned char value);
unsigned char _i2c_read_register (int fd, int client_addr, unsigned char reg);
int            i2c_lltransfer    (int fd, int client_addr, const char *write_buf, unsigned int write_len,
                                                                 char *read_buf,  unsigned int read_len );
int            i2c_write_register(int fd, int client_addr, unsigned char reg, unsigned char value);
unsigned char  i2c_read_register (int fd, int client_addr, unsigned char reg);
int            i2c_dump_register (int fd, int client_addr, unsigned char start_reg, unsigned char *buff, int len);
int i2c_open         (const char *dev, int rw);
int i2c_close        (int fd);
int i2c_open_bind    (unsigned int i2c_slave_addr);
#endif

int I2COpen           (unsigned char autoProcess, unsigned char createThread);
int I2CClose          (void);
int I2CDeviceIoControl(void);
int I2CRead           (void);
int I2CWrite          (void);
int I2CSetBufferSize  (unsigned short bufferSize);
int I2CBufferUpdate   (void);
int I2CHandler        (void);
int I2CReadBuffer     (unsigned short cnt, unsigned char bufferMode, unsigned char *rBuff);
int I2CEmptyBuffer    (void);
int I2CPktsInBuffer   (unsigned short *pktsInBuffer);
int I2CCreateMutex    (void);
int I2CLockMutex      (void);
int I2CUnlockMutex    (void);

int I2CWriteBurst     (unsigned char slaveAddr, unsigned char registerAddr, unsigned short length, unsigned char *data);
int I2CReadBurst      (unsigned char slaveAddr, unsigned char registerAddr, unsigned short length, unsigned char *data);

int I2COpenBB         (void);
int I2CCloseBB        (int i2cHandle);

#ifdef __cplusplus
}
#endif

#endif /* _TEMPLATE_H */
