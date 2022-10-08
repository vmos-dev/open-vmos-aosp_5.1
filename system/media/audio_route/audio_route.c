/*
 * Copyright (C) 2013 The Android Open Source Project
 * Inspired by TinyHW, written by Mark Brown at Wolfson Micro
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

#define LOG_TAG "audio_route"
/*#define LOG_NDEBUG 0*/

#include <errno.h>
#include <expat.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <cutils/log.h>

#include <tinyalsa/asoundlib.h>

#define BUF_SIZE 1024
#define MIXER_XML_PATH "/system/etc/mixer_paths.xml"
#define INITIAL_MIXER_PATH_SIZE 8

struct mixer_state {
    struct mixer_ctl *ctl;
    unsigned int num_values;
    int *old_value;
    int *new_value;
    int *reset_value;
};

struct mixer_setting {
    unsigned int ctl_index;
    unsigned int num_values;
    int *value;
};

struct mixer_value {
    unsigned int ctl_index;
    int index;
    int value;
};

struct mixer_path {
    char *name;
    unsigned int size;
    unsigned int length;
    struct mixer_setting *setting;
};

struct audio_route {
    struct mixer *mixer;
    unsigned int num_mixer_ctls;
    struct mixer_state *mixer_state;

    unsigned int mixer_path_size;
    unsigned int num_mixer_paths;
    struct mixer_path *mixer_path;
};

struct config_parse_state {
    struct audio_route *ar;
    struct mixer_path *path;
    int level;
};

/* path functions */

static bool is_supported_ctl_type(enum mixer_ctl_type type)
{
    switch (type) {
    case MIXER_CTL_TYPE_BOOL:
    case MIXER_CTL_TYPE_INT:
    case MIXER_CTL_TYPE_ENUM:
        return true;
    default:
        return false;
    }
}

static inline struct mixer_ctl *index_to_ctl(struct audio_route *ar,
                                             unsigned int ctl_index)
{
    return ar->mixer_state[ctl_index].ctl;
}

static void path_print(struct audio_route *ar, struct mixer_path *path)
{
    unsigned int i;
    unsigned int j;

    ALOGE("Path: %s, length: %d", path->name, path->length);
    for (i = 0; i < path->length; i++) {
        struct mixer_ctl *ctl = index_to_ctl(ar, path->setting[i].ctl_index);

        ALOGE("  id=%d: ctl=%s", i, mixer_ctl_get_name(ctl));
        for (j = 0; j < path->setting[i].num_values; j++)
            ALOGE("    id=%d value=%d", j, path->setting[i].value[j]);
    }
}

static void path_free(struct audio_route *ar)
{
    unsigned int i;

    for (i = 0; i < ar->num_mixer_paths; i++) {
        if (ar->mixer_path[i].name)
            free(ar->mixer_path[i].name);
        if (ar->mixer_path[i].setting) {
            if (ar->mixer_path[i].setting->value)
                free(ar->mixer_path[i].setting->value);
            free(ar->mixer_path[i].setting);
        }
    }
    free(ar->mixer_path);
}

static struct mixer_path *path_get_by_name(struct audio_route *ar,
                                           const char *name)
{
    unsigned int i;

    for (i = 0; i < ar->num_mixer_paths; i++)
        if (strcmp(ar->mixer_path[i].name, name) == 0)
            return &ar->mixer_path[i];

    return NULL;
}

static struct mixer_path *path_create(struct audio_route *ar, const char *name)
{
    struct mixer_path *new_mixer_path = NULL;

    if (path_get_by_name(ar, name)) {
        ALOGE("Path name '%s' already exists", name);
        return NULL;
    }

