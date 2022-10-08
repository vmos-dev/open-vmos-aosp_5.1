#ifndef TRACE_CONF_H
#define TRACE_CONF_H

#include <android/log.h>

#define LOG_TAG "libvpx"
#define LOGD(fmt_str, ...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "[%s:%d] "fmt_str, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#endif
