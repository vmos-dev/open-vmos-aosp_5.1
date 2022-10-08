/*
** Copyright 2010 The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

/*
 * Micro-benchmarking of sleep/cpu speed/memcpy/memset/memory reads/strcmp.
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <sched.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>

// The default size of data that will be manipulated in each iteration of
// a memory benchmark. Can be modified with the --data_size option.
#define DEFAULT_DATA_SIZE       1000000000

// The amount of memory allocated for the cold benchmarks to use.
#define DEFAULT_COLD_DATA_SIZE  128*1024*1024

// The default size of the stride between each buffer for cold benchmarks.
#define DEFAULT_COLD_STRIDE_SIZE  4096

// Number of nanoseconds in a second.
#define NS_PER_SEC              1000000000

// The maximum number of arguments that a benchmark will accept.
#define MAX_ARGS    2

// Default memory alignment of malloc.
#define DEFAULT_MALLOC_MEMORY_ALIGNMENT   8

// Contains information about benchmark options.
typedef struct {
    bool print_average;
    bool print_each_iter;

    int dst_align;
    int dst_or_mask;
    int src_align;
    int src_or_mask;

    int cpu_to_lock;

    int data_size;
    int dst_str_size;
    int cold_data_size;
    int cold_stride_size;

    int args[MAX_ARGS];
    int num_args;
} command_data_t;

typedef void *(*void_func_t)();
typedef void *(*memcpy_func_t)(void *, const void *, size_t);
typedef void *(*memset_func_t)(void *, int, size_t);
typedef int (*strcmp_func_t)(const char *, const char *);
typedef char *(*str_func_t)(char *, const char *);
typedef size_t (*strlen_func_t)(const char *);

// Struct that contains a mapping of benchmark name to benchmark function.
typedef struct {
    const char *name;
    int (*ptr)(const char *, const command_data_t &, void_func_t func);
    void_func_t func;
} function_t;

// Get the current time in nanoseconds.
uint64_t nanoTime() {
  struct timespec t;

  t.tv_sec = t.tv_nsec = 0;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return static_cast<uint64_t>(t.tv_sec) * NS_PER_SEC + t.tv_nsec;
}

// Allocate memory with a specific alignment and return that pointer.
// This function assumes an alignment value that is a power of 2.
// If the alignment is 0, then use the pointer returned by malloc.
uint8_t *getAlignedMemory(uint8_t *orig_ptr, int alignment, int or_mask) {
  uint64_t ptr = reinterpret_cast<uint64_t>(orig_ptr);
  if (alignment > 0) {
      // When setting the alignment, set it to exactly the alignment chosen.
      // The pointer returned will be guaranteed not to be aligned to anything
      // more than that.
      ptr += alignment - (ptr & (alignment - 1));
      ptr |= alignment | or_mask;
  }

  return reinterpret_cast<uint8_t*>(ptr);
}

// Allocate memory with a specific alignment and return that pointer.
// This function assumes an alignment value that is a power of 2.
// If the alignment is 0, then use the pointer returned by malloc.
uint8_t *allocateAlignedMemory(size_t size, int alignment, int or_mask) {
  uint64_t ptr = reinterpret_cast<uint64_t>(malloc(size + 3 * alignment));
  if (!ptr)
      return NULL;
  return getAlignedMemory((uint8_t*)ptr, alignment, or_mask);
}

void initString(uint8_t *buf, size_t size) {
    for (size_t i = 0; i < size - 1; i++) {
        buf[i] = static_cast<char>(32 + (i % 96));
    }
    buf[size-1] = '\0';
}

static inline double computeAverage(uint64_t time_ns, size_t size, size_t copies) {
    return ((size/1024.0) * copies) / ((double)time_ns/NS_PER_SEC);
}

static inline double computeRunningAvg(double avg, double running_avg, size_t cur_idx) {
    return (running_avg / (cur_idx + 1)) * cur_idx + (avg / (cur_idx + 1));
}

static inline double computeRunningSquareAvg(double avg, double square_avg, size_t cur_idx) {
    return (square_avg / (cur_idx + 1)) * cur_idx + (avg / (cur_idx + 1)) * avg;
}

static inline double computeStdDev(double square_avg, double running_avg) {
    return sqrt(square_avg - running_avg * running_avg);
}

static inline void printIter(uint64_t time_ns, const char *name, size_t size, size_t copies, double avg) {
    printf("%s %zux%zu bytes took %.06f seconds (%f MB/s)\n",
           name, copies, size, (double)time_ns/NS_PER_SEC, avg/1024.0);
}

static inline void printSummary(uint64_t /*time_ns*/, const char *name, size_t size, size_t copies, double running_avg, double std_dev, double min, double max) {
    printf("  %s %zux%zu bytes average %.2f MB/s std dev %.4f min %.2f MB/s max %.2f MB/s\n",
           name, copies, size, running_avg/1024.0, std_dev/1024.0, min/1024.0,
           max/1024.0);
}