    /* check if we need to allocate more space for mixer paths */
    if (ar->mixer_path_size <= ar->num_mixer_paths) {
        if (ar->mixer_path_size == 0)
            ar->mixer_path_size = INITIAL_MIXER_PATH_SIZE;
        else
            ar->mixer_path_size *= 2;

        new_mixer_path = realloc(ar->mixer_path, ar->mixer_path_size *
                                 sizeof(struct mixer_path));
        if (new_mixer_path == NULL) {
            ALOGE("Unable to allocate more paths");
            return NULL;
        } else {
            ar->mixer_path = new_mixer_path;
        }
    }

    /* initialise the new mixer path */
    ar->mixer_path[ar->num_mixer_paths].name = strdup(name);
    ar->mixer_path[ar->num_mixer_paths].size = 0;
    ar->mixer_path[ar->num_mixer_paths].length = 0;
    ar->mixer_path[ar->num_mixer_paths].setting = NULL;

    /* return the mixer path just added, then increment number of them */
    return &ar->mixer_path[ar->num_mixer_paths++];
}

static int find_ctl_index_in_path(struct mixer_path *path,
                                  unsigned int ctl_index)
{
    unsigned int i;

    for (i = 0; i < path->length; i++)
        if (path->setting[i].ctl_index == ctl_index)
            return i;

    return -1;
}

static int alloc_path_setting(struct mixer_path *path)
{
    struct mixer_setting *new_path_setting;
    int path_index;

    /* check if we need to allocate more space for path settings */
    if (path->size <= path->length) {
        if (path->size == 0)
            path->size = INITIAL_MIXER_PATH_SIZE;
        else
            path->size *= 2;

        new_path_setting = realloc(path->setting,
                                   path->size * sizeof(struct mixer_setting));
        if (new_path_setting == NULL) {
            ALOGE("Unable to allocate more path settings");
            return -1;
        } else {
            path->setting = new_path_setting;
        }
    }

    path_index = path->length;
    path->length++;

    return path_index;
}

static int path_add_setting(struct audio_route *ar, struct mixer_path *path,
                            struct mixer_setting *setting)
{
    int path_index;

    if (find_ctl_index_in_path(path, setting->ctl_index) != -1) {
        struct mixer_ctl *ctl = index_to_ctl(ar, setting->ctl_index);

        ALOGE("Control '%s' already exists in path '%s'",
              mixer_ctl_get_name(ctl), path->name);
        return -1;
    }

    path_index = alloc_path_setting(path);
    if (path_index < 0)
        return -1;

    path->setting[path_index].ctl_index = setting->ctl_index;
    path->setting[path_index].num_values = setting->num_values;
    path->setting[path_index].value = malloc(setting->num_values * sizeof(int));
    /* copy all values */
    memcpy(path->setting[path_index].value, setting->value,
           setting->num_values * sizeof(int));

    return 0;
}

static int path_add_value(struct audio_route *ar, struct mixer_path *path,
                          struct mixer_value *mixer_value)
{
    unsigned int i;
    int path_index;
    unsigned int num_values;
    struct mixer_ctl *ctl;

    /* Check that mixer value index is within range */
    ctl = index_to_ctl(ar, mixer_value->ctl_index);
    num_values = mixer_ctl_get_num_values(ctl);
    if (mixer_value->index >= (int)num_values) {
        ALOGE("mixer index %d is out of range for '%s'", mixer_value->index,
              mixer_ctl_get_name(ctl));
        return -1;
    }

    path_index = find_ctl_index_in_path(path, mixer_value->ctl_index);
    if (path_index < 0) {
        /* New path */

        path_index = alloc_path_setting(path);
        if (path_index < 0)
            return -1;

        /* initialise the new path setting */
        path->setting[path_index].ctl_index = mixer_value->ctl_index;
        path->setting[path_index].num_values = num_values;
        path->setting[path_index].value = malloc(num_values * sizeof(int));
        path->setting[path_index].value[0] = mixer_value->value;
    }

    if (mixer_value->index == -1) {
        /* set all values the same */
        for (i = 0; i < num_values; i++)
            path->setting[path_index].value[i] = mixer_value->value;
    } else {
        /* set only one value */
        path->setting[path_index].value[mixer_value->index] = mixer_value->value;
    }

