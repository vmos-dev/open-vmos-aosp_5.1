/*
 * module.c, dynamic module interface
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

#include <string.h>
#include <stdlib.h>
#include <pthread.h>

#include <module.h>

#define LOG_TAG "module"
#include <log.h>

static struct module *g_module_head;
static char *g_module_err;

static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

#define for_each_module(__module, __head)               \
    for ((__module) = (__head); (__module) != NULL;     \
         (__module) = (__module)->next)

#define find_last_module(__head)                \
    ({                                          \
        struct module *m;                       \
                                                \
        for_each_module(m, (__head)) {          \
            if (!m->next)                       \
                break;                          \
        }                                       \
        m;                                      \
    })


static struct module *module_find_with_name(struct module *head,
                                            const char *filename)
{
    struct module *module;

    for_each_module(module, head) {
        if (!strcmp(module->name, filename))
            return module;
    }

    return NULL;
}

static struct module *module_find_with_handle(struct module *head,
                                              const void *handle)
{
    struct module *module;

    for_each_module(module, head) {
        if (module->handle == handle)
            return module;
    }

    return NULL;
}

static struct module *module_add_list(struct module *head,
                                      struct module *add)
{
    struct module *last;

    last = find_last_module(head);
    if (last)
        last->next = add;
    else
        head = add;

    return head;
}

static struct module *module_del_list(struct module *head,
                                      struct module *del)
{
    struct module *prev = NULL;

    for_each_module(prev, head) {
        if (prev->next == del)
            break;
    }

    if (!prev)
        head = del->next;
    else
        prev->next = del->next;

    return head;
}

static inline void module_set_error(const char *dlerr)
{
    if (g_module_err)
        free(g_module_err);

    if (dlerr)
        g_module_err = strdup(dlerr);
    else
        g_module_err = NULL;
}

const char *module_error(void)
{
    return g_module_err;
}

struct module *module_open(const char *file, int flag)
{
    struct module *new, *existing;
    void *handle;
    const char *dlerr;
    int init_ret = 0;

    pthread_mutex_lock(&g_lock);

    existing = module_find_with_name(g_module_head, file);
    if (existing) {
        LOGE("found opened module %s with name\n", existing->name);
        existing->ref_count++;
        pthread_mutex_unlock(&g_lock);
        return existing;
    }

    new = malloc(sizeof(*new));
    if (!new) {
        pthread_mutex_unlock(&g_lock);
        return NULL;
    }

    new->ref_count = 1;
    new->priv = NULL;
    new->next = NULL;
    new->handle = NULL;

    new->handle = dlopen(file, flag);
    if (!(new->handle)) {
        LOGE("dlopen failed (%s)\n", file);
        dlerr = dlerror();
        module_set_error(dlerr);
        goto free_new;
    }

    existing = module_find_with_handle(g_module_head, new->handle);
    if (existing) {
        LOGE("found opened module %s with handle\n", existing->name);
        existing->ref_count++;

        free(new);
        pthread_mutex_unlock(&g_lock);
        return existing;
    }

    new->name = strdup(file);
    if (!new->name) {
        goto free_handle;
    }

    dlerror();
    new->init = dlsym(new->handle, "module_init");
    dlerr = dlerror();
    if (!dlerr && new->init) {
        LOGE("module %s has init(), call the symbol\n", new->name);
        init_ret = new->init(new);
    }

    if (init_ret) {
        LOGE("module %s init() failed (%d)\n", new->name, init_ret);
        goto free_handle;
    }

    dlerror();
    new->exit = dlsym(new->handle, "module_exit");
    dlerr = dlerror();
    if (dlerr)
        new->exit = NULL;

    g_module_head = module_add_list(g_module_head, new);

    pthread_mutex_unlock(&g_lock);
    return new;

free_handle:
    dlclose(new->handle);

free_new:
    free(new);

    pthread_mutex_unlock(&g_lock);
    return NULL;
}

int module_close(struct module *module)
{
    const char *dlerr;
    int ret = 0;

    if (!module || !module->handle)
        return 0;

    pthread_mutex_lock(&g_lock);

    module->ref_count--;
    ret = module->ref_count;

    LOGV("module %s decrease refcont (%d)\n", module->name, module->ref_count);

    if (!module->ref_count) {
        if (module->exit)
            module->exit(module);

        if (dlclose(module->handle)) {
            LOGE("module %s dlclose failed",module->name);
        }

        g_module_head = module_del_list(g_module_head, module);

        LOGV("module %s closed\n", module->name);

        free(module->name);
        free(module);
    }

    pthread_mutex_unlock(&g_lock);
    return ret;
}

void *module_symbol(struct module *module, const char *string)
{
    void *symbol = NULL;
    const char *dlerr;

    if (!module || !module->handle || !string)
        return NULL;

    pthread_mutex_lock(&g_lock);

    symbol = dlsym(module->handle, string);
    if (!symbol) {
        dlerr = dlerror();
        LOGE("not founded symbol %s in module %s (%s)\n",
             string, module->name, dlerr);
        module_set_error(dlerr);
        symbol = NULL;
    }
    else
        LOGV("found symbol %s in module %s\n", string, module->name);

    pthread_mutex_unlock(&g_lock);
    return symbol;
}
