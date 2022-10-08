/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef _HARDWARE_VIBRATOR_H
#define _HARDWARE_VIBRATOR_H

#if __cplusplus
extern "C" {
#endif

/**
 * Return whether the device has a vibrator.
 *
 * @return 1 if a vibrator exists, 0 if it doesn't.
 */
int vibrator_exists();

/**
 * Turn on vibrator
 *
 * @param timeout_ms number of milliseconds to vibrate
 *
 * @return 0 if successful, -1 if error
 */
int vibrator_on(int timeout_ms);

/**
 * Turn off vibrator
 *
 * @return 0 if successful, -1 if error
 */
int vibrator_off();

#if __cplusplus
}  // extern "C"
#endif

#endif  // _HARDWARE_VIBRATOR_H
