/*
 * Copyright (C) 2010 The Android Open Source Project
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
 *
 */

/*
 * WiFi load, scan, associate, unload stress test
 *
 * Repeatedly executes the following sequence:
 *
 *   1. Load WiFi driver
 *   2. Start supplicant
 *   3. Random delay
 *   4. Obtain supplicant status (optional)
 *   5. Stop supplicant
 *   6. Unload WiFi driver
 *
 * The "Obtain supplicant status" step is optional and is pseudo
 * randomly performed 50% of the time.  The default range of
 * delay after start supplicant is intentionally selected such
 * that the obtain supplicant status and stop supplicant steps
 * may be performed while the WiFi driver is still performing a scan
 * or associate.  The default values are given by DEFAULT_DELAY_MIN
 * and DEFAULT_DELAY_MAX.  Other values can be specified through the
 * use of the -d and -D command-line options.
 *
 * Each sequence is refered to as a pass and by default an unlimited
 * number of passes are performed.  An override of the range of passes
 * to be executed is available through the use of the -s (start) and
 * -e (end) command-line options.  Can also specify a single specific
 * pass via the -p option.  There is also a default time in which the
 * test executes, which is given by DEFAULT_DURATION and can be overriden
 * through the use of the -t command-line option.
 */

#include <assert.h>
#include <errno.h>
#include <libgen.h>
#include <math.h>
#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <hardware_legacy/wifi.h>

#define LOG_TAG "wifiLoadScanAssocTest"
#include <utils/Log.h>
#include <testUtil.h>

#define DEFAULT_START_PASS     0
#define DEFAULT_END_PASS     999
#define DEFAULT_DURATION       FLT_MAX // A fairly long time, so that
                                       // range of passes will have
                                       // precedence
#define DEFAULT_DELAY_MIN      0.0     // Min delay after start supplicant
#define DEFAULT_DELAY_MAX     20.0     // Max delay after start supplicant
#define DELAY_EXP            150.0     // Exponent which determines the
                                       // amount by which values closer
                                       // to DELAY_MIN are favored.

#define CMD_STATUS           "wpa_cli status 2>&1"
#define CMD_STOP_FRAMEWORK   "stop 2>&1"
#define CMD_START_FRAMEWORK  "start 2>&1"

#define MAXSTR      100
#define MAXCMD      500

typedef unsigned int bool_t;
#define true (0 == 0)
#define false (!true)

// File scope variables
cpu_set_t availCPU;
unsigned int numAvailCPU;
float delayMin = DEFAULT_DELAY_MIN;
float delayMax = DEFAULT_DELAY_MAX;
bool_t driverLoadedAtStart;

// Command-line mutual exclusion detection flags.
// Corresponding flag set true once an option is used.
bool_t eFlag, sFlag, pFlag;

// File scope prototypes
static void init(void);
static void randDelay(void);
static void randBind(const cpu_set_t *availSet, int *chosenCPU);

/*
 * Main
 *
 * Performs the following high-level sequence of operations:
 *
 *   1. Command-line parsing
 *
 *   2. Initialization
 *
 *   3. Execute passes that repeatedly perform the WiFi load, scan,
 *      associate, unload sequence.
 *
 *   4. Restore state of WiFi driver to state it was at the
 *      start of the test.
 *
 *   5. Restart framework
 */