// For the cold benchmarks, a large buffer will be created which
// contains many "size" buffers. This function will figure out the increment
// needed between each buffer so that each one is aligned to "alignment".
int getAlignmentIncrement(size_t size, int alignment) {
    if (alignment == 0) {
        alignment = DEFAULT_MALLOC_MEMORY_ALIGNMENT;
    }
    alignment *= 2;
    return size + alignment - (size % alignment);
}

uint8_t *getColdBuffer(int num_buffers, size_t incr, int alignment, int or_mask) {
    uint8_t *buffers = reinterpret_cast<uint8_t*>(malloc(num_buffers * incr + 3 * alignment));
    if (!buffers) {
        return NULL;
    }
    return getAlignedMemory(buffers, alignment, or_mask);
}

static inline double computeColdAverage(uint64_t time_ns, size_t size, size_t copies, size_t num_buffers) {
    return ((size/1024.0) * copies * num_buffers) / ((double)time_ns/NS_PER_SEC);
}

static void inline printColdIter(uint64_t time_ns, const char *name, size_t size, size_t copies, size_t num_buffers, double avg) {
    printf("%s %zux%zux%zu bytes took %.06f seconds (%f MB/s)\n",
           name, copies, num_buffers, size, (double)time_ns/NS_PER_SEC, avg/1024.0);
}

static void inline printColdSummary(
        uint64_t /*time_ns*/, const char *name, size_t size, size_t copies, size_t num_buffers,
        double running_avg, double square_avg, double min, double max) {
    printf("  %s %zux%zux%zu bytes average %.2f MB/s std dev %.4f min %.2f MB/s max %.2f MB/s\n",
           name, copies, num_buffers, size, running_avg/1024.0,
           computeStdDev(running_avg, square_avg)/1024.0, min/1024.0, max/1024.0);
}

#define MAINLOOP(cmd_data, BENCH, COMPUTE_AVG, PRINT_ITER, PRINT_AVG) \
    uint64_t time_ns;                                                 \
    int iters = cmd_data.args[1];                                     \
    bool print_average = cmd_data.print_average;                      \
    bool print_each_iter = cmd_data.print_each_iter;                  \
    double min = 0.0, max = 0.0, running_avg = 0.0, square_avg = 0.0; \
    double avg;                                                       \
    for (int i = 0; iters == -1 || i < iters; i++) {                  \
        time_ns = nanoTime();                                         \
        BENCH;                                                        \
        time_ns = nanoTime() - time_ns;                               \
        avg = COMPUTE_AVG;                                            \
        if (print_average) {                                          \
            running_avg = computeRunningAvg(avg, running_avg, i);     \
            square_avg = computeRunningSquareAvg(avg, square_avg, i); \
            if (min == 0.0 || avg < min) {                            \
                min = avg;                                            \
            }                                                         \
            if (avg > max) {                                          \
                max = avg;                                            \
            }                                                         \
        }                                                             \
        if (print_each_iter) {                                        \
            PRINT_ITER;                                               \
        }                                                             \
    }                                                                 \
    if (print_average) {                                              \
        PRINT_AVG;                                                    \
    }

#define MAINLOOP_DATA(name, cmd_data, size, BENCH)                    \
    size_t copies = cmd_data.data_size/size;                          \
    size_t j;                                                         \
    MAINLOOP(cmd_data,                                                \
             for (j = 0; j < copies; j++) {                           \
                 BENCH;                                               \
             },                                                       \
             computeAverage(time_ns, size, copies),                   \
             printIter(time_ns, name, size, copies, avg),             \
             double std_dev = computeStdDev(square_avg, running_avg); \
             printSummary(time_ns, name, size, copies, running_avg,   \
                          std_dev, min, max));

#define MAINLOOP_COLD(name, cmd_data, size, num_incrs, BENCH)                 \
    size_t num_strides = num_buffers / num_incrs;                             \
    if ((num_buffers % num_incrs) != 0) {                                     \
        num_strides--;                                                        \
    }                                                                         \
    size_t copies = 1;                                                        \
    num_buffers = num_incrs * num_strides;                                    \
    if (num_buffers * size < static_cast<size_t>(cmd_data.data_size)) {       \
        copies = cmd_data.data_size / (num_buffers * size);                   \
    }                                                                         \
    if (num_strides == 0) {                                                   \
        printf("%s: Chosen options lead to no copies, aborting.\n", name);    \
        return -1;                                                            \
    }                                                                         \
    size_t j, k;                                                              \
    MAINLOOP(cmd_data,                                                        \
             for (j = 0; j < copies; j++) {                                   \
                 for (k = 0; k < num_incrs; k++) {                            \
                     BENCH;                                                   \
                }                                                             \
            },                                                                \
            computeColdAverage(time_ns, size, copies, num_buffers),           \
            printColdIter(time_ns, name, size, copies, num_buffers, avg),     \
            printColdSummary(time_ns, name, size, copies, num_buffers,        \
                             running_avg, square_avg, min, max));

