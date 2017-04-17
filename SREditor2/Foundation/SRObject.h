#pragma once

#include "SRBase.h"
#include <memory> // for shared_ptr

//#define SR_USE_MATRIX_DRAW

namespace sr {

class SRString;
using SRStringUPtr = std::unique_ptr<SRString>;

class SRObject;
using SRObjectPtr = std::shared_ptr<SRObject>;

class SRObject {
public:
  // 추후 필요할 경우 사용하기 위함(e.g, std::unordered_map<UIFont, UIFontPtr, Hash<UIFont>, Equal<UIFont>> _fontCache;)
  //template<typename Key>
  //struct Hash {
  //  std::size_t operator()(const Key& k) const;
  //};
  //template<typename Key>
  //struct Equal {
  //  bool operator()(const Key& lhs, const Key& rhs) const;
  //};
  struct DebugInfo {
    SRUInt _obj_cnt;
  };

  SRObject();
  virtual ~SRObject();

  virtual SRUInt hash() const;
  virtual SRStringUPtr toString() const;
  virtual void dump() const;

  SRIndex id() const { return _id; }

  static DebugInfo _debugInfo;
  static const DebugInfo& debugInfo() {
    return _debugInfo;
  }

protected:
  friend bool operator==(const SRObject& lhs, const SRObject& rhs);
  friend bool operator!=(const SRObject& lhs, const SRObject& rhs);
  virtual bool isEqual(const SRObject& obj) const;

  SRIndex _id;
};

// Effective STL, Item 32 - Scott Meyers Erase-Remove idiom 
// https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Erase-Remove
// like boost's remove_erase function
template<class Container, class Value>
void/*Container&*/ remove_erase(Container& target, const Value& value) {
  target.erase(std::remove(target.begin(), target.end(), value), target.end());
  //return target;
}

} // namespace sr
