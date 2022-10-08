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

/* Report realtime, uptime, awake percentage, and sleep percentage to stdout.
 * Primarily called by powerdroid test harness.
 */

#include <stdlib.h>
#include <stdio.h>

#include "utils/SystemClock.h"


int main(int argc, char *argv[])
{

    int64_t realtime, uptime;
    int64_t awaketime, sleeptime;

    uptime = android::uptimeMillis();
    realtime = android::elapsedRealtime();

    if (realtime == 0) {
        realtime = 1;
    }

    awaketime = ((1000 * uptime / realtime) + 5) / 10;
    sleeptime = ((1000 * (realtime - uptime) / realtime) + 5) / 10;

    printf("%jd %jd %jd %jd\n", (intmax_t) realtime, (intmax_t) uptime, 
            (intmax_t) awaketime, (intmax_t) sleeptime);

}


/* vim:ts=4:sw=4:softtabstop=4:smarttab:expandtab */