// This version of the macro creates a single buffer of the given size and
// alignment. The variable "buf" will be a pointer to the buffer and should
// be used by the BENCH code.
// INIT - Any specialized code needed to initialize the data. This will only
//        be executed once.
// BENCH - The actual code to benchmark and is timed.
#define BENCH_ONE_BUF(name, cmd_data, INIT, BENCH)                            \
    size_t size = cmd_data.args[0]; \
    uint8_t *buf = allocateAlignedMemory(size, cmd_data.dst_align, cmd_data.dst_or_mask); \
    if (!buf)                                                                 \
        return -1;                                                            \
    INIT;                                                                     \
    MAINLOOP_DATA(name, cmd_data, size, BENCH);

// This version of the macro creates two buffers of the given sizes and
// alignments. The variables "buf1" and "buf2" will be pointers to the
// buffers and should be used by the BENCH code.
// INIT - Any specialized code needed to initialize the data. This will only
//        be executed once.
// BENCH - The actual code to benchmark and is timed.
#define BENCH_TWO_BUFS(name, cmd_data, INIT, BENCH)                           \
    size_t size = cmd_data.args[0];                                           \
    uint8_t *buf1 = allocateAlignedMemory(size, cmd_data.src_align, cmd_data.src_or_mask); \
    if (!buf1)                                                                \
        return -1;                                                            \
    size_t total_size = size;                                                 \
    if (cmd_data.dst_str_size > 0)                                            \
        total_size += cmd_data.dst_str_size;                                  \
    uint8_t *buf2 = allocateAlignedMemory(total_size, cmd_data.dst_align, cmd_data.dst_or_mask); \
    if (!buf2)                                                                \
        return -1;                                                            \
    INIT;                                                                     \
    MAINLOOP_DATA(name, cmd_data, size, BENCH);

// This version of the macro attempts to benchmark code when the data
// being manipulated is not in the cache, thus the cache is cold. It does
// this by creating a single large buffer that is designed to be larger than
// the largest cache in the system. The variable "buf" will be one slice
// of the buffer that the BENCH code should use that is of the correct size
// and alignment. In order to avoid any algorithms that prefetch past the end
// of their "buf" and into the next sequential buffer, the code strides
// through the buffer. Specifically, as "buf" values are iterated in BENCH
// code, the end of "buf" is guaranteed to be at least "stride_size" away
// from the next "buf".
// INIT - Any specialized code needed to initialize the data. This will only
//        be executed once.
// BENCH - The actual code to benchmark and is timed.
#define COLD_ONE_BUF(name, cmd_data, INIT, BENCH)                             \
    size_t size = cmd_data.args[0];                                           \
    size_t incr = getAlignmentIncrement(size, cmd_data.dst_align);            \
    size_t num_buffers = cmd_data.cold_data_size / incr;                      \
    size_t buffer_size = num_buffers * incr;                                  \
    uint8_t *buffer = getColdBuffer(num_buffers, incr, cmd_data.dst_align, cmd_data.dst_or_mask); \
    if (!buffer)                                                              \
        return -1;                                                            \
    size_t num_incrs = cmd_data.cold_stride_size / incr + 1;                  \
    size_t stride_incr = incr * num_incrs;                                    \
    uint8_t *buf;                                                             \
    size_t l;                                                                 \
    INIT;                                                                     \
    MAINLOOP_COLD(name, cmd_data, size, num_incrs,                            \
                  buf = buffer + k * incr;                                    \
                  for (l = 0; l < num_strides; l++) {                         \
                      BENCH;                                                  \
                      buf += stride_incr;                                     \
                  });

