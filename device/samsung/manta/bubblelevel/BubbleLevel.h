/*
 * Copyright (C) 2012 The Android Open Source Project
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

#ifndef ANDROID_BUBBLE_LEVEL_
#define ANDROID_BUBBLE_LEVEL_

// Callback for level measurements. isLevel is true when the device lies flat.
#define BL_POLL_INTERVAL_MIN_SEC 1
#define BL_POLL_INTERVAL_DEFAULT_SEC 10

typedef void (*BubbleLevel_CallBack_t)(bool isLevel, void *userData);

#ifdef __cplusplus

class BubbleLevel
{
public:
    virtual ~BubbleLevel() {}

    static BubbleLevel *create();

    // Set a callback called every time level measurement is complete
    virtual int setCallback(BubbleLevel_CallBack_t callback, void *userData) = 0;
    // Set the callback interval in seconds
    virtual int setPollInterval(unsigned int seconds) = 0;
    // Start polling for level: the callback will be called every poll interval
    virtual int startPolling() = 0;
    // Start polling for level: the callback will not be called any more and the accelerometer
    // resource is released
    virtual int stopPolling() = 0;
    // The callback will be called once with current level measurement
    virtual int pollOnce() = 0;
};

extern "C" {
#endif /* __cplusplus */

struct bubble_level *bubble_level_create();
void bubble_level_release(const struct bubble_level *bubble_level);

struct bubble_level
{
    // Set a callback called every time level measurement is complete
    int (*set_callback)(const struct bubble_level *bubble_level,
                     BubbleLevel_CallBack_t callback, void *userData);
    // Set the callback interval in seconds
    int (*set_poll_interval)(const struct bubble_level *bubble_level,
                          unsigned int seconds);
    // Start polling for level: the callback will be called every poll interval
    int (*start_polling)(const struct bubble_level *bubble_level);
    // Start polling for level: the callback will not be called any more and the accelerometer
    // resource is released
    int (*stop_polling)(const struct bubble_level *bubble_level);
    // The callback will be called once with current level measurement
    int (*poll_once)(const struct bubble_level *bubble_level);
};

#ifdef __cplusplus
};
#endif /* __cplusplus */

#endif /*ANDROID_BUBBLE_LEVEL_*/