int
main(int argc, char *argv[])
{
    FILE *fp;
    int rv, opt;
    int cpu;
    char *chptr;
    unsigned int pass;
    char cmd[MAXCMD];
    float duration = DEFAULT_DURATION;
    unsigned int startPass = DEFAULT_START_PASS, endPass = DEFAULT_END_PASS;
    struct timeval startTime, currentTime, delta;

    testSetLogCatTag(LOG_TAG);

    // Parse command line arguments
    while ((opt = getopt(argc, argv, "d:D:s:e:p:t:?")) != -1) {
        switch (opt) {
        case 'd': // Minimum Delay
            delayMin = strtod(optarg, &chptr);
            if ((*chptr != '\0') || (delayMin < 0.0)) {
                testPrintE("Invalid command-line specified minimum delay "
                    "of: %s", optarg);
                exit(1);
            }
            break;

        case 'D': // Maximum Delay
            delayMax = strtod(optarg, &chptr);
            if ((*chptr != '\0') || (delayMax < 0.0)) {
                testPrintE("Invalid command-line specified maximum delay "
                    "of: %s", optarg);
                exit(2);
            }
            break;

        case 't': // Duration
            duration = strtod(optarg, &chptr);
            if ((*chptr != '\0') || (duration < 0.0)) {
                testPrintE("Invalid command-line specified duration of: %s",
                    optarg);
                exit(3);
            }
            break;

        case 's': // Starting Pass
            if (sFlag || pFlag) {
                testPrintE("Invalid combination of command-line options,");
                if (sFlag) {
                    testPrintE("  -s flag specified multiple times.");
                } else {
                    testPrintE("  -s and -p flags are mutually exclusive.");
                }
                exit(10);
            }
            sFlag = true;
            startPass = strtoul(optarg, &chptr, 10);
            if (*chptr != '\0') {
                testPrintE("Invalid command-line specified starting pass "
                    "of: %s", optarg);
                exit(4);
            }
            break;

        case 'e': // Ending Pass
            if (eFlag || pFlag) {
                testPrintE("Invalid combination of command-line options,");
                if (sFlag) {
                    testPrintE("  -e flag specified multiple times.");
                } else {
                    testPrintE("  -e and -p flags are mutually exclusive.");
                }
                exit(11);
            }
            eFlag = true;
            endPass = strtoul(optarg, &chptr, 10);
            if (*chptr != '\0') {
                testPrintE("Invalid command-line specified ending pass "
                    "of: %s", optarg);
                exit(5);
            }
            break;

        case 'p': // Single Specific Pass
            if (pFlag || sFlag || eFlag) {
                testPrintE("Invalid combination of command-line options,");
                if (pFlag) {
                    testPrintE("  -p flag specified multiple times.");
                } else {
                    testPrintE("  -p and -%c flags are mutually exclusive.",
                        (sFlag) ? 's' : 'e');
                }
                exit(12);
            }
            pFlag = true;
            endPass = startPass = strtoul(optarg, &chptr, 10);
            if (*chptr != '\0') {
                testPrintE("Invalid command-line specified pass "
                    "of: %s", optarg);
                exit(13);
            }
            break;

        case '?':
        default:
            testPrintE("  %s [options]", basename(argv[0]));
            testPrintE("    options:");
            testPrintE("      -s Starting pass");
            testPrintE("      -e Ending pass");
            testPrintE("      -p Specific single pass");
            testPrintE("      -t Duration");
            testPrintE("      -d Delay min");
            testPrintE("      -D Delay max");
            exit(((optopt == 0) || (optopt == '?')) ? 0 : 6);
        }
    }
    if (delayMax < delayMin) {
        testPrintE("Unexpected maximum delay less than minimum delay");
        testPrintE("  delayMin: %f delayMax: %f", delayMin, delayMax);
        exit(7);
    }
    if (endPass < startPass) {
        testPrintE("Unexpected ending pass before starting pass");
        testPrintE("  startPass: %u endPass: %u", startPass, endPass);
        exit(8);
    }
    if (argc != optind) {
        testPrintE("Unexpected command-line postional argument");
        testPrintE("  %s [-s start_pass] [-e end_pass] [-d duration]",
            basename(argv[0]));
        exit(9);
    }
    testPrintI("duration: %g", duration);
    testPrintI("startPass: %u", startPass);
    testPrintI("endPass: %u", endPass);
    testPrintI("delayMin: %f", delayMin);
    testPrintI("delayMax: %f", delayMax);

    init();

    // For each pass
    gettimeofday(&startTime, NULL);
    for (pass = startPass; pass <= endPass; pass++) {
        // Stop if duration of work has already been performed
        gettimeofday(&currentTime, NULL);
        delta = tvDelta(&startTime, &currentTime);
        if (tv2double(&delta) > duration) { break; }

        testPrintI("==== Starting pass: %u", pass);

        // Use a pass dependent sequence of random numbers
        srand48(pass);

        // Load WiFi Driver
        randBind(&availCPU, &cpu);
        if ((rv = wifi_load_driver()) != 0) {
            testPrintE("CPU: %i wifi_load_driver() failed, rv: %i\n",
                cpu, rv);
            exit(20);
        }
        testPrintI("CPU: %i wifi_load_driver succeeded", cpu);

        // Start Supplicant
        randBind(&availCPU, &cpu);
        if ((rv = wifi_start_supplicant(false)) != 0) {
            testPrintE("CPU: %i wifi_start_supplicant() failed, rv: %i\n",
                cpu, rv);
            exit(21);
        }
        testPrintI("CPU: %i wifi_start_supplicant succeeded", cpu);

        // Sleep a random amount of time
        randDelay();

        /*
         * Obtain WiFi Status
         * Half the time skip this step, which helps increase the
         * level of randomization.
         */
        if (testRandBool()) {
            rv = snprintf(cmd, sizeof(cmd), "%s", CMD_STATUS);
            if (rv >= (signed) sizeof(cmd) - 1) {
                testPrintE("Command too long for: %s\n", CMD_STATUS);
                exit(22);
            }
            testExecCmd(cmd);
        }

        // Stop Supplicant
        randBind(&availCPU, &cpu);
        if ((rv = wifi_stop_supplicant(false)) != 0) {
            testPrintE("CPU: %i wifi_stop_supplicant() failed, rv: %i\n",
                cpu, rv);
            exit(23);
        }
        testPrintI("CPU: %i wifi_stop_supplicant succeeded", cpu);

        // Unload WiFi Module
        randBind(&availCPU, &cpu);
        if ((rv = wifi_unload_driver()) != 0) {
            testPrintE("CPU: %i wifi_unload_driver() failed, rv: %i\n",
                cpu, rv);
            exit(24);
        }
        testPrintI("CPU: %i wifi_unload_driver succeeded", cpu);

        testPrintI("==== Completed pass: %u", pass);
    }

    // If needed restore WiFi driver to state it was in at the
    // start of the test.  It is assumed that it the driver
    // was loaded, then the wpa_supplicant was also running.
    if (driverLoadedAtStart) {
        // Load driver
        if ((rv = wifi_load_driver()) != 0) {
            testPrintE("main load driver failed, rv: %i", rv);
            exit(25);
        }

        // Start supplicant
        if ((rv = wifi_start_supplicant(false)) != 0) {
            testPrintE("main start supplicant failed, rv: %i", rv);
            exit(26);
        }

        // Obtain WiFi Status
        rv = snprintf(cmd, sizeof(cmd), "%s", CMD_STATUS);
        if (rv >= (signed) sizeof(cmd) - 1) {
            testPrintE("Command too long for: %s\n", CMD_STATUS);
            exit(22);
        }
        testExecCmd(cmd);
    }

    // Start framework
    rv = snprintf(cmd, sizeof(cmd), "%s", CMD_START_FRAMEWORK);
    if (rv >= (signed) sizeof(cmd) - 1) {
        testPrintE("Command too long for: %s\n", CMD_START_FRAMEWORK);
        exit(27);
    }
    testExecCmd(cmd);

    testPrintI("Successfully completed %u passes", pass - startPass);

    return 0;
}