// This version of the macro attempts to benchmark code when the data
// being manipulated is not in the cache, thus the cache is cold. It does
// this by creating two large buffers each of which is designed to be
// larger than the largest cache in the system. Two variables "buf1" and
// "buf2" will be the two buffers that BENCH code should use. In order
// to avoid any algorithms that prefetch past the end of either "buf1"
// or "buf2" and into the next sequential buffer, the code strides through
// both buffers. Specifically, as "buf1" and "buf2" values are iterated in
// BENCH code, the end of "buf1" and "buf2" is guaranteed to be at least
// "stride_size" away from the next "buf1" and "buf2".
// INIT - Any specialized code needed to initialize the data. This will only
//        be executed once.
// BENCH - The actual code to benchmark and is timed.
#define COLD_TWO_BUFS(name, cmd_data, INIT, BENCH)                            \
    size_t size = cmd_data.args[0];                                           \
    size_t buf1_incr = getAlignmentIncrement(size, cmd_data.src_align);       \
    size_t total_size = size;                                                 \
    if (cmd_data.dst_str_size > 0)                                            \
        total_size += cmd_data.dst_str_size;                                  \
    size_t buf2_incr = getAlignmentIncrement(total_size, cmd_data.dst_align); \
    size_t max_incr = (buf1_incr > buf2_incr) ? buf1_incr : buf2_incr;        \
    size_t num_buffers = cmd_data.cold_data_size / max_incr;                  \
    size_t buffer1_size = num_buffers * buf1_incr;                            \
    size_t buffer2_size = num_buffers * buf2_incr;                            \
    uint8_t *buffer1 = getColdBuffer(num_buffers, buf1_incr, cmd_data.src_align, cmd_data.src_or_mask); \
    if (!buffer1)                                                             \
        return -1;                                                            \
    uint8_t *buffer2 = getColdBuffer(num_buffers, buf2_incr, cmd_data.dst_align, cmd_data.dst_or_mask); \
    if (!buffer2)                                                             \
        return -1;                                                            \
    size_t min_incr = (buf1_incr < buf2_incr) ? buf1_incr : buf2_incr;        \
    size_t num_incrs = cmd_data.cold_stride_size / min_incr + 1;              \
    size_t buf1_stride_incr = buf1_incr * num_incrs;                          \
    size_t buf2_stride_incr = buf2_incr * num_incrs;                          \
    size_t l;                                                                 \
    uint8_t *buf1;                                                            \
    uint8_t *buf2;                                                            \
    INIT;                                                                     \
    MAINLOOP_COLD(name, cmd_data, size, num_incrs,                            \
                  buf1 = buffer1 + k * buf1_incr;                             \
                  buf2 = buffer2 + k * buf2_incr;                             \
                  for (l = 0; l < num_strides; l++) {                         \
                      BENCH;                                                  \
                      buf1 += buf1_stride_incr;                               \
                      buf2 += buf2_stride_incr;                               \
                  });

int benchmarkSleep(const char* /*name*/, const command_data_t &cmd_data, void_func_t /*func*/) {
    int delay = cmd_data.args[0];
    MAINLOOP(cmd_data, sleep(delay),
             (double)time_ns/NS_PER_SEC,
             printf("sleep(%d) took %.06f seconds\n", delay, avg);,
             printf("  sleep(%d) average %.06f seconds std dev %f min %.06f seconds max %0.6f seconds\n", \
                    delay, running_avg, computeStdDev(square_avg, running_avg), \
                    min, max));

    return 0;
}

int benchmarkCpu(const char* /*name*/, const command_data_t &cmd_data, void_func_t /*func*/) {
    // Use volatile so that the loop is not optimized away by the compiler.
    volatile int cpu_foo;

    MAINLOOP(cmd_data,
             for (cpu_foo = 0; cpu_foo < 100000000; cpu_foo++),
             (double)time_ns/NS_PER_SEC,
             printf("cpu took %.06f seconds\n", avg),
             printf("  cpu average %.06f seconds std dev %f min %0.6f seconds max %0.6f seconds\n", \
                    running_avg, computeStdDev(square_avg, running_avg), min, max));

    return 0;
}

int benchmarkMemset(const char *name, const command_data_t &cmd_data, void_func_t func) {
    memset_func_t memset_func = reinterpret_cast<memset_func_t>(func);
    BENCH_ONE_BUF(name, cmd_data, ;, memset_func(buf, i, size));

    return 0;
}

int benchmarkMemsetCold(const char *name, const command_data_t &cmd_data, void_func_t func) {
    memset_func_t memset_func = reinterpret_cast<memset_func_t>(func);
    COLD_ONE_BUF(name, cmd_data, ;, memset_func(buf, l, size));

    return 0;
}

int benchmarkMemcpy(const char *name, const command_data_t &cmd_data, void_func_t func) {
    memcpy_func_t memcpy_func = reinterpret_cast<memcpy_func_t>(func);

    BENCH_TWO_BUFS(name, cmd_data,
                   memset(buf1, 0xff, size); \
                   memset(buf2, 0, size),
                   memcpy_func(buf2, buf1, size));

    return 0;
}

