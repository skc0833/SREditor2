#pragma once

#define SR_PROF_TIME_START(tag) \
  UITimeInterval __time_now = std::chrono::system_clock::now(); \
  UITimeInterval __time_start = __time_now; \
  UITimeInterval __time_last = __time_now; \
  std::chrono::milliseconds __elapsed_ms; \
  auto __now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(__time_now); \
  SR_LOGD(L"%12lldms %-16s: [BEGIN] +++", __now_ms.time_since_epoch().count(), tag ? tag : L"");

#define SR_PROF_TIME_STAMP(tag) \
  __time_now = std::chrono::system_clock::now(); \
  __now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(__time_now); \
  __elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(__time_now - __time_last); \
  SR_LOGD(L"%12lldms %-16s: %8lldms", __now_ms.time_since_epoch().count(), tag ? tag : L"", __elapsed_ms.count()); \
  __time_last = __time_now;

#define SR_PROF_TIME_END(tag) \
  __time_now = std::chrono::system_clock::now(); \
  __now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(__time_now); \
  __elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(__time_now - __time_start); \
  SR_LOGD(L"%12lldms %-16s: [END] --- %8lldms", __now_ms.time_since_epoch().count(), tag ? tag : L"", __elapsed_ms.count());
