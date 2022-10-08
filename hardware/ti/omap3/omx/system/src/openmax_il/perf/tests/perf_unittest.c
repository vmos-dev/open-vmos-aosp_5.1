
/*
 *  Copyright 2001-2008 Texas Instruments - http://www.ti.com/
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifdef __PERF_UNIT_TEST__

    #include "perf.h"
    #include "perf_config.h"
    #include <assert.h>
    #include <math.h>

/* unit test for the TIME macros */
void time_unit_test()
{
    TIME_STRUCT t1, t2;

    TIME_SET(t1, 1, 999999);
    TIME_SET(t2, 2, 123456);

    if (TIME_MICROSECONDS(t1) == 999999)
    {   /* we carry microseconds */
        assert(TIME_MICROSECONDS(t1) == 999999);   /* MICROSECONDS */
        assert(TIME_SECONDS(t1) == 1);             /* SECONDS */

        assert(TIME_DELTA(t2, t1) == 123457);      /* DELTA */

        TIME_COPY(t2, t1);                         /* COPY */
        assert(TIME_MICROSECONDS(t2) == 999999);
        assert(TIME_SECONDS(t2) == 1);

        TIME_INCREASE(t1, 4294967295u);             /* INCREASE */
        assert(TIME_SECONDS(t1) == 4296);
        assert(TIME_MICROSECONDS(t1) == 967294);
    }
    else if (TIME_MICROSECONDS(t1) == 0)
    {   /* we only carry seconds */
        assert(TIME_MICROSECONDS(t1) == 0);        /* MICROSECONDS */
        assert(TIME_SECONDS(t1) == 1);             /* SECONDS */

        assert(TIME_DELTA(t2, t1) == 1000000);     /* DELTA */

        TIME_COPY(t2, t1);                         /* COPY */
        assert(TIME_MICROSECONDS(t2) == 0);
        assert(TIME_SECONDS(t2) == 1);

        TIME_INCREASE(t1, 4294967295u);            /* INCREASE */
        assert(TIME_SECONDS(t1) == 4295);
        assert(TIME_MICROSECONDS(t1) == 0);

    }
    else
    {
        assert(!"unknown TIME structure");
    }
}

/* calculate how fast TIME_GET is */
void time_get_test()
{
    TIME_STRUCT t1, t2, t3;
    unsigned long count = 0, delta;
    float sum_d = 0, sum_dd = 0;

    /* get time */
    TIME_GET(t1);

    /* measure how much TIME's resolution is */
    for (count = 0; count < 1000; count++)
    {   /* take 1000 measurements */
        do
        {
            TIME_GET(t2);
        }
        while (TIME_SECONDS(t1) == TIME_SECONDS(t2) &&
               TIME_MICROSECONDS(t1) == TIME_MICROSECONDS(t2));

        delta = TIME_DELTA(t2, t1);
        sum_d += delta;
        sum_dd += delta * delta;

        TIME_COPY(t1, t2);
    }

    sum_d /= count;
    sum_dd /= count;

    fprintf(stderr, "GET_TIME resolution: %g +- %g us\n",
            sum_d, sqrt(sum_dd - sum_d * sum_d));

    /* measure how fast TIME_GET is */

    /* wait until the next second */
    do
    {
        TIME_GET(t2);
    }
    while (TIME_SECONDS(t1) == TIME_SECONDS(t2));

    /* count how many times we can get the time in a second */
    do
    {
        TIME_GET(t3);
        count++;
    }
    while (TIME_SECONDS(t3) == TIME_SECONDS(t2));

    fprintf(stderr, "We can get the time %lu times a second (get time takes"
            " %6lg us-s)\n[%lu.%06lu, %lu.%06lu, %lu.%06lu]\n",
            count,
            1000000.0/count,
            TIME_SECONDS(t1), TIME_MICROSECONDS(t1),
            TIME_SECONDS(t2), TIME_MICROSECONDS(t2),
            TIME_SECONDS(t3), TIME_MICROSECONDS(t3));
}

/* Internal unit tests

        TIME arithmetics
        TIME speed
 */

void internal_unit_test()
{
    time_unit_test();

    /* measure TIME_GET 3 times */
    time_get_test();
    time_get_test();
    time_get_test();
}