int benchmarkMemcpyCold(const char *name, const command_data_t &cmd_data, void_func_t func) {
    memcpy_func_t memcpy_func = reinterpret_cast<memcpy_func_t>(func);

    COLD_TWO_BUFS(name, cmd_data,
                  memset(buffer1, 0xff, buffer1_size); \
                  memset(buffer2, 0x0, buffer2_size),
                  memcpy_func(buf2, buf1, size));

    return 0;
}

int benchmarkMemread(const char *name, const command_data_t &cmd_data, void_func_t /*func*/) {
    int size = cmd_data.args[0];

    uint32_t *src = reinterpret_cast<uint32_t*>(malloc(size));
    if (!src)
        return -1;
    memset(src, 0xff, size);

    // Use volatile so the compiler does not optimize away the reads.
    volatile int foo;
    size_t k;
    MAINLOOP_DATA(name, cmd_data, size,
                  for (k = 0; k < size/sizeof(uint32_t); k++) foo = src[k]);

    return 0;
}

int benchmarkStrcmp(const char *name, const command_data_t &cmd_data, void_func_t func) {
    strcmp_func_t strcmp_func = reinterpret_cast<strcmp_func_t>(func);

    int retval;
    BENCH_TWO_BUFS(name, cmd_data,
                   initString(buf1, size); \
                   initString(buf2, size),
                   retval = strcmp_func(reinterpret_cast<char*>(buf1), reinterpret_cast<char*>(buf2)); \
                   if (retval != 0) printf("%s failed, return value %d\n", name, retval));

    return 0;
}

int benchmarkStrcmpCold(const char *name, const command_data_t &cmd_data, void_func_t func) {
    strcmp_func_t strcmp_func = reinterpret_cast<strcmp_func_t>(func);

    int retval;
    COLD_TWO_BUFS(name, cmd_data,
                  memset(buffer1, 'a', buffer1_size); \
                  memset(buffer2, 'a', buffer2_size); \
                  for (size_t i =0; i < num_buffers; i++) { \
                      buffer1[size-1+buf1_incr*i] = '\0'; \
                      buffer2[size-1+buf2_incr*i] = '\0'; \
                  },
                  retval = strcmp_func(reinterpret_cast<char*>(buf1), reinterpret_cast<char*>(buf2)); \
                  if (retval != 0) printf("%s failed, return value %d\n", name, retval));

    return 0;
}

int benchmarkStrlen(const char *name, const command_data_t &cmd_data, void_func_t func) {
    size_t real_size;
    strlen_func_t strlen_func = reinterpret_cast<strlen_func_t>(func);
    BENCH_ONE_BUF(name, cmd_data,
                  initString(buf, size),
                  real_size = strlen_func(reinterpret_cast<char*>(buf)); \
                  if (real_size + 1 != size) { \
                      printf("%s failed, expected %zu, got %zu\n", name, size, real_size); \
                      return -1; \
                  });

    return 0;
}

int benchmarkStrlenCold(const char *name, const command_data_t &cmd_data, void_func_t func) {
    strlen_func_t strlen_func = reinterpret_cast<strlen_func_t>(func);
    size_t real_size;
    COLD_ONE_BUF(name, cmd_data,
                 memset(buffer, 'a', buffer_size); \
                 for (size_t i = 0; i < num_buffers; i++) { \
                     buffer[size-1+incr*i] = '\0'; \
                 },
                 real_size = strlen_func(reinterpret_cast<char*>(buf)); \
                 if (real_size + 1 != size) { \
                     printf("%s failed, expected %zu, got %zu\n", name, size, real_size); \
                     return -1; \
                 });
    return 0;
}

int benchmarkStrcat(const char *name, const command_data_t &cmd_data, void_func_t func) {
    str_func_t str_func = reinterpret_cast<str_func_t>(func);

    int dst_str_size = cmd_data.dst_str_size;
    if (dst_str_size <= 0) {
        printf("%s requires --dst_str_size to be set to a non-zero value.\n",
               name);
        return -1;
    }
    BENCH_TWO_BUFS(name, cmd_data,
                   initString(buf1, size); \
                   initString(buf2, dst_str_size),
                   str_func(reinterpret_cast<char*>(buf2), reinterpret_cast<char*>(buf1)); buf2[dst_str_size-1] = '\0');

    return 0;
}

int benchmarkStrcatCold(const char *name, const command_data_t &cmd_data, void_func_t func) {
    str_func_t str_func = reinterpret_cast<str_func_t>(func);

    int dst_str_size = cmd_data.dst_str_size;
    if (dst_str_size <= 0) {
        printf("%s requires --dst_str_size to be set to a non-zero value.\n",
               name);
        return -1;
    }
    COLD_TWO_BUFS(name, cmd_data,
                  memset(buffer1, 'a', buffer1_size); \
                  memset(buffer2, 'b', buffer2_size); \
                  for (size_t i = 0; i < num_buffers; i++) { \
                      buffer1[size-1+buf1_incr*i] = '\0'; \
                      buffer2[dst_str_size-1+buf2_incr*i] = '\0'; \
                  },
                  str_func(reinterpret_cast<char*>(buf2), reinterpret_cast<char*>(buf1)); buf2[dst_str_size-1] = '\0');

    return 0;
}


