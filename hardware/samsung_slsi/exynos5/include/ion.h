/*
 * Copyright (C) 2012 Samsung Electronics Co., Ltd.
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

#ifndef _LIB_ION_H_
#define _LIB_ION_H_

#include <unistd.h> /* size_t */

#define ION_FLAG_CACHED 1
#define ION_FLAG_CACHED_NEEDS_SYNC 2

#define ION_HEAP_SYSTEM_MASK            (1 << 0)
#define ION_HEAP_SYSTEM_CONTIG_MASK     (1 << 1)
#define ION_HEAP_EXYNOS_CONTIG_MASK     (1 << 4)
#define ION_HEAP_EXYNOS_MASK            (1 << 5)
#define ION_EXYNOS_FIMD_VIDEO_MASK    (1 << 28)
#define ION_EXYNOS_GSC_MASK		(1 << 27)
#define ION_EXYNOS_MFC_OUTPUT_MASK    (1 << 26)
#define ION_EXYNOS_MFC_INPUT_MASK    (1 << 25)


/* ION_MSYNC_FLAGS
 * values of @flags parameter to ion_msync()
 *
 * IMSYNC_DEV_TO_READ: Device only reads the buffer
 * IMSYNC_DEV_TO_WRITE: Device may writes to the buffer
 * IMSYNC_DEV_TO_RW: Device reads and writes to the buffer
 *
 * IMSYNC_SYNC_FOR_DEV: ion_msync() for device to access the buffer
 * IMSYNC_SYNC_FOR_CPU: ion_msync() for CPU to access the buffer after device
 *                      has accessed it.
 *
 * The values must be ORed with one of IMSYNC_DEV_* and one of IMSYNC_SYNC_*.
 * Otherwise, ion_msync() will not effect.
 */
enum ION_MSYNC_FLAGS {
    IMSYNC_DEV_TO_READ = 0,
    IMSYNC_DEV_TO_WRITE = 1,
    IMSYNC_DEV_TO_RW = 2,
    IMSYNC_SYNC_FOR_DEV = 0x10000,
    IMSYNC_SYNC_FOR_CPU = 0x20000,
};

#ifdef __cplusplus
extern "C" {
#endif

/* ion_client
 * An ION client is an object or an entity that needs to use the service of
 * ION and has unique address space. ion_client is an identifier of an ION
 * client and it represents the ION client.
 * All operations on ION needs a valid ion_client value and it can be obtained
 * by ion_client_create().
 */
typedef int ion_client;

/* ion_buffer
 * An identifier of a buffer allocated from ION. You must obtain to access
 * a buffer allocated from ION. If you have an effective ion_buffer, you have
 * three options to work with it.
 * - To access  the buffer, you can request an address (user virtual address)
 *   of the buffer with ion_map().
 * - To pass the buffer to the kernel, you can pass the ion_buffer to the
 *   kernel driver directly, if the kernel driver can work with ION.
 * - To pass the buffer to other processes, you can pass the ion_buffer to
 *   other processes through RPC machanism such as socket communication or
 *   Android Binder because ion_buffer is actually an open file descripotor
 *   of the current process.
 */
typedef int ion_buffer;

/* ion_client_create()
 * @RETURN: new ion_client.
 *          netative value if creating new ion_client is failed.
 *
 * A call to ion_client_create() must be paired with ion_client_destroy(),
 * symmetrically. ion_client_destroy() needs a valid ion_client that
 * is returned by ion_client_create().
 */
ion_client ion_client_create(void);

/* ion_client_destroy()
 * @client: An ion_client value to remove.
 */
void ion_client_destroy(ion_client client);

/* ion_alloc() - Allocates new buffer from ION.
 * @client: A valid ion_client value returned by ion_client_create().
 * @len: Size of a buffer required in bytes.
 * @align: Alignment requirements of @len and the start address of the allocated
 *         buffer. If the @len is not aligned by @align, ION allocates a buffer
 *         that is aligned by @align and the size of the buffer will be larger
 *         than @len.
 * @heap_mask: Mask of heaps which you want this allocation to be served from.
 * @flags: Additional requirements about buffer. ION_FLAG_CACHED for a 
 * 	   buffer you want to have a cached mapping of
 * @RETURN: An ion_buffer that represents the buffer allocated. It is only
 *          unique in the context of the given client, @client.
 *          -error if the allocation failed.
 *          See the description of ion_buffer above for detailed information.
 */
ion_buffer ion_alloc(ion_client client, size_t len, size_t align,
                     unsigned int heap_mask, unsigned int flags);

/* ion_free() - Frees an existing buffer that is allocated by ION
 * @buffer: An ion_buffer of the buffer to be released.
 */
void ion_free(ion_buffer buffer);

/* ion_map() - Obtains a virtual address of the buffer identied by @buffer
 * @buffer: The buffer to map. The virtual address returned is allocated by the
 *          kernel.
 * @len: The size of the buffer to map. This must not exceed the size of the
 *       buffer represented by @fd_buf. Thus you need to know the size of it
 *       before calling this function. If @len is less than the size of the
 *       buffer, this function just map just the size requested (@len) not the
 *       entire buffer.
 * @offset: How many pages will be ignored while mapping.@offset number of
 *       pages from the start of the buffer will not be mapped.
 * @RETURN: The start virtual addres mapped.
 *          MAP_FAILED if mapping fails.
 *
 * Note that @len + (@offset * PAGE_SIZE) must not exceed the size of the
 * buffer.
 */
void *ion_map(ion_buffer buffer, size_t len, off_t offset);

/* ion_unmap() - Frees the buffer mapped by ion_map()
 * @addr: The address returned by ion_map().
 * @len: The size of the buffer mapped by ion_map().
 * @RETURN: 0 on success, and -1 on failure.
 *          errno is also set on failure.
 */
int ion_unmap(void *addr, size_t len);

/* ion_msync() - Makes sure that data in the buffer are visible to H/W peri.
 * @client: A valid ion_client value returned by ion_client_create().
 * @buffer: The buffer to perform ion_msync().
 * @flags: Direction of access of H/W peri and CPU. See the description of
 *         ION_MSYNC_FLAGS.
 * @size: Size to ion_msync() in bytes.
 * @offset: Where ion_msync() start in @buffer, size in bytes.
 * @RETURN: 0 if successful. -error, otherwise.
 *
 * Note that @offset + @size must not exceed the size of @buffer.
 */
int ion_sync(ion_client client, ion_buffer buffer);

int ion_incRef(int fd, int share_fd, unsigned long **handle);

int ion_decRef(int fd, unsigned long *handle);

#ifdef __cplusplus
}
#endif
#endif /* _LIB_ION_H_ */
