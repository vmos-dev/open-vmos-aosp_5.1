/*
 * Copyright (C) Texas Instruments - http://www.ti.com/
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

#ifndef DEBUG_UTILS_H
#define DEBUG_UTILS_H

#include <android/log.h>
#include <utils/threads.h>
#include <utils/Vector.h>




namespace Ti {




// use 2 space characters for call stack indent
static const int kFunctionLoggerIndentSize = 2;




template <int Size = kFunctionLoggerIndentSize>
class IndentString
{
public:
    IndentString(int length);

    const char * string() const;

private:
    int calculateOffset(int length) const;

private:
    const int mOffset;
};




class Debug
{
public:
    static Debug * instance();

    int offsetForCurrentThread();
    void log(int priority, const char * format, ...);

private:
    class ThreadInfo
    {
    public:
        ThreadInfo() :
            threadId(0), callOffset(0)
        {}

        volatile int32_t threadId;
        int callOffset;
    };

    class Data : public android::RefBase
    {
    public:
        android::Vector<ThreadInfo*> threads;
    };

private:
    // called from FunctionLogger
    void increaseOffsetForCurrentThread();
    void decreaseOffsetForCurrentThread();

private:
    Debug();

    void grow();
    ThreadInfo * registerThread(Data * data, int32_t threadId);
    ThreadInfo * findCurrentThreadInfo();
    void addOffsetForCurrentThread(int offset);

private:
    static Debug sInstance;

    mutable android::Mutex mMutex;
    android::sp<Data> mData;

    friend class FunctionLogger;
};




class FunctionLogger
{
public:
    FunctionLogger(const char * file, int line, const char * function);
    ~FunctionLogger();

    void setExitLine(int line);

private:
    const char * const mFile;
    const int mLine;
    const char * const mFunction;
    const void * const mThreadId;
    int mExitLine;
};




#ifdef TI_UTILS_FUNCTION_LOGGER_ENABLE
#   define LOG_FUNCTION_NAME Ti::FunctionLogger __function_logger_instance(__FILE__, __LINE__, __FUNCTION__);
#   define LOG_FUNCTION_NAME_EXIT __function_logger_instance.setExitLine(__LINE__);
#else
#   define LOG_FUNCTION_NAME int __function_logger_instance;
#   define LOG_FUNCTION_NAME_EXIT (void*)__function_logger_instance;
#endif

#ifdef TI_UTILS_DEBUG_USE_TIMESTAMPS
    // truncate timestamp to 1000 seconds to fit into 6 characters
#   define TI_UTILS_DEBUG_TIMESTAMP_TOKEN "[%06d] "
#   define TI_UTILS_DEBUG_TIMESTAMP_VARIABLE static_cast<int>(nanoseconds_to_milliseconds(systemTime()) % 1000000),
#else
#   define TI_UTILS_DEBUG_TIMESTAMP_TOKEN
#   define TI_UTILS_DEBUG_TIMESTAMP_VARIABLE
#endif




#define DBGUTILS_LOGV_FULL(priority, file, line, function, format, ...)       \
    do                                                                        \
    {                                                                         \
        Ti::Debug * const debug = Ti::Debug::instance();                      \
        debug->log(priority, format,                                          \
                TI_UTILS_DEBUG_TIMESTAMP_VARIABLE                             \
                reinterpret_cast<int>(androidGetThreadId()),                  \
                Ti::IndentString<>(debug->offsetForCurrentThread()).string(), \
                file, line, function, __VA_ARGS__);                           \
    } while (0)

#define DBGUTILS_LOGV(...) DBGUTILS_LOGV_FULL(ANDROID_LOG_VERBOSE, __FILE__, __LINE__, __FUNCTION__, TI_UTILS_DEBUG_TIMESTAMP_TOKEN "(%x) %s  %s:%d %s - " __VA_ARGS__, "")
#define DBGUTILS_LOGD(...) DBGUTILS_LOGV_FULL(ANDROID_LOG_DEBUG,   __FILE__, __LINE__, __FUNCTION__, TI_UTILS_DEBUG_TIMESTAMP_TOKEN "(%x) %s  %s:%d %s - " __VA_ARGS__, "")
#define DBGUTILS_LOGI(...) DBGUTILS_LOGV_FULL(ANDROID_LOG_INFO,    __FILE__, __LINE__, __FUNCTION__, TI_UTILS_DEBUG_TIMESTAMP_TOKEN "(%x) %s  %s:%d %s - " __VA_ARGS__, "")
#define DBGUTILS_LOGW(...) DBGUTILS_LOGV_FULL(ANDROID_LOG_WARN,    __FILE__, __LINE__, __FUNCTION__, TI_UTILS_DEBUG_TIMESTAMP_TOKEN "(%x) %s  %s:%d %s - " __VA_ARGS__, "")
#define DBGUTILS_LOGE(...) DBGUTILS_LOGV_FULL(ANDROID_LOG_ERROR,   __FILE__, __LINE__, __FUNCTION__, TI_UTILS_DEBUG_TIMESTAMP_TOKEN "(%x) %s  %s:%d %s - " __VA_ARGS__, "")
#define DBGUTILS_LOGF(...) DBGUTILS_LOGV_FULL(ANDROID_LOG_FATAL,   __FILE__, __LINE__, __FUNCTION__, TI_UTILS_DEBUG_TIMESTAMP_TOKEN "(%x) %s  %s:%d %s - " __VA_ARGS__, "")

#define DBGUTILS_LOGVA DBGUTILS_LOGV
#define DBGUTILS_LOGVB DBGUTILS_LOGV

#define DBGUTILS_LOGDA DBGUTILS_LOGD
#define DBGUTILS_LOGDB DBGUTILS_LOGD

#define DBGUTILS_LOGEA DBGUTILS_LOGE
#define DBGUTILS_LOGEB DBGUTILS_LOGE

// asserts
#define _DBGUTILS_PLAIN_ASSERT(condition)                              \
    do                                                                 \
    {                                                                  \
        if ( !(condition) )                                            \
        {                                                              \
            __android_log_print(ANDROID_LOG_FATAL, "Ti::Debug",        \
                    "Condition failed: " #condition);                  \
            __android_log_print(ANDROID_LOG_FATAL, "Ti::Debug",        \
                    "Aborting process...");                            \
            abort();                                                   \
        }                                                              \
    } while (0)

#define _DBGUTILS_PLAIN_ASSERT_X(condition, ...)                       \
    do                                                                 \
    {                                                                  \
        if ( !(condition) )                                            \
        {                                                              \
            __android_log_print(ANDROID_LOG_FATAL, "Ti::Debug",        \
                    "Condition failed: " #condition ": " __VA_ARGS__); \
            __android_log_print(ANDROID_LOG_FATAL, "Ti::Debug",        \
                    "Aborting process...");                            \
            abort();                                                   \
        }                                                              \
    } while (0)

#define DBGUTILS_ASSERT(condition)                                           \
    do                                                                       \
    {                                                                        \
        if ( !(condition) )                                                  \
        {                                                                    \
            DBGUTILS_LOGF("Condition failed: " #condition);                  \
            DBGUTILS_LOGF("Aborting process...");                            \
            abort();                                                         \
        }                                                                    \
    } while (0)

#define DBGUTILS_ASSERT_X(condition, ...)                                    \
    do                                                                       \
    {                                                                        \
        if ( !(condition) )                                                  \
        {                                                                    \
            DBGUTILS_LOGF("Condition failed: " #condition ": " __VA_ARGS__); \
            DBGUTILS_LOGF("Aborting process...");                            \
            abort();                                                         \
        }                                                                    \
    } while (0)




static const int kIndentStringMaxLength = 128;

template <int Size>
inline int IndentString<Size>::calculateOffset(const int length) const
{
    const int offset = kIndentStringMaxLength - length*Size;
    return offset < 0 ? 0 : offset;
}

template <int Size>
inline IndentString<Size>::IndentString(const int length) :
    mOffset(calculateOffset(length))
{}

template <int Size>
inline const char * IndentString<Size>::string() const
{
    extern const char sIndentStringBuffer[];
    return sIndentStringBuffer + mOffset;
}




inline Debug * Debug::instance()
{ return &sInstance; }


inline Debug::ThreadInfo * Debug::findCurrentThreadInfo()
{
    // retain reference to threads data
    android::sp<Data> data = mData;

    // iterate over threads to locate thread id,
    // this is safe from race conditions because each thread
    // is able to modify only his own ThreadInfo structure
    const int32_t threadId = reinterpret_cast<int32_t>(androidGetThreadId());
    const int size = int(data->threads.size());
    for ( int i = 0; i < size; ++i )
    {
        ThreadInfo * const threadInfo = data->threads.itemAt(i);
        if ( threadInfo->threadId == threadId )
            return threadInfo;
    }

    // this thread has not been registered yet,
    // try to fing empty thread info slot
    while ( true )
    {
        ThreadInfo * const threadInfo = registerThread(data.get(), threadId);
        if ( threadInfo )
            return threadInfo;

        // failed registering thread, because all slots are occupied
        // grow the data and try again
        grow();

        data = mData;
    }

    // should never reach here
    _DBGUTILS_PLAIN_ASSERT(false);
    return 0;
}


inline void Debug::addOffsetForCurrentThread(const int offset)
{
    if ( offset == 0 )
        return;

    ThreadInfo * const threadInfo = findCurrentThreadInfo();
    _DBGUTILS_PLAIN_ASSERT(threadInfo);

    threadInfo->callOffset += offset;

    if ( threadInfo->callOffset == 0 )
    {
        // thread call stack has dropped to zero, unregister it
        android_atomic_acquire_store(0, &threadInfo->threadId);
    }
}


inline int Debug::offsetForCurrentThread()
{
#ifdef TI_UTILS_FUNCTION_LOGGER_ENABLE
    ThreadInfo * const threadInfo = findCurrentThreadInfo();
    _DBGUTILS_PLAIN_ASSERT(threadInfo);

    return threadInfo->callOffset;
#else
    return 0;
#endif
}


inline void Debug::increaseOffsetForCurrentThread()
{
#ifdef TI_UTILS_FUNCTION_LOGGER_ENABLE
    addOffsetForCurrentThread(1);
#endif
}


inline void Debug::decreaseOffsetForCurrentThread()
{
#ifdef TI_UTILS_FUNCTION_LOGGER_ENABLE
    addOffsetForCurrentThread(-1);
#endif
}


inline void Debug::log(const int priority, const char * const format, ...)
{
    va_list args;
    va_start(args, format);
    __android_log_vprint(priority, LOG_TAG, format, args);
    va_end(args);
}




inline FunctionLogger::FunctionLogger(const char * const file, const int line, const char * const function) :
    mFile(file), mLine(line), mFunction(function), mThreadId(androidGetThreadId()), mExitLine(-1)
{
    Debug * const debug = Debug::instance();
    debug->increaseOffsetForCurrentThread();
    android_printLog(ANDROID_LOG_DEBUG, LOG_TAG,
            TI_UTILS_DEBUG_TIMESTAMP_TOKEN "(%x) %s+ %s:%d %s - ENTER",
            TI_UTILS_DEBUG_TIMESTAMP_VARIABLE
            (int)mThreadId, IndentString<>(debug->offsetForCurrentThread()).string(),
            mFile, mLine, mFunction);
}


inline FunctionLogger::~FunctionLogger()
{
    Debug * const debug = Debug::instance();
    android_printLog(ANDROID_LOG_DEBUG, LOG_TAG,
            TI_UTILS_DEBUG_TIMESTAMP_TOKEN "(%x) %s- %s:%d %s - EXIT",
            TI_UTILS_DEBUG_TIMESTAMP_VARIABLE
            (int)mThreadId, IndentString<>(debug->offsetForCurrentThread()).string(),
            mFile, mExitLine == -1 ? mLine : mExitLine, mFunction);
    debug->decreaseOffsetForCurrentThread();
}


inline void FunctionLogger::setExitLine(const int line)
{
    if ( mExitLine != -1 )
    {
        Debug * const debug = Debug::instance();
        android_printLog(ANDROID_LOG_DEBUG, LOG_TAG,
                TI_UTILS_DEBUG_TIMESTAMP_TOKEN "(%x) %s  %s:%d %s - Double function exit trace detected. Previous: %d",
                TI_UTILS_DEBUG_TIMESTAMP_VARIABLE
                (int)mThreadId, IndentString<>(debug->offsetForCurrentThread()).string(),
                mFile, line, mFunction, mExitLine);
    }

    mExitLine = line;
}




} // namespace Ti




#endif //DEBUG_UTILS_H