int benchmarkStrcpy(const char *name, const command_data_t &cmd_data, void_func_t func) {
    str_func_t str_func = reinterpret_cast<str_func_t>(func);

    BENCH_TWO_BUFS(name, cmd_data,
                   initString(buf1, size); \
                   memset(buf2, 0, size),
                   str_func(reinterpret_cast<char*>(buf2), reinterpret_cast<char*>(buf1)));

    return 0;
}

int benchmarkStrcpyCold(const char *name, const command_data_t &cmd_data, void_func_t func) {
    str_func_t str_func = reinterpret_cast<str_func_t>(func);

    COLD_TWO_BUFS(name, cmd_data,
                  memset(buffer1, 'a', buffer1_size); \
                  for (size_t i = 0; i < num_buffers; i++) { \
                     buffer1[size-1+buf1_incr*i] = '\0'; \
                  } \
                  memset(buffer2, 0, buffer2_size),
                  str_func(reinterpret_cast<char*>(buf2), reinterpret_cast<char*>(buf1)));

    return 0;
}

// Create the mapping structure.
function_t function_table[] = {
    { "cpu", benchmarkCpu, NULL },
    { "memcpy", benchmarkMemcpy, reinterpret_cast<void_func_t>(memcpy) },
    { "memcpy_cold", benchmarkMemcpyCold, reinterpret_cast<void_func_t>(memcpy) },
    { "memread", benchmarkMemread, NULL },
    { "memset", benchmarkMemset, reinterpret_cast<void_func_t>(memset) },
    { "memset_cold", benchmarkMemsetCold, reinterpret_cast<void_func_t>(memset) },
    { "sleep", benchmarkSleep, NULL },
    { "strcat", benchmarkStrcat, reinterpret_cast<void_func_t>(strcat) },
    { "strcat_cold", benchmarkStrcatCold, reinterpret_cast<void_func_t>(strcat) },
    { "strcmp", benchmarkStrcmp, reinterpret_cast<void_func_t>(strcmp) },
    { "strcmp_cold", benchmarkStrcmpCold, reinterpret_cast<void_func_t>(strcmp) },
    { "strcpy", benchmarkStrcpy, reinterpret_cast<void_func_t>(strcpy) },
    { "strcpy_cold", benchmarkStrcpyCold, reinterpret_cast<void_func_t>(strcpy) },
    { "strlen", benchmarkStrlen, reinterpret_cast<void_func_t>(strlen) },
    { "strlen_cold", benchmarkStrlenCold, reinterpret_cast<void_func_t>(strlen) },
};