    return 0;
}

static int path_add_path(struct audio_route *ar, struct mixer_path *path,
                         struct mixer_path *sub_path)
{
    unsigned int i;

    for (i = 0; i < sub_path->length; i++)
        if (path_add_setting(ar, path, &sub_path->setting[i]) < 0)
            return -1;

    return 0;
}

static int path_apply(struct audio_route *ar, struct mixer_path *path)
{
    unsigned int i;
    unsigned int ctl_index;
    struct mixer_ctl *ctl;
    enum mixer_ctl_type type;

    for (i = 0; i < path->length; i++) {
        ctl_index = path->setting[i].ctl_index;
        ctl = index_to_ctl(ar, ctl_index);
        type = mixer_ctl_get_type(ctl);
        if (!is_supported_ctl_type(type))
            continue;

        /* apply the new value(s) */
        memcpy(ar->mixer_state[ctl_index].new_value, path->setting[i].value,
               path->setting[i].num_values * sizeof(int));
    }

    return 0;
}

static int path_reset(struct audio_route *ar, struct mixer_path *path)
{
    unsigned int i;
    unsigned int j;
    unsigned int ctl_index;
    struct mixer_ctl *ctl;
    enum mixer_ctl_type type;

    for (i = 0; i < path->length; i++) {
        ctl_index = path->setting[i].ctl_index;
        ctl = index_to_ctl(ar, ctl_index);
        type = mixer_ctl_get_type(ctl);
        if (!is_supported_ctl_type(type))
            continue;

        /* reset the value(s) */
        memcpy(ar->mixer_state[ctl_index].new_value,
               ar->mixer_state[ctl_index].reset_value,
               ar->mixer_state[ctl_index].num_values * sizeof(int));
    }

    return 0;
}

/* mixer helper function */
static int mixer_enum_string_to_value(struct mixer_ctl *ctl, const char *string)
{
    unsigned int i;

    /* Search the enum strings for a particular one */
    for (i = 0; i < mixer_ctl_get_num_enums(ctl); i++) {
        if (strcmp(mixer_ctl_get_enum_string(ctl, i), string) == 0)
            break;
    }

    return i;
}

static void start_tag(void *data, const XML_Char *tag_name,
                      const XML_Char **attr)
{
    const XML_Char *attr_name = NULL;
    const XML_Char *attr_id = NULL;
    const XML_Char *attr_value = NULL;
    struct config_parse_state *state = data;
    struct audio_route *ar = state->ar;
    unsigned int i;
    unsigned int ctl_index;
    struct mixer_ctl *ctl;
    int value;
    unsigned int id;
    struct mixer_value mixer_value;
    enum mixer_ctl_type type;

    /* Get name, id and value attributes (these may be empty) */
    for (i = 0; attr[i]; i += 2) {
        if (strcmp(attr[i], "name") == 0)
            attr_name = attr[i + 1];
        if (strcmp(attr[i], "id") == 0)
            attr_id = attr[i + 1];
        else if (strcmp(attr[i], "value") == 0)
            attr_value = attr[i + 1];
    }

    /* Look at tags */
    if (strcmp(tag_name, "path") == 0) {
        if (attr_name == NULL) {
            ALOGE("Unnamed path!");
        } else {
            if (state->level == 1) {
                /* top level path: create and stash the path */
                state->path = path_create(ar, (char *)attr_name);
            } else {
                /* nested path */
                struct mixer_path *sub_path = path_get_by_name(ar, attr_name);
                path_add_path(ar, state->path, sub_path);
            }
        }
    }

    else if (strcmp(tag_name, "ctl") == 0) {
        /* Obtain the mixer ctl and value */
        ctl = mixer_get_ctl_by_name(ar->mixer, attr_name);
        if (ctl == NULL) {
            ALOGE("Control '%s' doesn't exist - skipping", attr_name);
            goto done;
        }

        switch (mixer_ctl_get_type(ctl)) {
        case MIXER_CTL_TYPE_BOOL:
        case MIXER_CTL_TYPE_INT:
            value = (int) strtol((char *)attr_value, NULL, 0);
            break;
        case MIXER_CTL_TYPE_ENUM:
            value = mixer_enum_string_to_value(ctl, (char *)attr_value);
            break;
        default:
            value = 0;
            break;
        }

        /* locate the mixer ctl in the list */
        for (ctl_index = 0; ctl_index < ar->num_mixer_ctls; ctl_index++) {
            if (ar->mixer_state[ctl_index].ctl == ctl)
                break;
        }

        if (state->level == 1) {
            /* top level ctl (initial setting) */

            type = mixer_ctl_get_type(ctl);
            if (is_supported_ctl_type(type)) {
                /* apply the new value */
                if (attr_id) {
                    /* set only one value */
                    id = atoi((char *)attr_id);
                    if (id < ar->mixer_state[ctl_index].num_values)
                        ar->mixer_state[ctl_index].new_value[id] = value;
                    else
                        ALOGE("value id out of range for mixer ctl '%s'",
                              mixer_ctl_get_name(ctl));
                } else {
                    /* set all values the same */
                    for (i = 0; i < ar->mixer_state[ctl_index].num_values; i++)
                        ar->mixer_state[ctl_index].new_value[i] = value;
                }
            }
        } else {
            /* nested ctl (within a path) */
            mixer_value.ctl_index = ctl_index;
            mixer_value.value = value;
            if (attr_id)
                mixer_value.index = atoi((char *)attr_id);
            else
                mixer_value.index = -1;
            path_add_value(ar, state->path, &mixer_value);
        }
    }

done:
    state->level++;
}

