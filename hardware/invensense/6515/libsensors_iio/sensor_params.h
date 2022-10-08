/*
* Copyright (C) 2014 Invensense, Inc.
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

#ifndef INV_SENSOR_PARAMS_H
#define INV_SENSOR_PARAMS_H

/******************************************/
/******************************************/
//MPU9350 INV_COMPASS
#define COMPASS_MPU9350_RANGE           (9830.f)
#define COMPASS_MPU9350_RESOLUTION      (0.15f)
#define COMPASS_MPU9350_POWER           (10.f)
#define COMPASS_MPU9350_MINDELAY        (1000)
//MPU9250 INV_COMPASS
#define COMPASS_MPU9250_RANGE           (9830.f)
#define COMPASS_MPU9250_RESOLUTION      (0.15f)
#define COMPASS_MPU9250_POWER           (10.f)
#define COMPASS_MPU9250_MINDELAY        (10000)
//MPU9150 INV_COMPASS
#define COMPASS_MPU9150_RANGE           (9830.f)
#define COMPASS_MPU9150_RESOLUTION      (0.285f)
#define COMPASS_MPU9150_POWER           (10.f)
#define COMPASS_MPU9150_MINDELAY        (10000)
//COMPASS_ID_AK8975
#define COMPASS_AKM8975_RANGE           (9830.f)
#define COMPASS_AKM8975_RESOLUTION      (0.285f)
#define COMPASS_AKM8975_POWER           (10.f)
#define COMPASS_AKM8975_MINDELAY        (10000)
//COMPASS_ID_AK8963C
#define COMPASS_AKM8963_RANGE           (9830.f)
#define COMPASS_AKM8963_RESOLUTION      (0.15f)
#define COMPASS_AKM8963_POWER           (10.f)
#define COMPASS_AKM8963_MINDELAY        (10000)
//COMPASS_ID_AK09911
#define COMPASS_AKM9911_RANGE           (9830.f)
#define COMPASS_AKM9911_RESOLUTION      (0.60f)
#define COMPASS_AKM9911_POWER           (10.f)
#define COMPASS_AKM9911_MINDELAY        (10000)
//COMPASS_ID_AMI30X
#define COMPASS_AMI30X_RANGE            (5461.f)
#define COMPASS_AMI30X_RESOLUTION       (0.9f)
#define COMPASS_AMI30X_POWER            (0.15f)
//COMPASS_ID_AMI306
#define COMPASS_AMI306_RANGE            (5461.f)
#define COMPASS_AMI306_RESOLUTION       (0.9f)
#define COMPASS_AMI306_POWER            (0.15f)
#define COMPASS_AMI306_MINDELAY         (10000)
//COMPASS_ID_YAS529
#define COMPASS_YAS529_RANGE            (19660.f)
#define COMPASS_YAS529_RESOLUTION       (0.012f)
#define COMPASS_YAS529_POWER            (4.f)
//COMPASS_ID_YAS53x
#define COMPASS_YAS53x_RANGE            (8001.f)
#define COMPASS_YAS53x_RESOLUTION       (0.012f)
#define COMPASS_YAS53x_POWER            (4.f)
#define COMPASS_YAS53x_MINDELAY         (10000)
//COMPASS_ID_HMC5883
#define COMPASS_HMC5883_RANGE           (10673.f)
#define COMPASS_HMC5883_RESOLUTION      (10.f)
#define COMPASS_HMC5883_POWER           (0.24f)
//COMPASS_ID_LSM303DLH
#define COMPASS_LSM303DLH_RANGE         (10240.f)
#define COMPASS_LSM303DLH_RESOLUTION    (1.f)
#define COMPASS_LSM303DLH_POWER         (1.f)
//COMPASS_ID_LSM303DLM
#define COMPASS_LSM303DLM_RANGE         (10240.f)
#define COMPASS_LSM303DLM_RESOLUTION    (1.f)
#define COMPASS_LSM303DLM_POWER         (1.f)
//COMPASS_ID_MMC314X
#define COMPASS_MMC314X_RANGE           (400.f)
#define COMPASS_MMC314X_RESOLUTION      (2.f)
#define COMPASS_MMC314X_POWER           (0.55f)
//COMPASS_ID_HSCDTD002B
#define COMPASS_HSCDTD002B_RANGE        (9830.f)
#define COMPASS_HSCDTD002B_RESOLUTION   (1.f)
#define COMPASS_HSCDTD002B_POWER        (1.f)
//COMPASS_ID_HSCDTD004A
#define COMPASS_HSCDTD004A_RANGE        (9830.f)
#define COMPASS_HSCDTD004A_RESOLUTION   (1.f)
#define COMPASS_HSCDTD004A_POWER        (1.f)
/*******************************************/
//ACCEL_ID_MPU6500
#define ACCEL_MPU6500_RANGE             (2.f * GRAVITY_EARTH)
#define ACCEL_MPU6500_RESOLUTION        (0.004f * GRAVITY_EARTH)
#define ACCEL_MPU6500_POWER             (0.5f)
#define ACCEL_MPU6500_MINDELAY          (5000)
//ACCEL_ID_MPU9350
#define ACCEL_MPU9350_RANGE             (2.f * GRAVITY_EARTH)
#define ACCEL_MPU9350_RESOLUTION        (0.004f * GRAVITY_EARTH)
#define ACCEL_MPU9350_POWER             (0.5f)
#define ACCEL_MPU9350_MINDELAY          (1000)
//ACCEL_ID_MPU9250
#define ACCEL_MPU9250_RANGE             (2.f * GRAVITY_EARTH)
#define ACCEL_MPU9250_RESOLUTION        (0.004f * GRAVITY_EARTH)
#define ACCEL_MPU9250_POWER             (0.5f)
#define ACCEL_MPU9250_MINDELAY          (5000)
//ACCEL_ID_MPU9150
#define ACCEL_MPU9150_RANGE             (2.f * GRAVITY_EARTH)
#define ACCEL_MPU9150_RESOLUTION        (0.004f * GRAVITY_EARTH)
#define ACCEL_MPU9150_POWER             (0.5f)
#define ACCEL_MPU9150_MINDELAY          (1000)
//ACCEL_ID_LIS331
#define ACCEL_LIS331_RANGE              (2.48f * GRAVITY_EARTH)
#define ACCEL_LIS331_RESOLUTION         (0.001f * GRAVITY_EARTH)
#define ACCEL_LIS331_POWER              (1.f)
//ACCEL_ID_LSM303DLX
#define ACCEL_LSM303DLX_RANGE           (2.48f * GRAVITY_EARTH)
#define ACCEL_LSM303DLX_RESOLUTION      (0.001f * GRAVITY_EARTH)
#define ACCEL_LSM303DLX_POWER           (1.f)
//ACCEL_ID_LIS3DH
#define ACCEL_LIS3DH_RANGE              (2.48f * GRAVITY_EARTH)
#define ACCEL_LIS3DH_RESOLUTION         (0.001f * GRAVITY_EARTH)
#define ACCEL_LIS3DH_POWER              (1.f)
//ACCEL_ID_KXSD9
#define ACCEL_KXSD9_RANGE               (2.5006f * GRAVITY_EARTH)
#define ACCEL_KXSD9_RESOLUTION          (0.001f * GRAVITY_EARTH)
#define ACCEL_KXSD9_POWER               (1.f)
//ACCEL_ID_KXTF9
#define ACCEL_KXTF9_RANGE               (1.f * GRAVITY_EARTH)
#define ACCEL_KXTF9_RESOLUTION          (0.033f * GRAVITY_EARTH)
#define ACCEL_KXTF9_POWER               (0.35f)
//ACCEL_ID_BMA150
#define ACCEL_BMA150_RANGE              (2.f * GRAVITY_EARTH)
#define ACCEL_BMA150_RESOLUTION         (0.004f * GRAVITY_EARTH)
#define ACCEL_BMA150_POWER              (0.2f)
//ACCEL_ID_BMA222
#define ACCEL_BMA222_RANGE              (2.f * GRAVITY_EARTH)
#define ACCEL_BMA222_RESOLUTION         (0.001f * GRAVITY_EARTH)
#define ACCEL_BMA222_POWER              (0.1f)
//ACCEL_ID_BMA250
#define ACCEL_BMA250_RANGE              (2.f * GRAVITY_EARTH)
#define ACCEL_BMA250_RESOLUTION         (0.00391f * GRAVITY_EARTH)
#define ACCEL_BMA250_POWER              (0.139f)
#define ACCEL_BMA250_MINDELAY           (1000)
//ACCEL_ID_ADXL34X
#define ACCEL_ADXL34X_RANGE             (2.f * GRAVITY_EARTH)
#define ACCEL_ADXL34X_RESOLUTION        (0.001f * GRAVITY_EARTH)
#define ACCEL_ADXL34X_POWER             (1.f)
//ACCEL_ID_MMA8450
#define ACCEL_MMA8450_RANGE             (2.f * GRAVITY_EARTH)
#define ACCEL_MMA8450_RESOLUTION        (0.001f * GRAVITY_EARTH)
#define ACCEL_MMA8450_POWER             (1.0f)
//ACCEL_ID_MMA845X
#define ACCEL_MMA845X_RANGE             (2.f * GRAVITY_EARTH)
#define ACCEL_MMA845X_RESOLUTION        (0.001f * GRAVITY_EARTH)
#define ACCEL_MMA845X_POWER             (1.f)
//ACCEL_ID_MPU6050
#define ACCEL_MPU6050_RANGE             (2.f * GRAVITY_EARTH)
#define ACCEL_MPU6050_RESOLUTION        (0.004f * GRAVITY_EARTH)
#define ACCEL_MPU6050_POWER             (5.5f)
#define ACCEL_MPU6050_MINDELAY          (1000)
/******************************************/
//GYRO MPU3050
#define RAD_P_DEG                       (3.14159f / 180.f)
#define GYRO_MPU3050_RANGE              (2000.f * RAD_P_DEG)
#define GYRO_MPU3050_RESOLUTION         (2000.f / 32768.f * RAD_P_DEG)
#define GYRO_MPU3050_POWER              (6.1f)
#define GYRO_MPU3050_MINDELAY           (1000)
//GYRO MPU6050
#define GYRO_MPU6050_RANGE              (2000.f * RAD_P_DEG)
#define GYRO_MPU6050_RESOLUTION         (2000.f / 32768.f * RAD_P_DEG)
#define GYRO_MPU6050_POWER              (5.5f)
#define GYRO_MPU6050_MINDELAY           (1000)
//GYRO MPU9150
#define GYRO_MPU9150_RANGE              (2000.f * RAD_P_DEG)
#define GYRO_MPU9150_RESOLUTION         (2000.f / 32768.f * RAD_P_DEG)
#define GYRO_MPU9150_POWER              (5.5f)
#define GYRO_MPU9150_MINDELAY           (1000)
//GYRO MPU9350
#define GYRO_MPU9350_RANGE              (2000.f * RAD_P_DEG)
#define GYRO_MPU9350_RESOLUTION         (2000.f / 32768.f * RAD_P_DEG)
#define GYRO_MPU9350_POWER              (5.5f)
#define GYRO_MPU9350_MINDELAY           (1000)
//GYRO MPU9250
#define GYRO_MPU9250_RANGE              (2000.f * RAD_P_DEG)
#define GYRO_MPU9250_RESOLUTION         (2000.f / 32768.f * RAD_P_DEG)
#define GYRO_MPU9250_POWER              (5.5f)
#define GYRO_MPU9250_MINDELAY           (5000)
//GYRO MPU6500
#define GYRO_MPU6500_RANGE              (2000.f * RAD_P_DEG)
#define GYRO_MPU6500_RESOLUTION         (2000.f / 32768.f * RAD_P_DEG)
#define GYRO_MPU6500_POWER              (5.5f)
#define GYRO_MPU6500_MINDELAY           (5000)
//GYRO ITG3500
#define GYRO_ITG3500_RANGE              (2000.f * RAD_P_DEG)
#define GYRO_ITG3500_RESOLUTION         (2000.f / 32768.f * RAD_P_DEG)
#define GYRO_ITG3500_POWER              (5.5f)
#define GYRO_ITG3500_MINDELAY           (1000)
/******************************************/
//PRESSURE BMP280
#define PRESSURE_BMP280_RANGE           (1100.f)   // hpa
#define PRESSURE_BMP280_RESOLUTION      (0.009995f)// in psi
#define PRESSURE_BMP280_POWER           (0.004f)   // 0.004mA
#define PRESSURE_BMP280_MINDELAY        (33333)    // 30Hz unit in ns 
#endif  /* INV_SENSOR_PARAMS_H */