void usage() {
    printf("Usage:\n");
    printf("  micro_bench [--data_size DATA_BYTES] [--print_average]\n");
    printf("              [--no_print_each_iter] [--lock_to_cpu CORE]\n");
    printf("              [--src_align ALIGN] [--src_or_mask OR_MASK]\n");
    printf("              [--dst_align ALIGN] [--dst_or_mask OR_MASK]\n");
    printf("              [--dst_str_size SIZE] [--cold_data_size DATA_BYTES]\n");
    printf("              [--cold_stride_size SIZE]\n");
    printf("    --data_size DATA_BYTES\n");
    printf("      For the data benchmarks (memcpy/memset/memread) the approximate\n");
    printf("      size of data, in bytes, that will be manipulated in each iteration.\n");
    printf("    --print_average\n");
    printf("      Print the average and standard deviation of all iterations.\n");
    printf("    --no_print_each_iter\n");
    printf("      Do not print any values in each iteration.\n");
    printf("    --lock_to_cpu CORE\n");
    printf("      Lock to the specified CORE. The default is to use the last core found.\n");
    printf("    --dst_align ALIGN\n");
    printf("      If the command supports it, align the destination pointer to ALIGN.\n");
    printf("      The default is to use the value returned by malloc.\n");
    printf("    --dst_or_mask OR_MASK\n");
    printf("      If the command supports it, or in the OR_MASK on to the destination pointer.\n");
    printf("      The OR_MASK must be smaller than the dst_align value.\n");
    printf("      The default value is 0.\n");

    printf("    --src_align ALIGN\n");
    printf("      If the command supports it, align the source pointer to ALIGN. The default is to use the\n");
    printf("      value returned by malloc.\n");
    printf("    --src_or_mask OR_MASK\n");
    printf("      If the command supports it, or in the OR_MASK on to the source pointer.\n");
    printf("      The OR_MASK must be smaller than the src_align value.\n");
    printf("      The default value is 0.\n");
    printf("    --dst_str_size SIZE\n");
    printf("      If the command supports it, create a destination string of this length.\n");
    printf("      The default is to not update the destination string.\n");
    printf("    --cold_data_size DATA_SIZE\n");
    printf("      For _cold benchmarks, use this as the total amount of memory to use.\n");
    printf("      The default is 128MB, and the number should be larger than the cache on the chip.\n");
    printf("      This value is specified in bytes.\n");
    printf("    --cold_stride_size SIZE\n");
    printf("      For _cold benchmarks, use this as the minimum stride between iterations.\n");
    printf("      The default is 4096 bytes and the number should be larger than the amount of data\n");
    printf("      pulled in to the cache by each run of the benchmark.\n");
    printf("    ITERS\n");
    printf("      The number of iterations to execute each benchmark. If not\n");
    printf("      passed in then run forever.\n");
    printf("  micro_bench cpu UNUSED [ITERS]\n");
    printf("  micro_bench [--dst_align ALIGN] [--dst_or_mask OR_MASK] memcpy NUM_BYTES [ITERS]\n");
    printf("  micro_bench memread NUM_BYTES [ITERS]\n");
    printf("  micro_bench [--dst_align ALIGN] [--dst_or_mask OR_MASK] memset NUM_BYTES [ITERS]\n");
    printf("  micro_bench sleep TIME_TO_SLEEP [ITERS]\n");
    printf("    TIME_TO_SLEEP\n");
    printf("      The time in seconds to sleep.\n");
    printf("  micro_bench [--src_align ALIGN] [--src_or_mask OR_MASK] [--dst_align ALIGN] [--dst_or_mask] [--dst_str_size SIZE] strcat NUM_BYTES [ITERS]\n");
    printf("  micro_bench [--src_align ALIGN] [--src_or_mask OR_MASK] [--dst_align ALIGN] [--dst_or_mask OR_MASK] strcmp NUM_BYTES [ITERS]\n");
    printf("  micro_bench [--src_align ALIGN] [--src_or_mask OR_MASK] [--dst_align ALIGN] [--dst_or_mask] strcpy NUM_BYTES [ITERS]\n");
    printf("  micro_bench [--dst_align ALIGN] [--dst_or_mask OR_MASK] strlen NUM_BYTES [ITERS]\n");
    printf("\n");
    printf("  In addition, memcpy/memcpy/memset/strcat/strcpy/strlen have _cold versions\n");
    printf("  that will execute the function on a buffer not in the cache.\n");
}

