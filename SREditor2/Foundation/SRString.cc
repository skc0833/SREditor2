#include "SRString.h"
#include <cstdarg>
#include <memory>
#include <wchar.h> // for _vsnwprintf_s

#undef SR_LOG_FUNC
#define SR_LOG_FUNC()

using namespace std;

namespace sr {

SRString::SRString() {
  SR_LOG_FUNC();
  init(L"", 0);
}

SRString::SRString(const std::wstring& str) {
  init(str.c_str(), wcslen(str.c_str()));
}

SRString::SRString(const wchar_t* characters) {
  SR_LOG_FUNC();
  init(characters, wcslen(characters));
  //this->SRString::SRString(characters, wcslen(characters)); // not good! gcc compile error!
}

SRString::SRString(const wchar_t* characters, SRUInt32 length) {
  SR_LOG_FUNC();
  init(characters, length);
}

SRString::SRString(const SRStringPtr aString) {
  SR_LOG_FUNC();
  init(aString->c_str(), aString->length());
}

SRString::SRString(const SRString& rhs) {
  SR_LOG_FUNC();
  _data.assign(rhs.data());
}

SRString::~SRString() {
  SR_LOG_FUNC();
}

SRUInt SRString::hash() const {
  return std::hash<std::wstring>()(data());
}

void SRString::init(const wchar_t* characters, SRUInt32 length) {
  //_data = new wstring(characters, length);
  _data.assign(characters, length);
  //SR_LOGD(L"init(%s, %d)", characters, length);
}

SRString& SRString::operator=(SRString rhs) {
  std::swap(_data, rhs._data);
  //_data.assign(rhs.data());
  return *this;
}

SRString& SRString::operator+=(const SRString& rhs) {
  append(rhs);
  return *this;
}

SRString& SRString::operator+=(SRInt rhs) {
  std::wstring s = std::to_wstring(rhs);
  append(SRString(s));
  return *this;
}

bool SRString::isEqual(const SRObject& obj) const {
  const auto& other = static_cast<const SRString&>(obj);
  const wstring& s1 = this->data();
  const wstring& s2 = other.data();
  if (s1.compare(s2) == 0) {
    return true;
  } else {
    return false;
  }
}

//bool SRString::operator!=(const SRObject& rhs) const {
//  return !(*this == rhs);
//}

bool SRString::operator<(const SRObject& rhs) const {
  if (this == &rhs) {
    return true;
  }
  const wstring& s1 = this->data();
  const wstring& s2 = ((SRString&)rhs).data();
  if (s1.compare(s2) < 0) {
    return true;
  } else {
    return false;
  }

  return true;
}

const wchar_t* SRString::c_str() const {
  return (const wchar_t*)(data().c_str());
}

SRString::operator const wchar_t*() const {
  return (const wchar_t*)(this->c_str());
}

SRString::operator const wchar_t*() {
  return (const wchar_t*)(this->c_str());
}

SRStringUPtr SRString::toString() const {
  return std::make_unique<SRString>(_data);
}

const std::wstring& SRString::data() const {
  return _data;
}

SRIndex SRString::length() const {
  return data().length();
}

SRString& SRString::append(const SRString& aString) {
  _data.append(aString.data());
  return *this;
}

SRString& SRString::replace(const SRString& replacement) {
  return replaceCharactersInRange(SRRange(0, length()), replacement);
}

SRString& SRString::replaceCharactersInRange(SRRange range, const SRString& replacement) {
  _data.replace(range.location, range.length, replacement.data());
  return *this;
}

SRRange SRString::range(const SRString& aString) {
  // An NSRange structure giving the location and length in the receiver of the 
  // first occurrence of aString. Returns {NSNotFound, 0} if aString is not found or is empty (@"").
  SRRange range(0, 0);
  std::wstring::size_type found = _data.find(aString.data());
  if (found != std::wstring::npos) {
    range.location = found;
    range.length = aString.length();
  }
  return range;
}

SRBool SRString::hasPrefix(const SRString& aString) {
  const std::wstring& s = aString.data();
  if (_data.length() < s.length()) return false;
  std::wstring::size_type pos = 0;
  std::wstring::size_type len = s.length();
  if (_data.compare(pos, len, s) == 0) return true;
  else return false;
}

SRBool SRString::hasSuffix(const SRString& aString) {
  const std::wstring& s = aString.data();
  if (_data.length() < s.length()) return false;
  std::wstring::size_type pos = _data.length() - s.length();
  std::wstring::size_type len = s.length();
  if (pos >= 0 && _data.compare(pos, len, s) == 0) return true;
  else return false;
}

SRStringPtr SRString::substring(const SRRange& aRange) {
  SRRange range = aRange;
  if (aRange.length <= 0) range.length = _data.length();
  auto ptr(std::make_shared<SRString>(_data.substr(range.location, range.length)));
  return ptr;
}

// Returns a new string containing the characters of the receiver up to, but not including, the one at a given index.
SRStringPtr SRString::substring(SRUInt anIndex) {
  auto ptr(std::make_shared<SRString>(_data.substr(0, anIndex)));
  return ptr;
}

//static
SRString SRString::format(const wchar_t* format, ...) {
  // from http://stackoverflow.com/questions/2342162/stdstring-formatting-like-sprintf
  va_list args;
  va_start(args, format);
  #ifndef _MSC_VER
    //GCC generates warning for valid use of snprintf to get
    //size of result string. We suppress warning with below macro.
    #ifdef __GNUC__
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wformat-nonliteral"
    #endif

    size_t size = std::snprintf(nullptr, 0, format, args) + 1; // Extra space for '\0'

    #ifdef __GNUC__
    # pragma GCC diagnostic pop
    #endif

    std::unique_ptr<char[]> buf(new char[ size ] ); 
    std::vsnprintf(buf.get(), size, format, args);
    std::wstring result(buf.get(), buf.get() + size - 1 ); // We don't want the '\0' inside
    //auto result = std::make_unique<SRString>(result);
    return result;
  #else
    int size = _vscwprintf(format, args);
    std::wstring result(++size, 0);
	_vsnwprintf_s((wchar_t*)result.data(), size, _TRUNCATE, format, args);
    //auto result = std::make_unique<SRString>(result);
    return result;
  #endif
  va_end(args);
}

} // namespace sr