static void end_tag(void *data, const XML_Char *tag_name)
{
    struct config_parse_state *state = data;
    (void)tag_name;

    state->level--;
}

static int alloc_mixer_state(struct audio_route *ar)
{
    unsigned int i;
    unsigned int j;
    unsigned int num_values;
    struct mixer_ctl *ctl;
    enum mixer_ctl_type type;

    ar->num_mixer_ctls = mixer_get_num_ctls(ar->mixer);
    ar->mixer_state = malloc(ar->num_mixer_ctls * sizeof(struct mixer_state));
    if (!ar->mixer_state)
        return -1;

    for (i = 0; i < ar->num_mixer_ctls; i++) {
        ctl = mixer_get_ctl(ar->mixer, i);
        num_values = mixer_ctl_get_num_values(ctl);

        ar->mixer_state[i].ctl = ctl;
        ar->mixer_state[i].num_values = num_values;

        /* Skip unsupported types that are not supported yet in XML */
        type = mixer_ctl_get_type(ctl);

        if (!is_supported_ctl_type(type))
            continue;

        ar->mixer_state[i].old_value = malloc(num_values * sizeof(int));
        ar->mixer_state[i].new_value = malloc(num_values * sizeof(int));
        ar->mixer_state[i].reset_value = malloc(num_values * sizeof(int));

        if (type == MIXER_CTL_TYPE_ENUM)
            ar->mixer_state[i].old_value[0] = mixer_ctl_get_value(ctl, 0);
        else
            mixer_ctl_get_array(ctl, ar->mixer_state[i].old_value, num_values);
        memcpy(ar->mixer_state[i].new_value, ar->mixer_state[i].old_value,
               num_values * sizeof(int));
    }

    return 0;
}

static void free_mixer_state(struct audio_route *ar)
{
    unsigned int i;
    enum mixer_ctl_type type;

    for (i = 0; i < ar->num_mixer_ctls; i++) {
        type = mixer_ctl_get_type(ar->mixer_state[i].ctl);
        if (!is_supported_ctl_type(type))
            continue;

        free(ar->mixer_state[i].old_value);
        free(ar->mixer_state[i].new_value);
        free(ar->mixer_state[i].reset_value);
    }

    free(ar->mixer_state);
    ar->mixer_state = NULL;
}

