/*
 * cmodule.cpp, component module interface class
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

#include <OMX_Core.h>

#include <cmodule.h>
#include <componentbase.h>

#define LOG_TAG "cmodule"
#include <log.h>

/*
 * constructor / deconstructor
 */
CModule::CModule(const OMX_STRING lname)
{
    module = NULL;
    wrs_omxil_cmodule = NULL;

    roles = NULL;
    nr_roles = 0;

    memset(cname, 0, OMX_MAX_STRINGNAME_SIZE);

    memset(this->lname, 0, OMX_MAX_STRINGNAME_SIZE);
   // strncpy(this->name, name, OMX_MAX_STRINGNAME_SIZE);
    strncpy(this->lname, lname, (strlen(lname) < OMX_MAX_STRINGNAME_SIZE) ? strlen(lname) : (OMX_MAX_STRINGNAME_SIZE-1));
    this->lname[OMX_MAX_STRINGNAME_SIZE-1] = '\0';
}

CModule::~CModule()
{
    if (module) {
        int ref_count;

        while ((ref_count = Unload()));
    }

    if (roles) {
        if (roles[0])
            free(roles[0]);
        free(roles);
    }
}

/* end of constructor / deconstructor */

/*
 * library loading / unloading
 */
OMX_ERRORTYPE CModule::Load(int flag)
{
    struct module *m;

    m = module_open(lname, flag);
    if (!m) {
        LOGE("module not founded (%s)\n", lname);
        return OMX_ErrorComponentNotFound;
    }

    if (m == module)
        return OMX_ErrorNone;

    wrs_omxil_cmodule = (struct wrs_omxil_cmodule_s *)
        module_symbol(m, WRS_OMXIL_CMODULE_SYMBOL_STRING);
    if (!wrs_omxil_cmodule) {
        LOGE("module %s symbol not founded (%s)\n",
             lname, WRS_OMXIL_CMODULE_SYMBOL_STRING);

        module_close(m);
        return OMX_ErrorInvalidComponent;
    }

    if (module)
        LOGE("module %s will be overwrite",module->name);
    module = m;
    LOGI("module %s successfully loaded\n", lname);

    return OMX_ErrorNone;
}

OMX_U32 CModule::Unload(void)
{
    int ref_count;

    ref_count = module_close(module);
    if (!ref_count) {
        module = NULL;
        wrs_omxil_cmodule = NULL;

        LOGI("module %s successfully unloaded\n", lname);
    }

    return ref_count;
}

/* end of library loading / unloading */

/*
 * accessor
 */
OMX_STRING CModule::GetLibraryName(void)
{
    return lname;
}

OMX_STRING CModule::GetComponentName(void)
{
    return cname;
}

OMX_ERRORTYPE CModule::GetComponentRoles(OMX_U32 *nr_roles, OMX_U8 **roles)
{
    OMX_U32 i;
    OMX_U32 this_nr_roles = this->nr_roles;

    if (!roles) {
        *nr_roles = this_nr_roles;
        return OMX_ErrorNone;
    }

    if (!nr_roles || (*nr_roles != this_nr_roles))
        return OMX_ErrorBadParameter;

    for (i = 0; i < this_nr_roles; i++) {
        if (!roles[i])
            break;

        if (roles && roles[i])
            strncpy((OMX_STRING)&roles[i][0],
                    (const OMX_STRING)&this->roles[i][0],
                    OMX_MAX_STRINGNAME_SIZE);
    }

    if (i != this_nr_roles)
        return OMX_ErrorBadParameter;

    *nr_roles = this_nr_roles;
    return OMX_ErrorNone;
}

bool CModule::QueryHavingThisRole(const OMX_STRING role)
{
    OMX_U32 i;

    if (!roles || !role)
        return false;

    for (i = 0; i < nr_roles; i++) {
        if (!strcmp((OMX_STRING)&roles[i][0], role))
            return true;
    }

    return false;
}

/* end of accessor */

/*
 * library symbol method and helpers
 */
OMX_ERRORTYPE CModule::InstantiateComponent(ComponentBase **instance)
{
    ComponentBase *cbase;
    OMX_ERRORTYPE ret;

    if (!instance)
        return OMX_ErrorBadParameter;
    *instance = NULL;

    if (!wrs_omxil_cmodule)
        return OMX_ErrorUndefined;

    ret = wrs_omxil_cmodule->ops->instantiate((void **)&cbase);
    if (ret != OMX_ErrorNone) {
        LOGE("%s failed to instantiate()\n", lname);
        return ret;
    }

    cbase->SetCModule(this);
    cbase->SetName(cname);
    ret = cbase->SetRolesOfComponent(nr_roles, (const OMX_U8 **)roles);
    if (ret != OMX_ErrorNone) {
        delete cbase;
        return ret;
    }

    *instance = cbase;
    return OMX_ErrorNone;
}

OMX_ERRORTYPE CModule::QueryComponentNameAndRoles(void)
{
    const char *name;
    OMX_U32 name_len;
    OMX_U32 copy_name_len;

    const OMX_U8 **roles;
    OMX_U32 nr_roles;
    OMX_U32 role_len;
    OMX_U32 copy_role_len;
    OMX_U8 **this_roles;

    OMX_U32 i;
    OMX_ERRORTYPE ret;

    if (this->roles)
        return OMX_ErrorNone;

    if (!wrs_omxil_cmodule)
        return OMX_ErrorUndefined;

    roles = (const OMX_U8 **)wrs_omxil_cmodule->roles;
    nr_roles = wrs_omxil_cmodule->nr_roles;

    this_roles = (OMX_U8 **)malloc(sizeof(OMX_STRING) * nr_roles);
    if (!this_roles)
        return OMX_ErrorInsufficientResources;

    this_roles[0] = (OMX_U8 *)malloc(OMX_MAX_STRINGNAME_SIZE * nr_roles);
    if (!this_roles[0]) {
        free(this_roles);
        return OMX_ErrorInsufficientResources;
    }

    for (i = 0; i < nr_roles; i++) {
        if (i < nr_roles - 1)
            this_roles[i+1] = this_roles[i] + OMX_MAX_STRINGNAME_SIZE;

        role_len = strlen((const OMX_STRING)&roles[i][0]);
        copy_role_len = role_len > OMX_MAX_STRINGNAME_SIZE-1 ?
            OMX_MAX_STRINGNAME_SIZE-1 : role_len;

        strncpy((OMX_STRING)&this_roles[i][0],
                (const OMX_STRING)&roles[i][0], copy_role_len);
        this_roles[i][copy_role_len] = '\0';
    }

    this->roles = this_roles;
    this->nr_roles = nr_roles;

    name = wrs_omxil_cmodule->name;
    name_len = strlen(name);
    copy_name_len = name_len > OMX_MAX_STRINGNAME_SIZE-1 ?
        OMX_MAX_STRINGNAME_SIZE-1 : name_len;

    strncpy(&cname[0], name, copy_name_len);
    cname[copy_name_len] = '\0';

    return OMX_ErrorNone;
}

/* end of library symbol method and helpers */
