/*
 * Copyright (C) 2014 The Android Open Source Project
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

#define LOG_TAG "platform_info"
#define LOG_NDDEBUG 0

#include <errno.h>
#include <stdio.h>
#include <expat.h>
#include <cutils/log.h>
#include <audio_hw.h>
#include "platform_api.h"
#include <platform.h>

#define PLATFORM_INFO_XML_PATH      "/system/etc/audio_platform_info.xml"

typedef enum {
    ROOT,
    ACDB,
    PCM_ID,
    BACKEND_NAME,
} section_t;

typedef void (* section_process_fn)(const XML_Char **attr);

static void process_acdb_id(const XML_Char **attr);
static void process_pcm_id(const XML_Char **attr);
static void process_backend_name(const XML_Char **attr);
static void process_root(const XML_Char **attr);

static section_process_fn section_table[] = {
    [ROOT] = process_root,
    [ACDB] = process_acdb_id,
    [PCM_ID] = process_pcm_id,
    [BACKEND_NAME] = process_backend_name,
};

static section_t section;

/*
 * <audio_platform_info>
 * <acdb_ids>
 * <device name="???" acdb_id="???"/>
 * ...
 * ...
 * </acdb_ids>
 * <backend_names>
 * <device name="???" backend="???"/>
 * ...
 * ...
 * </backend_names>
 * <pcm_ids>
 * <usecase name="???" type="in/out" id="???"/>
 * ...
 * ...
 * </pcm_ids>
 * </audio_platform_info>
 */

static void process_root(const XML_Char **attr __unused)
{
}

/* mapping from usecase to pcm dev id */
static void process_pcm_id(const XML_Char **attr)
{
    int index;

    if (strcmp(attr[0], "name") != 0) {
        ALOGE("%s: 'name' not found, no ACDB ID set!", __func__);
        goto done;
    }

    index = platform_get_usecase_index((char *)attr[1]);
    if (index < 0) {
        ALOGE("%s: usecase %s in %s not found!",
              __func__, attr[1], PLATFORM_INFO_XML_PATH);
        goto done;
    }

    if (strcmp(attr[2], "type") != 0) {
        ALOGE("%s: usecase type not mentioned", __func__);
        goto done;
    }

    int type = -1;

    if (!strcasecmp((char *)attr[3], "in")) {
        type = 1;
    } else if (!strcasecmp((char *)attr[3], "out")) {
        type = 0;
    } else {
        ALOGE("%s: type must be IN or OUT", __func__);
        goto done;
    }

    if (strcmp(attr[4], "id") != 0) {
        ALOGE("%s: usecase id not mentioned", __func__);
        goto done;
    }

    int id = atoi((char *)attr[5]);

    if (platform_set_usecase_pcm_id(index, type, id) < 0) {
        ALOGE("%s: usecase %s in %s, type %d id %d was not set!",
              __func__, attr[1], PLATFORM_INFO_XML_PATH, type, id);
        goto done;
    }

done:
    return;
}

/* backend to be used for a device */
static void process_backend_name(const XML_Char **attr)
{
    int index;

    if (strcmp(attr[0], "name") != 0) {
        ALOGE("%s: 'name' not found, no ACDB ID set!", __func__);
        goto done;
    }

    index = platform_get_snd_device_index((char *)attr[1]);
    if (index < 0) {
        ALOGE("%s: Device %s in %s not found, no ACDB ID set!",
              __func__, attr[1], PLATFORM_INFO_XML_PATH);
        goto done;
    }

    if (strcmp(attr[2], "backend") != 0) {
        ALOGE("%s: Device %s in %s has no backed set!",
              __func__, attr[1], PLATFORM_INFO_XML_PATH);
        goto done;
    }

    if (platform_set_snd_device_backend(index, attr[3]) < 0) {
        ALOGE("%s: Device %s in %s, backend %s was not set!",
              __func__, attr[1], PLATFORM_INFO_XML_PATH, attr[3]);
        goto done;
    }

done:
    return;
}