/*
 * Initialize
 *
 * Perform testcase initialization, which includes:
 *
 *   1. Determine which CPUs are available for use
 *
 *   2. Determine total number of available CPUs
 *
 *   3. Stop framework
 *
 *   4. Determine whether WiFi driver is loaded and if so
 *      stop wpa_supplicant and unload WiFi driver.
 */
void
init(void)
{
    int rv;
    unsigned int n1;
    char cmd[MAXCMD];

    // Use whichever CPUs are available at start of test
    rv = sched_getaffinity(0, sizeof(availCPU), &availCPU);
    if (rv != 0) {
        testPrintE("init sched_getaffinity failed, rv: %i errno: %i",
            rv, errno);
        exit(40);
    }

    // How many CPUs are available
    numAvailCPU = 0;
    for (n1 = 0; n1 < CPU_SETSIZE; n1++) {
        if (CPU_ISSET(n1, &availCPU)) { numAvailCPU++; }
    }
    testPrintI("numAvailCPU: %u", numAvailCPU);

    // Stop framework
    rv = snprintf(cmd, sizeof(cmd), "%s", CMD_STOP_FRAMEWORK);
    if (rv >= (signed) sizeof(cmd) - 1) {
        testPrintE("Command too long for: %s\n", CMD_STOP_FRAMEWORK);
        exit(41);
    }
    testExecCmd(cmd);

    // Is WiFi driver loaded?
    // If so stop the wpa_supplicant and unload the driver.
    driverLoadedAtStart = is_wifi_driver_loaded();
    testPrintI("driverLoadedAtStart: %u", driverLoadedAtStart);
    if (driverLoadedAtStart) {
        // Stop wpa_supplicant
        // Might already be stopped, in which case request should
        // return immediately with success.
        if ((rv = wifi_stop_supplicant(false)) != 0) {
            testPrintE("init stop supplicant failed, rv: %i", rv);
            exit(42);
        }
        testPrintI("Stopped wpa_supplicant");

        if ((rv = wifi_unload_driver()) != 0) {
            testPrintE("init unload driver failed, rv: %i", rv);
            exit(43);
        }
        testPrintI("WiFi driver unloaded");
    }

}

