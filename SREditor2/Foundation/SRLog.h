#pragma once

//#ifdef __cplusplus
//extern "C" {
//#endif

void __sr_log_print_a(const char* filename, int line, int prio, const char* tag, const char* fmt, ...);
void __sr_log_print_w(const wchar_t* filename, int line, int prio, const wchar_t* tag, const wchar_t* fmt, ...);

void __sr_log_assert_a(const char* filename, int line, const char* cond, const char* tag, const char* fmt, ...);
void __sr_log_assert_w(const wchar_t* filename, int line, const wchar_t* cond, const wchar_t* tag, const wchar_t* fmt, ...);

void __sr_not_impl_w(const wchar_t* filename, int line, const wchar_t* func);

//#ifdef __cplusplus
//}
//#endif

typedef enum SR_LogPriority {
  SR_LOG_UNKNOWN = 0,
  SR_LOG_DEFAULT,    /* only for SetMinPriority() */
  SR_LOG_VERBOSE,
  SR_LOG_DEBUG,
  SR_LOG_INFO,
  SR_LOG_WARN,
  SR_LOG_ERROR,
  SR_LOG_FATAL,
  SR_LOG_SILENT,     /* only for SetMinPriority(); must be last */
} SR_LogPriority;

#define WIDEN2(x)   L##x
#define WIDEN(x)    WIDEN2(x)
#define __WFILE__       WIDEN(__FILE__)
#define __WFUNCTION__   WIDEN(__FUNCTION__)

// ---------------------------------------------------------------------

/*
 * Normally we strip SR_LOGV (VERBOSE messages) from release builds.
 * You can modify this (for example with "#define SR_LOG_NDEBUG 0"
 * at the top of your source file) to change that behavior.
 */
#ifndef SR_LOG_NDEBUG
#ifdef SR_NDEBUG
#define SR_LOG_NDEBUG 1
#else
#define SR_LOG_NDEBUG 0
#endif
#endif

/*
 * This is the local tag used for the following simplified
 * logging macros.  You can change this preprocessor definition
 * before using the other macros to change the tag.
 */
#ifndef SR_LOG_TAG
#define SR_LOG_TAG NULL
#endif

// ---------------------------------------------------------------------

/*
 * Simplified macro to send a verbose log message using the current SR_LOG_TAG.
 */
#ifndef SR_LOGV
#if SR_LOG_NDEBUG
#define SR_LOGV(...)   ((void)0)
#else
#define SR_LOGV(...) ((void)SR_LOG(SR_LOG_VERBOSE, SR_LOG_TAG, __VA_ARGS__))
#endif
#endif

//#define SR_CONDITION(cond)     (__builtin_expect((cond)!=0, 0))
#define SR_CONDITION(cond)     (cond)