static void process_acdb_id(const XML_Char **attr)
{
    int index;

    if (strcmp(attr[0], "name") != 0) {
        ALOGE("%s: 'name' not found, no ACDB ID set!", __func__);
        goto done;
    }

    index = platform_get_snd_device_index((char *)attr[1]);
    if (index < 0) {
        ALOGE("%s: Device %s in %s not found, no ACDB ID set!",
              __func__, attr[1], PLATFORM_INFO_XML_PATH);
        goto done;
    }

    if (strcmp(attr[2], "acdb_id") != 0) {
        ALOGE("%s: Device %s in %s has no acdb_id, no ACDB ID set!",
              __func__, attr[1], PLATFORM_INFO_XML_PATH);
        goto done;
    }

    if (platform_set_snd_device_acdb_id(index, atoi((char *)attr[3])) < 0) {
        ALOGE("%s: Device %s in %s, ACDB ID %d was not set!",
              __func__, attr[1], PLATFORM_INFO_XML_PATH, atoi((char *)attr[3]));
        goto done;
    }

done:
    return;
}

static void start_tag(void *userdata __unused, const XML_Char *tag_name,
                      const XML_Char **attr)
{
    const XML_Char              *attr_name = NULL;
    const XML_Char              *attr_value = NULL;
    unsigned int                i;

    if (strcmp(tag_name, "acdb_ids") == 0) {
        section = ACDB;
    } else if (strcmp(tag_name, "pcm_ids") == 0) {
        section = PCM_ID;
    } else if (strcmp(tag_name, "backend_names") == 0) {
        section = BACKEND_NAME;
    } else if (strcmp(tag_name, "device") == 0) {
        if ((section != ACDB) && (section != BACKEND_NAME)) {
            ALOGE("device tag only supported for acdb/backend names");
            return;
        }

        /* call into process function for the current section */
        section_process_fn fn = section_table[section];
        fn(attr);
    } else if (strcmp(tag_name, "usecase") == 0) {
        if (section != PCM_ID) {
            ALOGE("usecase tag only supported with PCM_ID section");
            return;
        }

        section_process_fn fn = section_table[PCM_ID];
        fn(attr);
    }

    return;
}

static void end_tag(void *userdata __unused, const XML_Char *tag_name)
{
    if (strcmp(tag_name, "acdb_ids") == 0) {
        section = ROOT;
    } else if (strcmp(tag_name, "pcm_ids") == 0) {
        section = ROOT;
    } else if (strcmp(tag_name, "backend_names") == 0) {
        section = ROOT;
    }
}

int platform_info_init(void)
{
    XML_Parser      parser;
    FILE            *file;
    int             ret = 0;
    int             bytes_read;
    void            *buf;
    static const uint32_t kBufSize = 1024;

    section = ROOT;

    file = fopen(PLATFORM_INFO_XML_PATH, "r");
    if (!file) {
        ALOGD("%s: Failed to open %s, using defaults.",
            __func__, PLATFORM_INFO_XML_PATH);
        ret = -ENODEV;
        goto done;
    }

    parser = XML_ParserCreate(NULL);
    if (!parser) {
        ALOGE("%s: Failed to create XML parser!", __func__);
        ret = -ENODEV;
        goto err_close_file;
    }

    XML_SetElementHandler(parser, start_tag, end_tag);

    while (1) {
        buf = XML_GetBuffer(parser, kBufSize);
        if (buf == NULL) {
            ALOGE("%s: XML_GetBuffer failed", __func__);
            ret = -ENOMEM;
            goto err_free_parser;
        }

        bytes_read = fread(buf, 1, kBufSize, file);
        if (bytes_read < 0) {
            ALOGE("%s: fread failed, bytes read = %d", __func__, bytes_read);
             ret = bytes_read;
            goto err_free_parser;
        }

        if (XML_ParseBuffer(parser, bytes_read,
                            bytes_read == 0) == XML_STATUS_ERROR) {
            ALOGE("%s: XML_ParseBuffer failed, for %s",
                __func__, PLATFORM_INFO_XML_PATH);
            ret = -EINVAL;
            goto err_free_parser;
        }

        if (bytes_read == 0)
            break;
    }

err_free_parser:
    XML_ParserFree(parser);
err_close_file:
    fclose(file);
done:
    return ret;
}