/*
 * Random Delay
 *
 * Delays for a random amount of time within the range given
 * by the file scope variables delayMin and delayMax.  The
 * selected amount of delay can come from any part of the
 * range, with a bias towards values closer to delayMin.
 * The amount of bias is determined by the setting of DELAY_EXP.
 * The setting of DELAY_EXP should always be > 1.0, with higher
 * values causing a more significant bias toward the value
 * of delayMin.
 */
void randDelay(void)
{
    const unsigned long nanosecspersec = 1000000000;
    float            fract, biasedFract, amt;
    struct timeval   startTime, endTime;

    // Obtain start time
    gettimeofday(&startTime, NULL);

    // Determine random amount to sleep.
    // Values closer to delayMin are prefered by an amount
    // determined by the value of DELAY_EXP.
    fract = testRandFract();
    biasedFract = pow(DELAY_EXP, fract) / pow(DELAY_EXP, 1.0);
    amt = delayMin + ((delayMax - delayMin) * biasedFract);

    // Delay
    testDelay(amt);

    // Obtain end time and display delta
    gettimeofday(&endTime, NULL);
    testPrintI("delay: %.2f",
        (float) (tv2double(&endTime) - tv2double(&startTime)));
}

static void
randBind(const cpu_set_t *availSet, int *chosenCPU)
{
    int rv;
    cpu_set_t cpuset;
    int chosenAvail, avail, cpu, currentCPU;

    // Randomly bind to a CPU
    // Lower 16 bits from random number generator thrown away,
    // because the low-order bits tend to have the same sequence for
    // different seed values.
    chosenAvail = testRandMod(numAvailCPU);
    CPU_ZERO(&cpuset);
    avail = 0;
    for (cpu = 0; cpu < CPU_SETSIZE; cpu++) {
        if (CPU_ISSET(cpu, availSet)) {
            if (chosenAvail == avail) {
                CPU_SET(cpu, &cpuset);
                break;
            }
            avail++;
        }
    }
    assert(cpu < CPU_SETSIZE);
    sched_setaffinity(0, sizeof(cpuset), &cpuset);

    // Confirm executing on requested CPU
    if ((currentCPU = sched_getcpu()) < 0) {
        testPrintE("randBind sched_getcpu() failed, rv: %i errno: %i",
                   currentCPU, errno);
        exit(80);

    }
    if (currentCPU != cpu) {
        testPrintE("randBind executing on unexpected CPU %i, expected %i",
            currentCPU, cpu);
        exit(81);
    }

    // Let the caller know which CPU was chosen
    *chosenCPU = cpu;
}