#ifndef SR_LOGV_IF
#if SR_LOG_NDEBUG
#define SR_LOGV_IF(cond, ...)   ((void)0)
#else
#define SR_LOGV_IF(cond, ...) \
    ( (SR_CONDITION(cond)) \
    ? ((void)SR_LOG(SR_LOG_VERBOSE, SR_LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif
#endif

/*
 * Simplified macro to send a debug log message using the current SR_LOG_TAG.
 */
#ifndef SR_LOGD
#define SR_LOGD(...) ((void)SR_LOG(SR_LOG_DEBUG, SR_LOG_TAG, __VA_ARGS__))
#endif

#ifndef SR_LOGD_IF
#define SR_LOGD_IF(cond, ...) \
    ( (SR_CONDITION(cond)) \
    ? ((void)SR_LOG(SR_LOG_DEBUG, SR_LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

/*
 * Simplified macro to send an info log message using the current SR_LOG_TAG.
 */
#ifndef SR_LOGI
#define SR_LOGI(...) ((void)SR_LOG(SR_LOG_INFO, SR_LOG_TAG, __VA_ARGS__))
#endif

#ifndef SR_LOGI_IF
#define SR_LOGI_IF(cond, ...) \
    ( (SR_CONDITION(cond)) \
    ? ((void)SR_LOG(SR_LOG_INFO, SR_LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

/*
 * Simplified macro to send a warning log message using the current SR_LOG_TAG.
 */
#ifndef SR_LOGW
#define SR_LOGW(...) ((void)SR_LOG(SR_LOG_WARN, SR_LOG_TAG, __VA_ARGS__))
#endif

#ifndef SR_LOGW_IF
#define SR_LOGW_IF(cond, ...) \
    ( (SR_CONDITION(cond)) \
    ? ((void)SR_LOG(SR_LOG_WARN, SR_LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

/*
 * Simplified macro to send an error log message using the current SR_LOG_TAG.
 */
#ifndef SR_LOGE
#define SR_LOGE(...) ((void)SR_LOG(SR_LOG_ERROR, SR_LOG_TAG, __VA_ARGS__))
#endif

#ifndef SR_LOGE_IF
#define SR_LOGE_IF(cond, ...) \
    ( (SR_CONDITION(cond)) \
    ? ((void)SR_LOG(SR_LOG_ERROR, SR_LOG_TAG, __VA_ARGS__)) \
    : (void)0 )
#endif

// ---------------------------------------------------------------------

#if SR_LOG_NDEBUG

#ifndef SR_LOG_ASSERT
#define SR_LOG_ASSERT(cond, ...) ((void)0)
#endif
#ifndef SR_ASSERT
#define SR_ASSERT(cond) ((void)0)
#endif
#ifndef SR_VERIFY
#define SR_VERIFY(cond) (cond)
#endif
#ifndef SR_NOT_IMPL
#define SR_NOT_IMPL() ((void)0)
#endif

#else //SR_LOG_NDEBUG

#ifndef SR_LOG_ASSERT
#ifdef SR_UNICODE
#define SR_LOG_ASSERT(cond, ...)  \
    ( (SR_CONDITION(!(cond))) \
    ? ((void)__sr_log_assert_w(__WFILE__, __LINE__, WIDEN(#cond), SR_LOG_TAG, ## __VA_ARGS__)) \
    : (void)0 )
#else
#define SR_LOG_ASSERT(cond, ...)  \
    ( (SR_CONDITION(!(cond))) \
    ? ((void)__sr_log_assert_a(__FILE__, __LINE__, #cond, SR_LOG_TAG, ## __VA_ARGS__)) \
    : (void)0 )
#endif
#endif

#ifndef SR_ASSERT
#ifdef SR_UNICODE
#define SR_ASSERT(cond)  \
    ( (SR_CONDITION(!(cond))) \
    ? ((void)__sr_log_assert_w(__WFILE__, __LINE__, WIDEN(#cond), SR_LOG_TAG, L"Assertion failed: " WIDEN(#cond))) \
    : (void)0 )
#else
#define SR_ASSERT(cond)  \
    ( (SR_CONDITION(!(cond))) \
    ? ((void)__sr_log_assert_a(__FILE__, __LINE__, #cond, SR_LOG_TAG, "Assertion failed: " #cond)) \
    : (void)0 )
#endif
#endif

#ifndef SR_VERIFY
#define SR_VERIFY(cond)  \
    ( (SR_CONDITION(!(cond))) \
    ? ((void)__sr_log_assert_w(__WFILE__, __LINE__, WIDEN(#cond), SR_LOG_TAG, L"Assertion failed: " WIDEN(#cond))) \
    : (void)0 )
#endif

#ifndef SR_NOT_IMPL
#define SR_NOT_IMPL() ((void)__sr_not_impl_w(__WFILE__, __LINE__, __WFUNCTION__))
#endif

#endif //SR_LOG_NDEBUG

// ---------------------------------------------------------------------

/*
 * Basic log message macro.
 *
 * Example:
 *  SR_LOG(SR_LOG_WARN, NULL, "Failed with error %d", errno);
 *
 * The second argument may be NULL or "" to indicate the "global" tag.
 */
#ifndef SR_LOG
#define SR_LOG(priority, tag, ...) \
    SR_LOG_PRI(priority, tag, __VA_ARGS__)
#endif

/*
 * Log macro that allows you to specify a number for the priority.
 */
#ifndef SR_LOG_PRI
//#define LOG_PRI(priority, tag, ...) \
//  android_printLog(priority, tag, __VA_ARGS__)
#ifdef SR_UNICODE
#define SR_LOG_PRI(priority, tag, ...) __sr_log_print_w(__WFILE__, __LINE__, priority, tag, __VA_ARGS__)
#else
#define SR_LOG_PRI(priority, tag, ...) __sr_log_print_a(__FILE__, __LINE__, priority, tag, __VA_ARGS__)
#endif
#endif

//#ifndef SR_LOG_DUMP // for SRObject::dump()
//#define SR_LOG_DUMP(str, ...)   SR_LOGD(L"%s" ## str, indent, __VA_ARGS__)
//#define SR_LOG_DUMP_START(s)    SR_LOGD(L"%s---- %s(%d) [START] ----", indent, s, id())
//#define SR_LOG_DUMP_END(s)      SR_LOGD(L"%s---- %s(%d) [END] ----", indent, s, id())
//#define SR_LOG_DUMP_END_NL(s)   SR_LOGD(L"%s---- %s(%d) [END] ----\n", indent, s, id())
//#endif

#ifndef SR_NDEBUG
// should be called in SRObject child.
//#define SR_LOG_FUNC()  __sr_log_print_a(__FILE__, __LINE__, SR_LOG_DEBUG, NULL, "%s(%d) this=0x%p", __FUNCTION__, SRObject::_id, this)
#define SR_LOG_FUNC()  SR_LOGD(L"%s(%d) this=0x%p", __WFUNCTION__, id(), this)
#else
#define SR_LOG_FUNC()
#endif
