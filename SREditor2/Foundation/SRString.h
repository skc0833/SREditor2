#pragma once

#include "SRObject.h"
#include <string> // for std::wstring

namespace sr {

class SRString;
using SRStringPtr = std::shared_ptr<SRString>;
using SRStringUPtr = std::unique_ptr<SRString>;

class SRString : public SRObject {
  //SR_MAKE_NONCOPYABLE(SRString);
public:
  SR_DECL_CREATE_FUNC(SRString);

  SRString();
  SRString(const std::wstring& str);
  SRString(const wchar_t* characters);
  SRString(const wchar_t* characters, SRUInt32 length);
  SRString(const SRStringPtr aString);
  SRString(const SRString& rhs);
  ~SRString();

  virtual SRUInt hash() const;

  virtual SRString& operator=(SRString rhs);
  virtual SRString& operator+=(const SRString& rhs);
  virtual SRString& operator+=(SRInt rhs);
  virtual bool operator<(const SRObject& rhs) const;

  operator const wchar_t*() const;
  operator const wchar_t*();
  virtual SRStringUPtr toString() const;

  const wchar_t* c_str() const;
  const std::wstring& data() const;
  SRIndex length() const;

  SRString& append(const SRString& aString);
  SRString& replace(const SRString& replacement);
  SRString& replaceCharactersInRange(SRRange range, const SRString& replacement);

  // Finding Characters and Substrings
  //
  // Finds and returns the range of the first occurrence of a given string within the receiver.
  SRRange range(const SRString& aString);

  // Identifying and Comparing Strings
  //
  SRBool hasPrefix(const SRString& aString);
  SRBool hasSuffix(const SRString& aString);

  // Dividing Strings
  //
  // Returns a string object containing the characters of the receiver that lie within a given range.
  SRStringPtr substring(const SRRange& range);
  // Returns a new string containing the characters of the receiver up to, but not including, the one at a given index.
  SRStringPtr substring(SRUInt anIndex);

  static SRString SRString::format(const wchar_t* format, ...);

protected:
  virtual bool isEqual(const SRObject& obj) const override;

private:
  void init(const wchar_t* characters, SRUInt32 length);
  std::wstring _data; // 디버깅 편의를 위해 void* 대신 타입을 명시함
};

} // namespace sr

// for SRDictionary's std::unordered_map
template <>
struct std::hash<sr::SRString> {
  std::size_t operator()(const sr::SRString& k) const {
    std::size_t a = std::hash<std::wstring>()(k.data());
    std::size_t b = k.hash();
    SR_ASSERT(0);
    return k.hash();
  }
};

template <>
struct std::hash<const sr::SRString> { // const 가 있어야 컴파일에러가 발생하지 않고 있음
  std::size_t operator()(const sr::SRString& k) const {
    std::size_t a = std::hash<std::wstring>()(k.data());
    std::size_t b = k.hash();
    return k.hash();
    //return std::hash<std::wstring>()(k.data());
  }
};