/* PERF test run - does not halt if it fails! */
void perf_test()
{
    float audioTime, videoTime;

    /* create performance objects */
    PERF_OBJHANDLE perfAVD = PERF_Create(PERF_FOURCC('M','P','l','a'),
                                         PERF_ModuleAudioDecode |
                                         PERF_ModuleVideoDecode |
                                         PERF_ModuleLLMM);
    PERF_OBJHANDLE perfAVE = PERF_Create(PERF_FOURCC('C','A','M',' '),
                                         PERF_ModuleAudioEncode |
                                         PERF_ModuleVideoEncode |
                                         PERF_ModuleHLMM);
    PERF_OBJHANDLE perfI = PERF_Create(PERF_FOURCC('I','M','G',' '),
                                       PERF_ModuleImageDecode |
                                       PERF_ModuleImageEncode |
                                       PERF_ModuleAlgorithm);

    PERF_Boundary(perfAVD, PERF_BoundarySetup | PERF_BoundaryStart);
    PERF_Boundary(perfAVE, PERF_BoundarySteadyState | PERF_BoundaryComplete);
    PERF_Boundary(perfI, PERF_BoundaryCleanup | PERF_BoundaryStart);

    PERF_SendingBuffer(perfAVD, 0x12340001, 0x0FFFFFFF, PERF_ModuleApplication);
    PERF_SendingFrame(perfAVE, 0x56780001, 0x0FFFFFFE, PERF_ModuleHardware);
    PERF_SendingBuffers(perfAVD, 0x12340002, 0x1234fff2, 0x01234567,
                        PERF_ModuleHLMM);
    PERF_SendingFrames(perfAVE, 0x56780002, 0x5678FFF2, 0x07654321,
                       PERF_ModuleSocketNode);

    PERF_SendingCommand(perfI, 0xFADEDACE, 0x00112233, PERF_ModuleMax);

    PERF_XferingBuffer(perfAVD, 0x12340001, 0x0FFFFFFF, PERF_ModuleApplication, PERF_ModuleMemory);
    PERF_XferingFrame(perfAVE, 0x56780001, 0x0FFFFFFE, PERF_ModuleHardware, PERF_ModuleCommonLayer);
    PERF_XferingBuffers(perfAVD, 0x12340002, 0x1234fff2, 0x01234567,
                        PERF_ModuleHLMM, PERF_ModuleAlgorithm);
    PERF_XferingFrames(perfAVE, 0x56780002, 0x5678FFF2, 0x07654321,
                       PERF_ModuleSocketNode, PERF_ModuleHardware);

    PERF_SendingCommand(perfI, 0xFADEDACE, 0x00112233, PERF_ModuleMax);

    PERF_ReceivedBuffer(perfAVD, 0x56780001, 0x0FFFFFFE, PERF_ModuleLLMM);
    PERF_ReceivedFrame(perfAVE, 0x12340001, 0x0FFFFFFF, PERF_ModuleMax);
    PERF_ReceivedBuffers(perfAVD, 0x56780002, 0x5678FFF2, 0x07654321,
                         PERF_ModuleAlgorithm);
    PERF_ReceivedFrames(perfAVE, 0x12340002, 0x1234FFF2, 0x01234567,
                        PERF_ModuleHLMM);

    PERF_ReceivedCommand(perfI, 0xABADDEED, 0x778899AA, PERF_ModuleHardware);

    audioTime = 0.01f; videoTime = 0.001f;
    PERF_SyncAV(perfAVD, audioTime, videoTime, PERF_SyncOpNone);
    audioTime = 1.23f; videoTime = 2.345f;
    PERF_SyncAV(perfAVD, audioTime, videoTime, PERF_SyncOpDropVideoFrame);
    audioTime = 34.56f; videoTime = 35.678f;
    PERF_SyncAV(perfAVE, audioTime, videoTime, PERF_SyncOpMax);

    PERF_ThreadCreated(perfI, 919, PERF_FOURCC('O','M','X','B'));

    PERF_Log(perfAVD, 0xFEDCBA98, 0x7654321F, 0xDB97531F);

    /* delete performance objects */
    PERF_Done(perfAVD);
    PERF_Done(perfAVE);
    PERF_Done(perfI);

    assert(perfAVD == NULL);
    assert(perfAVE == NULL);
    assert(perfI == NULL);
}

/*

  Features to be tested:


4) Log only customizable mode
        log interface gets created - needs to hand verify

*/

void create_config_file(char *content)
{
    FILE *f = fopen(PERF_CONFIG_FILE, "wt");
    if (f)
    {
        fprintf(f, "%s", content);
        fclose(f);
    }
    else
    {
        printf("Could not create config file\n");
        exit(1);
    }
}

void delete_config_file()
{
    unlink(PERF_CONFIG_FILE);
}

void test_PERF(char *description)
{
    fprintf(stderr,"-- START -- %s --\n", description);
    fprintf(stdout,"-- START -- %s --\n", description);

    perf_test();

    fprintf(stderr,"-- END -- %s --\n", description);
    fprintf(stdout,"-- END -- %s --\n", description);
}

#ifdef _WIN32
#define INVALID_PATH "iNvAlId_dIr\\"
#else
#define INVALID_PATH "iNvAlId_dIr/"
#endif

