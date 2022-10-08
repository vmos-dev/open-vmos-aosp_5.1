/*
 * module.h, dynamic module interface
 *
 * Copyright (c) 2009-2010 Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __MODULE_H
#define __MODULE_H

#include <dlfcn.h>

#ifdef __cplusplus
extern "C" {
#endif

struct module;

typedef int (*module_init_t)(struct module *module);
typedef void (*module_exit_t)(struct module *module);

struct module {
    char *name;

    int ref_count;
    void *handle;

    module_init_t init;
    module_exit_t exit;

    void *priv;
    struct module *next;
};

#define MODULE_LAZY RTLD_LAZY
#define MODULE_NOW RTLD_NOW
#define MODUEL_GLOBAL RTLD_GLOBAL

struct module *module_open(const char *path, int flag);
int module_close(struct module *module);
void *module_symbol(struct module *module, const char *string);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* __MODULE_H */