/* Update the mixer with any changed values */
int audio_route_update_mixer(struct audio_route *ar)
{
    unsigned int i;
    unsigned int j;
    struct mixer_ctl *ctl;

    for (i = 0; i < ar->num_mixer_ctls; i++) {
        unsigned int num_values = ar->mixer_state[i].num_values;
        enum mixer_ctl_type type;

        ctl = ar->mixer_state[i].ctl;

        /* Skip unsupported types */
        type = mixer_ctl_get_type(ctl);
        if (!is_supported_ctl_type(type))
            continue;

        /* if the value has changed, update the mixer */
        bool changed = false;
        for (j = 0; j < num_values; j++) {
            if (ar->mixer_state[i].old_value[j] != ar->mixer_state[i].new_value[j]) {
                changed = true;
                break;
            }
        }
        if (changed) {
            if (type == MIXER_CTL_TYPE_ENUM)
                mixer_ctl_set_value(ctl, 0, ar->mixer_state[i].new_value[0]);
            else
                mixer_ctl_set_array(ctl, ar->mixer_state[i].new_value, num_values);
            memcpy(ar->mixer_state[i].old_value, ar->mixer_state[i].new_value,
                   num_values * sizeof(int));
        }
    }

    return 0;
}

/* saves the current state of the mixer, for resetting all controls */
static void save_mixer_state(struct audio_route *ar)
{
    unsigned int i;
    enum mixer_ctl_type type;

    for (i = 0; i < ar->num_mixer_ctls; i++) {
        type = mixer_ctl_get_type(ar->mixer_state[i].ctl);
        if (!is_supported_ctl_type(type))
            continue;

        memcpy(ar->mixer_state[i].reset_value, ar->mixer_state[i].new_value,
               ar->mixer_state[i].num_values * sizeof(int));
    }
}

/* Reset the audio routes back to the initial state */
void audio_route_reset(struct audio_route *ar)
{
    unsigned int i;
    enum mixer_ctl_type type;

    /* load all of the saved values */
    for (i = 0; i < ar->num_mixer_ctls; i++) {
        type = mixer_ctl_get_type(ar->mixer_state[i].ctl);
        if (!is_supported_ctl_type(type))
            continue;

        memcpy(ar->mixer_state[i].new_value, ar->mixer_state[i].reset_value,
               ar->mixer_state[i].num_values * sizeof(int));
    }
}

/* Apply an audio route path by name */
int audio_route_apply_path(struct audio_route *ar, const char *name)
{
    struct mixer_path *path;

    if (!ar) {
        ALOGE("invalid audio_route");
        return -1;
    }

    path = path_get_by_name(ar, name);
    if (!path) {
        ALOGE("unable to find path '%s'", name);
        return -1;
    }

    path_apply(ar, path);

    return 0;
}

/* Reset an audio route path by name */
int audio_route_reset_path(struct audio_route *ar, const char *name)
{
    struct mixer_path *path;

    if (!ar) {
        ALOGE("invalid audio_route");
        return -1;
    }

    path = path_get_by_name(ar, name);
    if (!path) {
        ALOGE("unable to find path '%s'", name);
        return -1;
    }

    path_reset(ar, path);

    return 0;
}

/*
 * Operates on the specified path .. controls will be updated in the
 * order listed in the XML file
 */
