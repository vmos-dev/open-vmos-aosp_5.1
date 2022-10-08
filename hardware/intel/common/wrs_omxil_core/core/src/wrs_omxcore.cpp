/*
 * wrs_core.cpp, Wind River OpenMax-IL Core
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include <OMX_Core.h>
#include <OMX_Component.h>

#include <list.h>
#include <cmodule.h>
#include <componentbase.h>

//#define LOG_NDEBUG 0

#define LOG_TAG "wrs-omxil-core"
#include <log.h>

static unsigned int g_initialized = 0;
static unsigned int g_nr_instances = 0;

static struct list *g_module_list = NULL;
static pthread_mutex_t g_module_lock = PTHREAD_MUTEX_INITIALIZER;

static struct list *construct_components(const char *config_file_name)
{
    FILE *config_file;
    char library_name[OMX_MAX_STRINGNAME_SIZE];
    char config_file_path[256];
    struct list *head = NULL;

    strncpy(config_file_path, "/etc/", 256);
    strncat(config_file_path, config_file_name, 256);
    config_file = fopen(config_file_path, "r");
    if (!config_file) {
        strncpy(config_file_path, "./", 256);
        strncat(config_file_path, config_file_name, 256);
        config_file = fopen(config_file_path, "r");
        if (!config_file) {
            LOGE("not found file %s\n", config_file_name);
            return NULL;
        }
    }

    while (fscanf(config_file, "%s", library_name) > 0) {
        CModule *cmodule;
        struct list *entry;
        OMX_ERRORTYPE ret;

        library_name[OMX_MAX_STRINGNAME_SIZE-1] = '\0';

        /* skip libraries starting with # */
        if (library_name[0] == '#')
            continue;

        cmodule = new CModule(&library_name[0]);
        if (!cmodule)
            continue;

        LOGI("found component library %s\n", library_name);

        ret = cmodule->Load(MODULE_LAZY);
        if (ret != OMX_ErrorNone)
            goto delete_cmodule;

        ret = cmodule->QueryComponentNameAndRoles();
        if (ret != OMX_ErrorNone)
            goto unload_cmodule;

        entry = list_alloc(cmodule);
        if (!entry)
            goto unload_cmodule;
        head = __list_add_tail(head, entry);

        cmodule->Unload();
        LOGI("module %s:%s added to component list\n",
             cmodule->GetLibraryName(), cmodule->GetComponentName());

        continue;

    unload_cmodule:
        cmodule->Unload();
    delete_cmodule:
        delete cmodule;
    }

    fclose(config_file);
    return head;
}

static struct list *destruct_components(struct list *head)
{
    struct list *entry, *next;

    list_foreach_safe(head, entry, next) {
        CModule *cmodule = static_cast<CModule *>(entry->data);

        head = __list_delete(head, entry);
        delete cmodule;
    }

