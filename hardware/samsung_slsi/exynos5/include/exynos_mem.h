/*
 * Copyright (C) 2008 The Android Open Source Project
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
#ifndef __INCLUDE_EXYNOS_MEM_H
#define __INCLUDE_EXYNOS_MEM_H __FILE__

/* IOCTL commands */
#define EXYNOS_MEM_SET_CACHEABLE	_IOW('M', 200, bool)
#define EXYNOS_MEM_PADDR_CACHE_FLUSH	_IOW('M', 201, struct exynos_mem_flush_range)

struct exynos_mem_flush_range {
	dma_addr_t	start;
	size_t		length;
};

#endif /* __INCLUDE_EXYNOS_MEM_H */
