#ifndef LOCAL_LOG_DEF_H
#define LOCAL_LOG_DEF_H

/* Log enablers, each of these independent */

#define PROCESS_VERBOSE (0) /* process log messages */
#define EXTRA_VERBOSE   (0) /* verbose log messages */
#define SYSFS_VERBOSE   (0) /* log sysfs interactions as cat/echo for repro
                               purpose on a shell */
#define FUNC_ENTRY      (0) /* log entry in all one-time functions */

/* Note that enabling this logs may affect performance */
#define HANDLER_ENTRY   (0) /* log entry in all handler functions */
#define ENG_VERBOSE     (0) /* log some a lot more info about the internals */
#define INPUT_DATA      (0) /* log the data input from the events */
#define HANDLER_DATA    (0) /* log the data fetched from the handlers */

#if defined ANDROID_JELLYBEAN
#define LOGV            ALOGV
#define LOGV_IF         ALOGV_IF
#define LOGD            ALOGD
#define LOGD_IF         ALOGD_IF
#define LOGI            ALOGI
#define LOGI_IF         ALOGI_IF
#define LOGW            ALOGW
#define LOGW_IF         ALOGW_IF
#define LOGE            ALOGE
#define LOGE_IF         ALOGE_IF
#define IF_LOGV         IF_ALOGV
#define IF_LOGD         IF_ALOGD
#define IF_LOGI         IF_ALOGI
#define IF_LOGW         IF_ALOGW
#define IF_LOGE         IF_ALOGE
#define LOG_ASSERT      ALOG_ASSERT
#define LOG                     ALOG
#define IF_LOG          IF_ALOG
#else
#warning "build for ICS or earlier version"
#endif


#define FUNC_LOG \
            LOGV("%s", __PRETTY_FUNCTION__)
#define VFUNC_LOG \
            LOGV_IF(FUNC_ENTRY, "Entering function '%s'", __PRETTY_FUNCTION__)
#define VHANDLER_LOG \
            LOGV_IF(HANDLER_ENTRY, "Entering handler '%s'", __PRETTY_FUNCTION__)

#endif /*ifndef LOCAL_LOG_DEF_H */