    return head;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_Init(void)
{
    int ret;

    LOGV("%s(): enter", __FUNCTION__);

    pthread_mutex_lock(&g_module_lock);
    if (!g_initialized) {
        g_module_list = construct_components("wrs_omxil_components.list");
        if (!g_module_list) {
            pthread_mutex_unlock(&g_module_lock);
            LOGE("%s(): exit failure, construct_components failed",
                 __FUNCTION__);
            return OMX_ErrorInsufficientResources;
        }

        g_initialized = 1;
    }
    pthread_mutex_unlock(&g_module_lock);

    LOGV("%s(): exit done", __FUNCTION__);
    return OMX_ErrorNone;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_Deinit(void)
{
    OMX_ERRORTYPE ret = OMX_ErrorNone;

    LOGV("%s(): enter", __FUNCTION__);

    pthread_mutex_lock(&g_module_lock);
    if (!g_nr_instances)
        g_module_list = destruct_components(g_module_list);
    else
        ret = OMX_ErrorUndefined;
    pthread_mutex_unlock(&g_module_lock);

    g_initialized = 0;

    LOGV("%s(): exit done (ret : 0x%08x)", __FUNCTION__, ret);
    return ret;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_ComponentNameEnum(
    OMX_OUT OMX_STRING cComponentName,
    OMX_IN OMX_U32 nNameLength,
    OMX_IN OMX_U32 nIndex)
{
    CModule *cmodule;
    ComponentBase *cbase;
    struct list *entry;
    OMX_STRING cname;

    pthread_mutex_lock(&g_module_lock);
    entry = __list_entry(g_module_list, nIndex);
    if (!entry) {
        pthread_mutex_unlock(&g_module_lock);
        return OMX_ErrorNoMore;
    }
    pthread_mutex_unlock(&g_module_lock);

    cmodule = static_cast<CModule *>(entry->data);
    cname = cmodule->GetComponentName();

    strncpy(cComponentName, cname, nNameLength);

    LOGV("%s(): found %u th component %s", __FUNCTION__, nIndex, cname);
    return OMX_ErrorNone;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_GetHandle(
    OMX_OUT OMX_HANDLETYPE* pHandle,
    OMX_IN OMX_STRING cComponentName,
    OMX_IN OMX_PTR pAppData,
    OMX_IN OMX_CALLBACKTYPE *pCallBacks)
{
    struct list *entry;
    OMX_ERRORTYPE ret;

    LOGV("%s(): enter, try to get %s", __FUNCTION__, cComponentName);

    pthread_mutex_lock(&g_module_lock);
    list_foreach(g_module_list, entry) {
        CModule *cmodule;
        OMX_STRING cname;

        cmodule = static_cast<CModule *>(entry->data);

        cname = cmodule->GetComponentName();
        if (!strcmp(cComponentName, cname)) {
            ComponentBase *cbase = NULL;

            ret = cmodule->Load(MODULE_NOW);
            if (ret != OMX_ErrorNone) {
                LOGE("%s(): exit failure, cmodule->Load failed\n",
                     __FUNCTION__);
                goto unlock_list;
            }

            ret = cmodule->InstantiateComponent(&cbase);
            if (ret != OMX_ErrorNone){
                LOGE("%s(): exit failure, cmodule->Instantiate failed\n",
                     __FUNCTION__);
                goto unload_cmodule;
            }

            ret = cbase->GetHandle(pHandle, pAppData, pCallBacks);
            if (ret != OMX_ErrorNone) {
                LOGE("%s(): exit failure, cbase->GetHandle failed\n",
                     __FUNCTION__);
                goto delete_cbase;
            }

            cbase->SetCModule(cmodule);

            g_nr_instances++;
            pthread_mutex_unlock(&g_module_lock);

            LOGI("get handle of component %s successfully", cComponentName);
            LOGV("%s(): exit done\n", __FUNCTION__);
            return OMX_ErrorNone;

        delete_cbase:
            delete cbase;
        unload_cmodule:
            cmodule->Unload();
        unlock_list:
            pthread_mutex_unlock(&g_module_lock);

            LOGE("%s(): exit failure, (ret : 0x%08x)\n", __FUNCTION__, ret);
            return ret;
        }
    }
    pthread_mutex_unlock(&g_module_lock);

    LOGE("%s(): exit failure, %s not found", __FUNCTION__, cComponentName);
    return OMX_ErrorInvalidComponent;
}

OMX_API OMX_ERRORTYPE OMX_APIENTRY OMX_FreeHandle(
    OMX_IN OMX_HANDLETYPE hComponent)
{
    ComponentBase *cbase;
    CModule *cmodule;
    OMX_ERRORTYPE ret;
    char cname[OMX_MAX_STRINGNAME_SIZE];

    if (!hComponent)
        return OMX_ErrorBadParameter;

    cbase = static_cast<ComponentBase *>
        (((OMX_COMPONENTTYPE *)hComponent)->pComponentPrivate);
    if (!cbase) {
        LOGE("%s(): exit failure, cannot find cbase pointer\n",
             __FUNCTION__);
        return OMX_ErrorBadParameter;
    }
    strcpy(&cname[0], cbase->GetName());

    LOGV("%s(): enter, try to free %s", __FUNCTION__, cbase->GetName());


    ret = cbase->FreeHandle(hComponent);
    if (ret != OMX_ErrorNone) {
        LOGE("%s(): exit failure, cbase->FreeHandle() failed (ret = 0x%08x)\n",
             __FUNCTION__, ret);
        return ret;
    }

    pthread_mutex_lock(&g_module_lock);
    g_nr_instances--;
    pthread_mutex_unlock(&g_module_lock);

    cmodule = cbase->GetCModule();
    if (!cmodule)
        LOGE("fatal error, %s does not have cmodule\n", cbase->GetName());

    delete cbase;

    if (cmodule)
        cmodule->Unload();

    LOGI("free handle of component %s successfully", cname);
    LOGV("%s(): exit done", __FUNCTION__);
    return OMX_ErrorNone;
}

OMX_API OMX_ERRORTYPE OMX_GetComponentsOfRole (
    OMX_IN OMX_STRING role,
    OMX_INOUT OMX_U32 *pNumComps,
    OMX_INOUT OMX_U8  **compNames)
{
    struct list *entry;
    OMX_U32 nr_comps = 0, copied_nr_comps = 0;

    pthread_mutex_lock(&g_module_lock);
    list_foreach(g_module_list, entry) {
        CModule *cmodule;
        OMX_STRING cname;
        bool having_role;

        cmodule = static_cast<CModule *>(entry->data);

        having_role = cmodule->QueryHavingThisRole(role);
        if (having_role) {
            if (compNames && compNames[nr_comps]) {
                cname = cmodule->GetComponentName();
                strncpy((OMX_STRING)&compNames[nr_comps][0], cname,
                        OMX_MAX_STRINGNAME_SIZE);
                copied_nr_comps++;
                LOGV("%s(): component %s has %s role", __FUNCTION__,
                     cname, role);
            }
            nr_comps++;
        }
    }
    pthread_mutex_unlock(&g_module_lock);

    if (!copied_nr_comps)
        *pNumComps = nr_comps;
    else {
        if (copied_nr_comps != *pNumComps)
            return OMX_ErrorBadParameter;
    }

    return OMX_ErrorNone;
}

OMX_API OMX_ERRORTYPE OMX_GetRolesOfComponent (
    OMX_IN OMX_STRING compName,
    OMX_INOUT OMX_U32 *pNumRoles,
    OMX_OUT OMX_U8 **roles)
{
    struct list *entry;

    pthread_mutex_lock(&g_module_lock);
    list_foreach(g_module_list, entry) {
        CModule *cmodule;
        ComponentBase *cbase;
        OMX_STRING cname;
        OMX_ERRORTYPE ret;

        cmodule = static_cast<CModule *>(entry->data);

        cname = cmodule->GetComponentName();
        if (!strcmp(compName, cname)) {
            pthread_mutex_unlock(&g_module_lock);
#if LOG_NDEBUG
            return cmodule->GetComponentRoles(pNumRoles, roles);
#else
            ret = cmodule->GetComponentRoles(pNumRoles, roles);
            if (ret != OMX_ErrorNone) {
                OMX_U32 i;

                for (i = 0; i < *pNumRoles; i++) {
                    LOGV("%s(): component %s has %s role", __FUNCTION__,
                         compName, &roles[i][0]);
                }
            }
            return ret;
#endif
        }
    }
    pthread_mutex_unlock(&g_module_lock);

    return OMX_ErrorInvalidComponent;
}
