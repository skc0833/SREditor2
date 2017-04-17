#pragma once

#ifdef _DEBUG // from vs2015
  #define SR_DEBUG_DRAW
#else
  //#define SR_NDEBUG // for release build!
#endif

#ifdef _UNICODE // from vs2015
  #define SR_UNICODE
#else
  #error "_UNICODE should be set!!!"
#endif

#define NULL 0

#define SR_MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define SR_MAX(a, b)  (((a) > (b)) ? (a) : (b))

#define SR_MAKE_NONCOPYABLE(ClassName) \
private: \
  ClassName(const ClassName&) = delete; \
  ClassName& operator=(const ClassName&) = delete;

// C++11 variadic template parameters
#define SR_DECL_CREATE_FUNC(classType)   \
  template<typename ... Ts> \
  static classType ## Ptr create(Ts&& ... params) { \
    return std::make_shared<classType>(params...); \
  }

// 기본 데이터 타입들 정의
#include "SRDataTypes.h"
#include "SRLog.h"
#include "SRDebug.h"
#include <cstddef> // for std::size_t

//#define _countof(_arr)  (sizeof(_arr) / sizeof(_arr[0])) // only supported vc++
template <typename T, std::size_t N>
constexpr std::size_t SR_countof(T const (&)[N]) noexcept {
  return N;
}