static int audio_route_update_path(struct audio_route *ar, const char *name, bool reverse)
{
    struct mixer_path *path;
    int32_t i, end;
    unsigned int j;

    if (!ar) {
        ALOGE("invalid audio_route");
        return -1;
    }

    path = path_get_by_name(ar, name);
    if (!path) {
        ALOGE("unable to find path '%s'", name);
        return -1;
    }

    i = reverse ? (path->length - 1) : 0;
    end = reverse ? -1 : (int32_t)path->length;

    while (i != end) {
        unsigned int ctl_index;
        enum mixer_ctl_type type;

        ctl_index = path->setting[i].ctl_index;

        struct mixer_state * ms = &ar->mixer_state[ctl_index];

        type = mixer_ctl_get_type(ms->ctl);
        if (!is_supported_ctl_type(type)) {
            continue;
        }

        /* if any value has changed, update the mixer */
        for (j = 0; j < ms->num_values; j++) {
            if (ms->old_value[j] != ms->new_value[j]) {
                if (type == MIXER_CTL_TYPE_ENUM)
                    mixer_ctl_set_value(ms->ctl, 0, ms->new_value[0]);
                else
                    mixer_ctl_set_array(ms->ctl, ms->new_value, ms->num_values);
                memcpy(ms->old_value, ms->new_value, ms->num_values * sizeof(int));
                break;
            }
        }

        i = reverse ? (i - 1) : (i + 1);
    }
    return 0;
}

int audio_route_apply_and_update_path(struct audio_route *ar, const char *name)
{
    if (audio_route_apply_path(ar, name) < 0) {
        return -1;
    }
    return audio_route_update_path(ar, name, false /*reverse*/);
}

int audio_route_reset_and_update_path(struct audio_route *ar, const char *name)
{
    if (audio_route_reset_path(ar, name) < 0) {
        return -1;
    }
    return audio_route_update_path(ar, name, true /*reverse*/);
}

struct audio_route *audio_route_init(unsigned int card, const char *xml_path)
{
    struct config_parse_state state;
    XML_Parser parser;
    FILE *file;
    int bytes_read;
    void *buf;
    int i;
    struct audio_route *ar;

    ar = calloc(1, sizeof(struct audio_route));
    if (!ar)
        goto err_calloc;

    ar->mixer = mixer_open(card);
    if (!ar->mixer) {
        ALOGE("Unable to open the mixer, aborting.");
        goto err_mixer_open;
    }

    ar->mixer_path = NULL;
    ar->mixer_path_size = 0;
    ar->num_mixer_paths = 0;

    /* allocate space for and read current mixer settings */
    if (alloc_mixer_state(ar) < 0)
        goto err_mixer_state;

    /* use the default XML path if none is provided */
    if (xml_path == NULL)
        xml_path = MIXER_XML_PATH;

    file = fopen(xml_path, "r");

    if (!file) {
        ALOGE("Failed to open %s", xml_path);
        goto err_fopen;
    }

    parser = XML_ParserCreate(NULL);
    if (!parser) {
        ALOGE("Failed to create XML parser");
        goto err_parser_create;
    }

    memset(&state, 0, sizeof(state));
    state.ar = ar;
    XML_SetUserData(parser, &state);
    XML_SetElementHandler(parser, start_tag, end_tag);

    for (;;) {
        buf = XML_GetBuffer(parser, BUF_SIZE);
        if (buf == NULL)
            goto err_parse;

        bytes_read = fread(buf, 1, BUF_SIZE, file);
        if (bytes_read < 0)
            goto err_parse;

        if (XML_ParseBuffer(parser, bytes_read,
                            bytes_read == 0) == XML_STATUS_ERROR) {
            ALOGE("Error in mixer xml (%s)", MIXER_XML_PATH);
            goto err_parse;
        }

        if (bytes_read == 0)
            break;
    }

    /* apply the initial mixer values, and save them so we can reset the
       mixer to the original values */
    audio_route_update_mixer(ar);
    save_mixer_state(ar);

    XML_ParserFree(parser);
    fclose(file);
    return ar;

err_parse:
    XML_ParserFree(parser);
err_parser_create:
    fclose(file);
err_fopen:
    free_mixer_state(ar);
err_mixer_state:
    mixer_close(ar->mixer);
err_mixer_open:
    free(ar);
    ar = NULL;
err_calloc:
    return NULL;
}

void audio_route_free(struct audio_route *ar)
{
    free_mixer_state(ar);
    mixer_close(ar->mixer);
    free(ar);
}
