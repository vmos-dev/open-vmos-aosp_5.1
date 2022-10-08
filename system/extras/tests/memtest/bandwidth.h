/*
 * Copyright (C) 2013 The Android Open Source Project
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

#ifndef __BANDWIDTH_H__
#define __BANDWIDTH_H__

#include "memtest.h"

// Bandwidth Class definitions.
class BandwidthBenchmark {
public:
    BandwidthBenchmark()
        : _size(0),
          _num_warm_loops(DEFAULT_NUM_WARM_LOOPS),
          _num_loops(DEFAULT_NUM_LOOPS) {}
    virtual ~BandwidthBenchmark() {}

    bool run() {
        if (_size == 0) {
            return false;
        }
        if (!canRun()) {
            return false;
        }

        bench(_num_warm_loops);

        nsecs_t t = system_time();
        bench(_num_loops);
        t = system_time() - t;

        _mb_per_sec = (_size*(_num_loops/_BYTES_PER_MB))/(t/_NUM_NS_PER_SEC);

        return true;
    }

    bool canRun() { return !usesNeon() || isNeonSupported(); }

    virtual bool setSize(size_t size) = 0;

    virtual const char *getName() = 0;

    virtual bool verify() = 0;

    virtual bool usesNeon() { return false; }

    bool isNeonSupported() {
#if defined(__ARM_NEON__)
        return true;
#else
        return false;
#endif
    }

    // Accessors/mutators.
    double mb_per_sec() { return _mb_per_sec; }
    size_t num_warm_loops() { return _num_warm_loops; }
    size_t num_loops() { return _num_loops; }
    size_t size() { return _size; }

    void set_num_warm_loops(size_t num_warm_loops) {
        _num_warm_loops = num_warm_loops;
    }
    void set_num_loops(size_t num_loops) { _num_loops = num_loops; }

    // Static constants
    static const unsigned int DEFAULT_NUM_WARM_LOOPS = 1000000;
    static const unsigned int DEFAULT_NUM_LOOPS = 20000000;

protected:
    virtual void bench(size_t num_loops) = 0;

    double _mb_per_sec;
    size_t _size;
    size_t _num_warm_loops;
    size_t _num_loops;

private:
    // Static constants
    static const double _NUM_NS_PER_SEC = 1000000000.0;
    static const double _BYTES_PER_MB = 1024.0* 1024.0;
};

class CopyBandwidthBenchmark : public BandwidthBenchmark {
public:
    CopyBandwidthBenchmark() : BandwidthBenchmark(), _src(NULL), _dst(NULL) { }

    bool setSize(size_t size) {
        if (_src) {
           free(_src);
        }
        if (_dst) {
            free(_dst);
        }

        if (size == 0) {
            _size = DEFAULT_COPY_SIZE;
        } else {
            _size = size;
        }

        _src = reinterpret_cast<char*>(memalign(64, _size));
        if (!_src) {
            perror("Failed to allocate memory for test.");
            return false;
        }
        _dst = reinterpret_cast<char*>(memalign(64, _size));
        if (!_dst) {
            perror("Failed to allocate memory for test.");
            return false;
        }

        return true;
    }
    virtual ~CopyBandwidthBenchmark() {
        if (_src) {
            free(_src);
            _src = NULL;
        }
        if (_dst) {
            free(_dst);
            _dst = NULL;
        }
    }

    bool verify() {
        memset(_src, 0x23, _size);
        memset(_dst, 0, _size);
        bench(1);
        if (memcmp(_src, _dst, _size) != 0) {
            printf("Buffers failed to compare after one loop.\n");
            return false;
        }

        memset(_src, 0x23, _size);
        memset(_dst, 0, _size);
        _num_loops = 2;
        bench(2);
        if (memcmp(_src, _dst, _size) != 0) {
            printf("Buffers failed to compare after two loops.\n");
            return false;
        }

        return true;
    }

protected:
    char *_src;
    char *_dst;

    static const unsigned int DEFAULT_COPY_SIZE = 8000;
};

class CopyLdrdStrdBenchmark : public CopyBandwidthBenchmark {
public:
    CopyLdrdStrdBenchmark() : CopyBandwidthBenchmark() { }
    virtual ~CopyLdrdStrdBenchmark() {}

    const char *getName() { return "ldrd/strd"; }

protected:
    // Copy using ldrd/strd instructions.
    void bench(size_t num_loops) {
        asm volatile(
            "stmfd sp!, {r0,r1,r2,r3,r4,r6,r7}\n"

            "mov r0, %0\n"
            "mov r1, %1\n"
            "mov r2, %2\n"
            "mov r3, %3\n"

            "0:\n"
            "mov r4, r2, lsr #6\n"

            "1:\n"
            "ldrd r6, r7, [r0]\n"
            "strd r6, r7, [r1]\n"
            "ldrd r6, r7, [r0, #8]\n"
            "strd r6, r7, [r1, #8]\n"
            "ldrd r6, r7, [r0, #16]\n"
            "strd r6, r7, [r1, #16]\n"
            "ldrd r6, r7, [r0, #24]\n"
            "strd r6, r7, [r1, #24]\n"
            "ldrd r6, r7, [r0, #32]\n"
            "strd r6, r7, [r1, #32]\n"
            "ldrd r6, r7, [r0, #40]\n"
            "strd r6, r7, [r1, #40]\n"
            "ldrd r6, r7, [r0, #48]\n"
            "strd r6, r7, [r1, #48]\n"
            "ldrd r6, r7, [r0, #56]\n"
            "strd r6, r7, [r1, #56]\n"

            "add  r0, r0, #64\n"
            "add  r1, r1, #64\n"
            "subs r4, r4, #1\n"
            "bgt 1b\n"

            "sub r0, r0, r2\n"
            "sub r1, r1, r2\n"
            "subs r3, r3, #1\n"
            "bgt 0b\n"

            "ldmfd sp!, {r0,r1,r2,r3,r4,r6,r7}\n"
        :: "r" (_src), "r" (_dst), "r" (_size), "r" (num_loops) : "r0", "r1", "r2", "r3");
    }
};

class CopyLdmiaStmiaBenchmark : public CopyBandwidthBenchmark {
public:
    CopyLdmiaStmiaBenchmark() : CopyBandwidthBenchmark() { }
    virtual ~CopyLdmiaStmiaBenchmark() {}

    const char *getName() { return "ldmia/stmia"; }

protected:
    // Copy using ldmia/stmia instructions.
    void bench(size_t num_loops) {
        asm volatile(
            "stmfd sp!, {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12}\n"

            "mov r0, %0\n"
            "mov r1, %1\n"
            "mov r2, %2\n"
            "mov r3, %3\n"

            "0:\n"
            "mov r4, r2, lsr #6\n"

            "1:\n"
            "ldmia r0!, {r5, r6, r7, r8, r9, r10, r11, r12}\n"
            "stmia r1!, {r5, r6, r7, r8, r9, r10, r11, r12}\n"
            "subs r4, r4, #1\n"
            "ldmia r0!, {r5, r6, r7, r8, r9, r10, r11, r12}\n"
            "stmia r1!, {r5, r6, r7, r8, r9, r10, r11, r12}\n"
            "bgt 1b\n"

            "sub r0, r0, r2\n"
            "sub r1, r1, r2\n"
            "subs r3, r3, #1\n"
            "bgt 0b\n"

            "ldmfd sp!, {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11,r12}\n"
        :: "r" (_src), "r" (_dst), "r" (_size), "r" (num_loops) : "r0", "r1", "r2", "r3");
    }
};

class CopyVld1Vst1Benchmark : public CopyBandwidthBenchmark {
public:
    CopyVld1Vst1Benchmark() : CopyBandwidthBenchmark() { }
    virtual ~CopyVld1Vst1Benchmark() {}

    const char *getName() { return "vld1/vst1"; }

    bool usesNeon() { return true; }

protected:
    // Copy using vld1/vst1 instructions.
    void bench(size_t num_loops) {
#if defined(__ARM_NEON__)
        asm volatile(
            "stmfd sp!, {r0,r1,r2,r3,r4}\n"

            "mov r0, %0\n"
            "mov r1, %1\n"
            "mov r2, %2\n"
            "mov r3, %3\n"

            "0:\n"
            "mov r4, r2, lsr #6\n"

            "1:\n"
            "vld1.8 {d0-d3}, [r0]!\n"
            "vld1.8 {d4-d7}, [r0]!\n"
            "subs r4, r4, #1\n"
            "vst1.8 {d0-d3}, [r1:128]!\n"
            "vst1.8 {d4-d7}, [r1:128]!\n"
            "bgt 1b\n"

            "sub r0, r0, r2\n"
            "sub r1, r1, r2\n"
            "subs r3, r3, #1\n"
            "bgt 0b\n"

            "ldmfd sp!, {r0,r1,r2,r3,r4}\n"
        :: "r" (_src), "r" (_dst), "r" (_size), "r" (num_loops) : "r0", "r1", "r2", "r3");
#endif
    }
};

class CopyVldrVstrBenchmark : public CopyBandwidthBenchmark {
public:
    CopyVldrVstrBenchmark() : CopyBandwidthBenchmark() { }
    virtual ~CopyVldrVstrBenchmark() {}

    const char *getName() { return "vldr/vstr"; }

    bool usesNeon() { return true; }

protected:
    // Copy using vldr/vstr instructions.
    void bench(size_t num_loops) {
#if defined(__ARM_NEON__)
        asm volatile(
            "stmfd sp!, {r0,r1,r2,r3,r4}\n"

            "mov r0, %0\n"
            "mov r1, %1\n"
            "mov r2, %2\n"
            "mov r3, %3\n"

            "0:\n"
            "mov r4, r2, lsr #6\n"

            "1:\n"
            "vldr d0, [r0, #0]\n"
            "subs r4, r4, #1\n"
            "vldr d1, [r0, #8]\n"
            "vstr d0, [r1, #0]\n"
            "vldr d0, [r0, #16]\n"
            "vstr d1, [r1, #8]\n"
            "vldr d1, [r0, #24]\n"
            "vstr d0, [r1, #16]\n"
            "vldr d0, [r0, #32]\n"
            "vstr d1, [r1, #24]\n"
            "vldr d1, [r0, #40]\n"
            "vstr d0, [r1, #32]\n"
            "vldr d0, [r0, #48]\n"
            "vstr d1, [r1, #40]\n"
            "vldr d1, [r0, #56]\n"
            "vstr d0, [r1, #48]\n"
            "add r0, r0, #64\n"
            "vstr d1, [r1, #56]\n"
            "add r1, r1, #64\n"
            "bgt 1b\n"

            "sub r0, r0, r2\n"
            "sub r1, r1, r2\n"
            "subs r3, r3, #1\n"
            "bgt 0b\n"

            "ldmfd sp!, {r0,r1,r2,r3,r4}\n"
        :: "r" (_src), "r" (_dst), "r" (_size), "r" (num_loops) : "r0", "r1", "r2", "r3");
#endif
    }
};

class CopyVldmiaVstmiaBenchmark : public CopyBandwidthBenchmark {
public:
    CopyVldmiaVstmiaBenchmark() : CopyBandwidthBenchmark() { }
    virtual ~CopyVldmiaVstmiaBenchmark() {}

    const char *getName() { return "vldmia/vstmia"; }

    bool usesNeon() { return true; }

protected:
    // Copy using vldmia/vstmia instructions.
    void bench(size_t num_loops) {
#if defined(__ARM_NEON__)
        asm volatile(
            "stmfd sp!, {r0,r1,r2,r3,r4}\n"

            "mov r0, %0\n"
            "mov r1, %1\n"
            "mov r2, %2\n"
            "mov r3, %3\n"

            "0:\n"
            "mov r4, r2, lsr #6\n"

            "1:\n"
            "vldmia r0!, {d0-d7}\n"
            "subs r4, r4, #1\n"
            "vstmia r1!, {d0-d7}\n"
            "bgt 1b\n"

            "sub r0, r0, r2\n"
            "sub r1, r1, r2\n"
            "subs r3, r3, #1\n"
            "bgt 0b\n"

            "ldmfd sp!, {r0,r1,r2,r3,r4}\n"
        :: "r" (_src), "r" (_dst), "r" (_size), "r" (num_loops) : "r0", "r1", "r2", "r3");
#endif
    }
};

class MemcpyBenchmark : public CopyBandwidthBenchmark {
public:
    MemcpyBenchmark() : CopyBandwidthBenchmark() { }
    virtual ~MemcpyBenchmark() {}

    const char *getName() { return "memcpy"; }

protected:
    void bench(size_t num_loops) {
        for (size_t i = 0; i < num_loops; i++) {
            memcpy(_dst, _src, _size);
        }
    }
};

class SingleBufferBandwidthBenchmark : public BandwidthBenchmark {
public:
    SingleBufferBandwidthBenchmark() : BandwidthBenchmark(), _buffer(NULL) { }
    virtual ~SingleBufferBandwidthBenchmark() {
        if (_buffer) {
            free(_buffer);
            _buffer = NULL;
        }
    }

    bool setSize(size_t size) {
        if (_buffer) {
            free(_buffer);
            _buffer = NULL;
        }

        if (_size == 0) {
            _size = DEFAULT_SINGLE_BUFFER_SIZE;
        } else {
            _size = size;
        }

        _buffer = reinterpret_cast<char*>(memalign(64, _size));
        if (!_buffer) {
            perror("Failed to allocate memory for test.");
            return false;
        }
        memset(_buffer, 0, _size);

        return true;
    }

    bool verify() { return true; }

protected:
    char *_buffer;

    static const unsigned int DEFAULT_SINGLE_BUFFER_SIZE = 16000;
};

class WriteBandwidthBenchmark : public SingleBufferBandwidthBenchmark {
public:
    WriteBandwidthBenchmark() : SingleBufferBandwidthBenchmark() { }
    virtual ~WriteBandwidthBenchmark() { }

    bool verify() {
        memset(_buffer, 0, _size);
        bench(1);
        for (size_t i = 0; i < _size; i++) {
            if (_buffer[i] != 1) {
                printf("Buffer failed to compare after one loop.\n");
                return false;
            }
        }

        memset(_buffer, 0, _size);
        bench(2);
        for (size_t i = 0; i < _size; i++) {
            if (_buffer[i] != 2) {
                printf("Buffer failed to compare after two loops.\n");
                return false;
            }
        }

        return true;
    }
};

class WriteStrdBenchmark : public WriteBandwidthBenchmark {
public:
    WriteStrdBenchmark() : WriteBandwidthBenchmark() { }
    virtual ~WriteStrdBenchmark() {}

    const char *getName() { return "strd"; }

protected:
    // Write a given value using strd.
    void bench(size_t num_loops) {
        asm volatile(
            "stmfd sp!, {r0,r1,r2,r3,r4,r5}\n"

            "mov r0, %0\n"
            "mov r1, %1\n"
            "mov r2, %2\n"

            "mov r4, #0\n"
            "mov r5, #0\n"

            "0:\n"
            "mov r3, r1, lsr #5\n"

            "add r4, r4, #0x01010101\n"
            "mov r5, r4\n"

            "1:\n"
            "subs r3, r3, #1\n"
            "strd r4, r5, [r0]\n"
            "strd r4, r5, [r0, #8]\n"
            "strd r4, r5, [r0, #16]\n"
            "strd r4, r5, [r0, #24]\n"
            "add  r0, r0, #32\n"
            "bgt 1b\n"

            "sub r0, r0, r1\n"
            "subs r2, r2, #1\n"
            "bgt 0b\n"

            "ldmfd sp!, {r0,r1,r2,r3,r4,r5}\n"
          :: "r" (_buffer), "r" (_size), "r" (num_loops) : "r0", "r1", "r2");
    }
};

class WriteStmiaBenchmark : public WriteBandwidthBenchmark {
public:
    WriteStmiaBenchmark() : WriteBandwidthBenchmark() { }
    virtual ~WriteStmiaBenchmark() {}

    const char *getName() { return "stmia"; }

protected:
      // Write a given value using stmia.
      void bench(size_t num_loops) {
          asm volatile(
              "stmfd sp!, {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11}\n"

              "mov r0, %0\n"
              "mov r1, %1\n"
              "mov r2, %2\n"

              "mov r4, #0\n"

              "0:\n"
              "mov r3, r1, lsr #5\n"

              "add r4, r4, #0x01010101\n"
              "mov r5, r4\n"
              "mov r6, r4\n"
              "mov r7, r4\n"
              "mov r8, r4\n"
              "mov r9, r4\n"
              "mov r10, r4\n"
              "mov r11, r4\n"

              "1:\n"
              "subs r3, r3, #1\n"
              "stmia r0!, {r4, r5, r6, r7, r8, r9, r10, r11}\n"
              "bgt 1b\n"

              "sub r0, r0, r1\n"
              "subs r2, r2, #1\n"
              "bgt 0b\n"

              "ldmfd sp!, {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11}\n"
        :: "r" (_buffer), "r" (_size), "r" (num_loops) : "r0", "r1", "r2");
    }
};

class WriteVst1Benchmark : public WriteBandwidthBenchmark {
public:
    WriteVst1Benchmark() : WriteBandwidthBenchmark() { }
    virtual ~WriteVst1Benchmark() {}

    const char *getName() { return "vst1"; }

    bool usesNeon() { return true; }

protected:
    // Write a given value using vst.
    void bench(size_t num_loops) {
#if defined(__ARM_NEON__)
        asm volatile(
            "stmfd sp!, {r0,r1,r2,r3,r4}\n"

            "mov r0, %0\n"
            "mov r1, %1\n"
            "mov r2, %2\n"
            "mov r4, #0\n"

            "0:\n"
            "mov r3, r1, lsr #5\n"

            "add r4, r4, #1\n"
            "vdup.8 d0, r4\n"
            "vmov d1, d0\n"
            "vmov d2, d0\n"
            "vmov d3, d0\n"

            "1:\n"
            "subs r3, r3, #1\n"
            "vst1.8 {d0-d3}, [r0:128]!\n"
            "bgt 1b\n"

            "sub r0, r0, r1\n"
            "subs r2, r2, #1\n"
            "bgt 0b\n"

            "ldmfd sp!, {r0,r1,r2,r3,r4}\n"
        :: "r" (_buffer), "r" (_size), "r" (num_loops) : "r0", "r1", "r2");
#endif
    }
};

class WriteVstrBenchmark : public WriteBandwidthBenchmark {
public:
    WriteVstrBenchmark() : WriteBandwidthBenchmark() { }
    virtual ~WriteVstrBenchmark() {}

    const char *getName() { return "vstr"; }

    bool usesNeon() { return true; }

protected:
    // Write a given value using vst.
    void bench(size_t num_loops) {
#if defined(__ARM_NEON__)
        asm volatile(
            "stmfd sp!, {r0,r1,r2,r3,r4}\n"

            "mov r0, %0\n"
            "mov r1, %1\n"
            "mov r2, %2\n"
            "mov r4, #0\n"

            "0:\n"
            "mov r3, r1, lsr #5\n"

            "add r4, r4, #1\n"
            "vdup.8 d0, r4\n"
            "vmov d1, d0\n"
            "vmov d2, d0\n"
            "vmov d3, d0\n"

            "1:\n"
            "vstr d0, [r0, #0]\n"
            "subs r3, r3, #1\n"
            "vstr d1, [r0, #8]\n"
            "vstr d0, [r0, #16]\n"
            "vstr d1, [r0, #24]\n"
            "add r0, r0, #32\n"
            "bgt 1b\n"

            "sub r0, r0, r1\n"
            "subs r2, r2, #1\n"
            "bgt 0b\n"

            "ldmfd sp!, {r0,r1,r2,r3,r4}\n"
        :: "r" (_buffer), "r" (_size), "r" (num_loops) : "r0", "r1", "r2");
#endif
    }
};

class WriteVstmiaBenchmark : public WriteBandwidthBenchmark {
public:
    WriteVstmiaBenchmark() : WriteBandwidthBenchmark() { }
    virtual ~WriteVstmiaBenchmark() {}

    const char *getName() { return "vstmia"; }

    bool usesNeon() { return true; }

protected:
    // Write a given value using vstmia.
    void bench(size_t num_loops) {
#if defined(__ARM_NEON__)
        asm volatile(
            "stmfd sp!, {r0,r1,r2,r3,r4}\n"

            "mov r0, %0\n"
            "mov r1, %1\n"
            "mov r2, %2\n"
            "mov r4, #0\n"

            "0:\n"
            "mov r3, r1, lsr #5\n"

            "add r4, r4, #1\n"
            "vdup.8 d0, r4\n"
            "vmov d1, d0\n"
            "vmov d2, d0\n"
            "vmov d3, d0\n"

            "1:\n"
            "subs r3, r3, #1\n"
            "vstmia r0!, {d0-d3}\n"
            "bgt 1b\n"

            "sub r0, r0, r1\n"
            "subs r2, r2, #1\n"
            "bgt 0b\n"

            "ldmfd sp!, {r0,r1,r2,r3,r4}\n"
        :: "r" (_buffer), "r" (_size), "r" (num_loops) : "r0", "r1", "r2");
#endif
    }
};

class MemsetBenchmark : public WriteBandwidthBenchmark {
public:
    MemsetBenchmark() : WriteBandwidthBenchmark() { }
    virtual ~MemsetBenchmark() {}

    const char *getName() { return "memset"; }

protected:
    void bench(size_t num_loops) {
        for (size_t i = 0; i < num_loops; i++) {
            memset(_buffer, (i % 255) + 1, _size);
        }
    }
};

class ReadLdrdBenchmark : public SingleBufferBandwidthBenchmark {
public:
    ReadLdrdBenchmark() : SingleBufferBandwidthBenchmark() { }
    virtual ~ReadLdrdBenchmark() {}

    const char *getName() { return "ldrd"; }

protected:
    // Write a given value using strd.
    void bench(size_t num_loops) {
        asm volatile(
            "stmfd sp!, {r0,r1,r2,r3,r4,r5}\n"

            "mov r0, %0\n"
            "mov r1, %1\n"
            "mov r2, %2\n"

            "0:\n"
            "mov r3, r1, lsr #5\n"

            "1:\n"
            "subs r3, r3, #1\n"
            "ldrd r4, r5, [r0]\n"
            "ldrd r4, r5, [r0, #8]\n"
            "ldrd r4, r5, [r0, #16]\n"
            "ldrd r4, r5, [r0, #24]\n"
            "add  r0, r0, #32\n"
            "bgt 1b\n"

            "sub r0, r0, r1\n"
            "subs r2, r2, #1\n"
            "bgt 0b\n"

            "ldmfd sp!, {r0,r1,r2,r3,r4,r5}\n"
          :: "r" (_buffer), "r" (_size), "r" (num_loops) : "r0", "r1", "r2");
    }
};

class ReadLdmiaBenchmark : public SingleBufferBandwidthBenchmark {
public:
    ReadLdmiaBenchmark() : SingleBufferBandwidthBenchmark() { }
    virtual ~ReadLdmiaBenchmark() {}

    const char *getName() { return "ldmia"; }

protected:
      // Write a given value using stmia.
      void bench(size_t num_loops) {
          asm volatile(
              "stmfd sp!, {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11}\n"

              "mov r0, %0\n"
              "mov r1, %1\n"
              "mov r2, %2\n"

              "0:\n"
              "mov r3, r1, lsr #5\n"

              "1:\n"
              "subs r3, r3, #1\n"
              "ldmia r0!, {r4, r5, r6, r7, r8, r9, r10, r11}\n"
              "bgt 1b\n"

              "sub r0, r0, r1\n"
              "subs r2, r2, #1\n"
              "bgt 0b\n"

              "ldmfd sp!, {r0,r1,r2,r3,r4,r5,r6,r7,r8,r9,r10,r11}\n"
        :: "r" (_buffer), "r" (_size), "r" (num_loops) : "r0", "r1", "r2");
    }
};

class ReadVld1Benchmark : public SingleBufferBandwidthBenchmark {
public:
    ReadVld1Benchmark() : SingleBufferBandwidthBenchmark() { }
    virtual ~ReadVld1Benchmark() {}

    const char *getName() { return "vld1"; }

    bool usesNeon() { return true; }

protected:
    // Write a given value using vst.
    void bench(size_t num_loops) {
#if defined(__ARM_NEON__)
        asm volatile(
            "stmfd sp!, {r0,r1,r2,r3}\n"

            "mov r0, %0\n"
            "mov r1, %1\n"
            "mov r2, %2\n"

            "0:\n"
            "mov r3, r1, lsr #5\n"

            "1:\n"
            "subs r3, r3, #1\n"
            "vld1.8 {d0-d3}, [r0:128]!\n"
            "bgt 1b\n"

            "sub r0, r0, r1\n"
            "subs r2, r2, #1\n"
            "bgt 0b\n"

            "ldmfd sp!, {r0,r1,r2,r3}\n"
        :: "r" (_buffer), "r" (_size), "r" (num_loops) : "r0", "r1", "r2");
#endif
    }
};

class ReadVldrBenchmark : public SingleBufferBandwidthBenchmark {
public:
    ReadVldrBenchmark() : SingleBufferBandwidthBenchmark() { }
    virtual ~ReadVldrBenchmark() {}

    const char *getName() { return "vldr"; }

    bool usesNeon() { return true; }

protected:
    // Write a given value using vst.
    void bench(size_t num_loops) {
#if defined(__ARM_NEON__)
        asm volatile(
            "stmfd sp!, {r0,r1,r2,r3}\n"

            "mov r0, %0\n"
            "mov r1, %1\n"
            "mov r2, %2\n"

            "0:\n"
            "mov r3, r1, lsr #5\n"

            "1:\n"
            "vldr d0, [r0, #0]\n"
            "subs r3, r3, #1\n"
            "vldr d1, [r0, #8]\n"
            "vldr d0, [r0, #16]\n"
            "vldr d1, [r0, #24]\n"
            "add r0, r0, #32\n"
            "bgt 1b\n"

            "sub r0, r0, r1\n"
            "subs r2, r2, #1\n"
            "bgt 0b\n"

            "ldmfd sp!, {r0,r1,r2,r3}\n"
        :: "r" (_buffer), "r" (_size), "r" (num_loops) : "r0", "r1", "r2");
#endif
    }
};


class ReadVldmiaBenchmark : public SingleBufferBandwidthBenchmark {
public:
    ReadVldmiaBenchmark() : SingleBufferBandwidthBenchmark() { }
    virtual ~ReadVldmiaBenchmark() {}

    const char *getName() { return "vldmia"; }

    bool usesNeon() { return true; }

protected:
    // Write a given value using vstmia.
    void bench(size_t num_loops) {
#if defined(__ARM_NEON__)
        asm volatile(
            "stmfd sp!, {r0,r1,r2,r3}\n"

            "mov r0, %0\n"
            "mov r1, %1\n"
            "mov r2, %2\n"

            "0:\n"
            "mov r3, r1, lsr #5\n"

            "1:\n"
            "subs r3, r3, #1\n"
            "vldmia r0!, {d0-d3}\n"
            "bgt 1b\n"

            "sub r0, r0, r1\n"
            "subs r2, r2, #1\n"
            "bgt 0b\n"

            "ldmfd sp!, {r0,r1,r2,r3}\n"
        :: "r" (_buffer), "r" (_size), "r" (num_loops) : "r0", "r1", "r2");
#endif
    }
};

#endif  // __BANDWIDTH_H__