function_t *processOptions(int argc, char **argv, command_data_t *cmd_data) {
    function_t *command = NULL;

    // Initialize the command_flags.
    cmd_data->print_average = false;
    cmd_data->print_each_iter = true;
    cmd_data->dst_align = 0;
    cmd_data->src_align = 0;
    cmd_data->src_or_mask = 0;
    cmd_data->dst_or_mask = 0;
    cmd_data->num_args = 0;
    cmd_data->cpu_to_lock = -1;
    cmd_data->data_size = DEFAULT_DATA_SIZE;
    cmd_data->dst_str_size = -1;
    cmd_data->cold_data_size = DEFAULT_COLD_DATA_SIZE;
    cmd_data->cold_stride_size = DEFAULT_COLD_STRIDE_SIZE;
    for (int i = 0; i < MAX_ARGS; i++) {
        cmd_data->args[i] = -1;
    }

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            int *save_value = NULL;
            if (strcmp(argv[i], "--print_average") == 0) {
                cmd_data->print_average = true;
            } else if (strcmp(argv[i], "--no_print_each_iter") == 0) {
                cmd_data->print_each_iter = false;
            } else if (strcmp(argv[i], "--dst_align") == 0) {
                save_value = &cmd_data->dst_align;
            } else if (strcmp(argv[i], "--src_align") == 0) {
                save_value = &cmd_data->src_align;
            } else if (strcmp(argv[i], "--dst_or_mask") == 0) {
                save_value = &cmd_data->dst_or_mask;
            } else if (strcmp(argv[i], "--src_or_mask") == 0) {
                save_value = &cmd_data->src_or_mask;
            } else if (strcmp(argv[i], "--lock_to_cpu") == 0) {
                save_value = &cmd_data->cpu_to_lock;
            } else if (strcmp(argv[i], "--data_size") == 0) {
                save_value = &cmd_data->data_size;
            } else if (strcmp(argv[i], "--dst_str_size") == 0) {
                save_value = &cmd_data->dst_str_size;
            } else if (strcmp(argv[i], "--cold_data_size") == 0) {
                save_value = &cmd_data->cold_data_size;
            } else if (strcmp(argv[i], "--cold_stride_size") == 0) {
                save_value = &cmd_data->cold_stride_size;
            } else {
                printf("Unknown option %s\n", argv[i]);
                return NULL;
            }
            if (save_value) {
                // Checking both characters without a strlen() call should be
                // safe since as long as the argument exists, one character will
                // be present (\0). And if the first character is '-', then
                // there will always be a second character (\0 again).
                if (i == argc - 1 || (argv[i + 1][0] == '-' && !isdigit(argv[i + 1][1]))) {
                    printf("The option %s requires one argument.\n",
                           argv[i]);
                    return NULL;
                }
                *save_value = (int)strtol(argv[++i], NULL, 0);
            }
        } else if (!command) {
            for (size_t j = 0; j < sizeof(function_table)/sizeof(function_t); j++) {
                if (strcmp(argv[i], function_table[j].name) == 0) {
                    command = &function_table[j];
                    break;
                }
            }
            if (!command) {
                printf("Uknown command %s\n", argv[i]);
                return NULL;
            }
        } else if (cmd_data->num_args > MAX_ARGS) {
            printf("More than %d number arguments passed in.\n", MAX_ARGS);
            return NULL;
        } else {
            cmd_data->args[cmd_data->num_args++] = atoi(argv[i]);
        }
    }

    // Check the arguments passed in make sense.
    if (cmd_data->num_args != 1 && cmd_data->num_args != 2) {
        printf("Not enough arguments passed in.\n");
        return NULL;
    } else if (cmd_data->dst_align < 0) {
        printf("The --dst_align option must be greater than or equal to 0.\n");
        return NULL;
    } else if (cmd_data->src_align < 0) {
        printf("The --src_align option must be greater than or equal to 0.\n");
        return NULL;
    } else if (cmd_data->data_size <= 0) {
        printf("The --data_size option must be a positive number.\n");
        return NULL;
    } else if ((cmd_data->dst_align & (cmd_data->dst_align - 1))) {
        printf("The --dst_align option must be a power of 2.\n");
        return NULL;
    } else if ((cmd_data->src_align & (cmd_data->src_align - 1))) {
        printf("The --src_align option must be a power of 2.\n");
        return NULL;
    } else if (!cmd_data->src_align && cmd_data->src_or_mask) {
        printf("The --src_or_mask option requires that --src_align be set.\n");
        return NULL;
    } else if (!cmd_data->dst_align && cmd_data->dst_or_mask) {
        printf("The --dst_or_mask option requires that --dst_align be set.\n");
        return NULL;
    } else if (cmd_data->src_or_mask > cmd_data->src_align) {
        printf("The value of --src_or_mask cannot be larger that --src_align.\n");
        return NULL;
    } else if (cmd_data->dst_or_mask > cmd_data->dst_align) {
        printf("The value of --src_or_mask cannot be larger that --src_align.\n");
        return NULL;
    }

    return command;
}

bool raisePriorityAndLock(int cpu_to_lock) {
    cpu_set_t cpuset;

    if (setpriority(PRIO_PROCESS, 0, -20)) {
        perror("Unable to raise priority of process.\n");
        return false;
    }

    CPU_ZERO(&cpuset);
    if (sched_getaffinity(0, sizeof(cpuset), &cpuset) != 0) {
        perror("sched_getaffinity failed");
        return false;
    }

    if (cpu_to_lock < 0) {
        // Lock to the last active core we find.
        for (int i = 0; i < CPU_SETSIZE; i++) {
            if (CPU_ISSET(i, &cpuset)) {
                cpu_to_lock = i;
            }
        }
    } else if (!CPU_ISSET(cpu_to_lock, &cpuset)) {
        printf("Cpu %d does not exist.\n", cpu_to_lock);
        return false;
    }

    if (cpu_to_lock < 0) {
        printf("Cannot find any valid cpu to lock.\n");
        return false;
    }

    CPU_ZERO(&cpuset);
    CPU_SET(cpu_to_lock, &cpuset);
    if (sched_setaffinity(0, sizeof(cpuset), &cpuset) != 0) {
        perror("sched_setaffinity failed");
        return false;
    }

    return true;
}

int main(int argc, char **argv) {
    command_data_t cmd_data;

    function_t *command = processOptions(argc, argv, &cmd_data);
    if (!command) {
      usage();
      return -1;
    }

    if (!raisePriorityAndLock(cmd_data.cpu_to_lock)) {
      return -1;
    }

    printf("%s\n", command->name);
    return (*command->ptr)(command->name, cmd_data, command->func);
}