void test_PERF_creation()
{
    /* No PERF object is created - e.g. if logging, no "X blocks created" message
       will be displayed at the end of the unit test */

    /* no config file */
    delete_config_file();
    test_PERF("no config file");

    /* mask is 0 */
    create_config_file("mask = 1\n\t\tmask= \t0\t\t\n");
    test_PERF("not enabled");

    /* no trace_file, debug or log_file is specified (e.g. only replay_file) */
    create_config_file("replay_file = replay this\nlog_file=NULL\nmask = 0xFFFFFFFF\n");
    test_PERF("not enabled");

    /* trace_file cannot be created (e.g. invalid directory), and no other file was specified */
    create_config_file("trace_file = " INVALID_PATH "trace\nmask = 0xFFFFFFFF\n");
    test_PERF("invalid trace file");

    /* trace_file cannot be created but delayed_open is enabled (object should
       get created, but no output should be written, and 0 buffers should be
       generated */
    create_config_file("trace_file = " INVALID_PATH "trace\ndelayed_open = 1\nmask = 0xFFFFFFFF\n");
    test_PERF("invalid trace file + delayed open");

    /* buffer_size is too large */
    create_config_file("mask = 0xFFFFFFFF\nbuffer_size = 0x7FFFFFFF\ntrace_file = ut_trace1\n");
    test_PERF("large trace buffer size");
}

void test_PERF_output()
{
    /* STDOUT csv non-detailed */
    create_config_file("mask = 0xFFFFFFFF\ndebug = true\ncsv = 1\n");
    test_PERF("STDOUT non-detailed debug CSV");

    /* STDOUT detailed */
    create_config_file("mask = 0xFFFFFFFF\nlog_file = STDOUT\ncsv = 0\n");
    test_PERF("STDOUT detailed debug");

    /* log_file cannot be created (e.g. invalid directory), it will be printed to STDOUT */
    create_config_file("log_file = " INVALID_PATH "log\nmask = 0xFFFFFFFF\n");
    test_PERF("STDOUT because invalid log file");

    /* STDERR non-csv detailed */
    create_config_file("mask = 0xFFFFFFFF\ndetailed_debug = true\ncsv = 0\n");
    test_PERF("STDERR detailed debug");

    /* STDERR detailed CSV*/
    create_config_file("mask = 0xFFFFFFFF\nlog_file = STDERR\ncsv = enabled\n");
    test_PERF("STDERR detailed CSV debug");

    /* FILE output */
    create_config_file("mask = 0xFFFFFFFF\nlog_file = ut_log1\n");
    test_PERF("FILE");

    /* trace output */
    create_config_file("mask = 0xFFFFFFFF\ntrace_file = ut_trace2\n");
    test_PERF("TRACE");

    /* trace, FILE and debug output */
    create_config_file("mask = 0xFFFFFFFF\nlog_file = ut_log2\ntrace_file = ut_trace3\ndebug = on\ndetailed_debug = off\ndelayed_open = 1\n");
    test_PERF("FILE, TRACE and DEBUG");
}

void test_PERF_config()
{
    PERF_Config config;

    PERF_Config_Init(&config);

    /* test string and numerical values + reassignment */
    create_config_file("log_file =   string1\n"
                       "log_file =  string1 string2 \n"
                       "trace_file = NULL \t\n"
                       "mask           = 0xFFFFFFFF\n"
                       "detailed_debug = $1234567A\n"
                       "debug          = 4294967295\n"
                       "csv            = on\n"
                       "delayed_open   = true\n"
                       "test.csv       = off\n"
                       "buffer_size    = -2\n");

    PERF_Config_Read(&config, 0);
    assert(config.mask == 0xFFFFFFFF);
    assert(config.detailed_debug = 0x1234567A);
    assert(config.debug = 0xFFFFFFFF);
    assert(config.buffer_size = 0xFFFFFFFE);
    assert(config.csv);
    assert(config.delayed_open);
    assert(!strcmp(config.log_file, "string1 string2"));
    assert(!config.trace_file);

    /* test remaining numerical (boolean) values */
    config.delayed_open = 0;
    create_config_file("delayed_open   = enabled\n"
                       "detailed_debug = off\n"
                       "debug          = true\n"
                       "test.debug     = false\n"
                       "test2.debug    = true\n"
                       "csv            = disabled\n");
    PERF_Config_Read(&config, "test");
    assert(!config.debug);
    assert(!config.detailed_debug);
    assert(!config.csv);
    assert(config.delayed_open);

    PERF_Config_Release(&config);
}

int main (int argc, char **argv)
{
    internal_unit_test();

    test_PERF_config();

    test_PERF_creation();
    test_PERF_output();

    return(0);
}

#endif

