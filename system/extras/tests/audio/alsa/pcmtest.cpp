/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless requied by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <gtest/gtest.h>
#include <linux/ioctl.h>
#define __force
#define __bitwise
#define __user
#include <sound/asound.h>
#include <sys/types.h>
#include <tinyalsa/asoundlib.h>

#define LOG_TAG "pcmtest"
#include <utils/Log.h>
#include <testUtil.h>

#define PCM_PREFIX	"pcm"
#define MIXER_PREFIX	"control"
#define TIMER_PREFIX	"timer"

const char kSoundDir[] = "/dev/snd";

typedef struct PCM_NODE {
    unsigned int card;
    unsigned int device;
    unsigned int flags;
} pcm_node_t;

static pcm_node_t *pcmnodes;

static unsigned int pcms;
static unsigned int cards;
static unsigned int mixers;
static unsigned int timers;

int getPcmNodes(void)
{
    DIR *d;
    struct dirent *de;
    unsigned int pcount = 0;

    d = opendir(kSoundDir);
    if (d == 0)
        return 0;
    while ((de = readdir(d)) != NULL) {
        if (de->d_name[0] == '.')
            continue;
        if (strstr(de->d_name, PCM_PREFIX))
            pcount++;
    }
    closedir(d);
    return pcount;
}

int getSndDev(unsigned int pcmdevs)
{
    DIR *d;
    struct dirent *de;
    unsigned int prevcard = -1;

    d = opendir(kSoundDir);
    if (d == 0)
        return -ENXIO;
    pcmnodes = (pcm_node_t *)malloc(pcmdevs * sizeof(pcm_node_t));
    if (!pcmnodes)
        return -ENOMEM;
    pcms = 0;
    while ((de = readdir(d)) != NULL) {
        if (de->d_name[0] == '.')
            continue;
        /* printf("%s\n", de->d_name); */
        if (strstr(de->d_name, PCM_PREFIX)) {
            char flags;

            EXPECT_LE(pcms, pcmdevs) << "Too many PCMs";
            if (pcms >= pcmdevs)
                continue;
            sscanf(de->d_name, PCM_PREFIX "C%uD%u", &(pcmnodes[pcms].card),
                   &(pcmnodes[pcms].device));
            flags = de->d_name[strlen(de->d_name)-1];
            if (flags == 'c') {
                pcmnodes[pcms].flags = PCM_IN;
            } else if(flags == 'p') {
                pcmnodes[pcms].flags = PCM_OUT;
            } else {
                pcmnodes[pcms].flags = -1;
                testPrintI("Unknown PCM type = %c", flags);
            }
            if (prevcard != pcmnodes[pcms].card)
                cards++;
            prevcard = pcmnodes[pcms].card;
            pcms++;
            continue;
        }
        if (strstr(de->d_name, MIXER_PREFIX)) {
            unsigned int mixer = -1;
            sscanf(de->d_name, MIXER_PREFIX "C%u", &mixer);
            mixers++;
            continue;
        }
        if (strstr(de->d_name, TIMER_PREFIX)) {
            timers++;
            continue;
        }
    }
    closedir(d);
    return 0;
}

int getPcmParams(unsigned int i)
{
    struct pcm_params *params;
    unsigned int min;
    unsigned int max;

    params = pcm_params_get(pcmnodes[i].card, pcmnodes[i].device,
                            pcmnodes[i].flags);
    if (params == NULL)
        return -ENODEV;

    min = pcm_params_get_min(params, PCM_PARAM_RATE);
    max = pcm_params_get_max(params, PCM_PARAM_RATE);
    EXPECT_LE(min, max);
    /* printf("        Rate:\tmin=%uHz\tmax=%uHz\n", min, max); */
    min = pcm_params_get_min(params, PCM_PARAM_CHANNELS);
    max = pcm_params_get_max(params, PCM_PARAM_CHANNELS);
    EXPECT_LE(min, max);
    /* printf("    Channels:\tmin=%u\t\tmax=%u\n", min, max); */
    min = pcm_params_get_min(params, PCM_PARAM_SAMPLE_BITS);
    max = pcm_params_get_max(params, PCM_PARAM_SAMPLE_BITS);
    EXPECT_LE(min, max);
    /* printf(" Sample bits:\tmin=%u\t\tmax=%u\n", min, max); */
    min = pcm_params_get_min(params, PCM_PARAM_PERIOD_SIZE);
    max = pcm_params_get_max(params, PCM_PARAM_PERIOD_SIZE);
    EXPECT_LE(min, max);
    /* printf(" Period size:\tmin=%u\t\tmax=%u\n", min, max); */
    min = pcm_params_get_min(params, PCM_PARAM_PERIODS);
    max = pcm_params_get_max(params, PCM_PARAM_PERIODS);
    EXPECT_LE(min, max);
    /* printf("Period count:\tmin=%u\t\tmax=%u\n", min, max); */

    pcm_params_free(params);
    return 0;
}

TEST(pcmtest, CheckAudioDir) {
    pcms = getPcmNodes();
    ASSERT_GT(pcms, 0);
}

TEST(pcmtest, GetSoundDevs) {
    int err = getSndDev(pcms);
    testPrintI(" DEVICES = PCMS:%u CARDS:%u MIXERS:%u TIMERS:%u",
               pcms, cards, mixers, timers);
    ASSERT_EQ(0, err);
}

TEST(pcmtest, CheckPcmSanity0) {
    ASSERT_NE(0, pcms);
}

TEST(pcmtest, CheckPcmSanity1) {
    EXPECT_NE(1, pcms % 2);
}

TEST(pcmtests, CheckMixerSanity) {
    ASSERT_NE(0, mixers);
    ASSERT_EQ(mixers, cards);
}

TEST(pcmtest, CheckTimesSanity0) {
    ASSERT_NE(0, timers);
}

TEST(pcmtest, CheckTimesSanity1) {
    EXPECT_EQ(1, timers);
}

TEST(pcmtest, CheckPcmDevices) {
    for (unsigned int i = 0; i < pcms; i++) {
        EXPECT_EQ(0, getPcmParams(i));
    }
    free(pcmnodes);
}

TEST(pcmtest, CheckMixerDevices) {
    struct mixer *mixer;
    for (unsigned int i = 0; i < mixers; i++) {
         mixer = mixer_open(i);
         EXPECT_TRUE(mixer != NULL);
         if (mixer)
             mixer_close(mixer);
    }
}

TEST(pcmtest, CheckTimer) {
    int ver = 0;
    int fd = open("/dev/snd/timer", O_RDWR | O_NONBLOCK);
    ASSERT_GE(fd, 0);
    int ret = ioctl(fd, SNDRV_TIMER_IOCTL_PVERSION, &ver);
    EXPECT_EQ(0, ret);
    testPrintI(" Timer Version = 0x%x", ver);
    close(fd);
}
