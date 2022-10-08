/*
 *
 * Copyright 2012 Samsung Electronics S.LSI Co. LTD
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

/*
 * @file        Exynos_OSAL_Library.c
 * @brief
 * @author      SeungBeom Kim (sbcrux.kim@samsung.com)
 * @version     2.0.0
 * @history
 *   2012.02.20 : Create
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

#include "Exynos_OSAL_Library.h"


void *Exynos_OSAL_dlopen(const char *filename, int flag)
{
    return dlopen(filename, flag);
}

void *Exynos_OSAL_dlsym(void *handle, const char *symbol)
{
    return dlsym(handle, symbol);
}

int Exynos_OSAL_dlclose(void *handle)
{
    return dlclose(handle);
}

const char *Exynos_OSAL_dlerror(void)
{
    return dlerror();
}
