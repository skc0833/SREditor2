#pragma once

#include "SRObject.h"
#include "SRString.h"
#include <unordered_map>
#include <sstream> // for wostringstream

namespace sr {

// SRStringPtr 을 사용하면 주소값을 비교하게 되어 동일한 SRStringPtr 객체로 검색해야만 검색이된다(동일 문자열로 검색 안됨)
template<class KeyType, class ValueType> class SRDictionary;

template<class KeyType, class ValueType>
using SRDictionaryPtr = std::shared_ptr<SRDictionary<KeyType, ValueType>>;

template<class KeyType, class ValueType>
using _SRDictionaryMap = std::unordered_map<KeyType, ValueType>;

// [참고] https://github.com/apple/swift-corelibs-foundation/blob/cfff4135acfbbfe9ac4f280be21ca0a13ddfdb0d/CoreFoundation/Collections.subproj/CFDictionary.c
template<class KeyType, class ValueType>
class SRDictionary : public SRObject {
public:
  SRDictionary();
  SRDictionary(const SRDictionary<KeyType, ValueType>& rhs);
  ~SRDictionary();

  virtual SRDictionary<KeyType, ValueType>& operator=(SRDictionary<KeyType, ValueType> rhs);

  virtual SRStringUPtr toString() const override;

  const _SRDictionaryMap<KeyType, ValueType>& data() const {
    return _map;
  }
  SRIndex count() const {
    return _map.size();
  }

  // Removes a given key and its associated value from the dictionary.
  // remove if present, else does nothing
  void removeValueForKey(const KeyType& key);
  //void removeAllValues();

  // Returns the value associated with a given key.
  // 원래 NSDictionary에서는 valueForKey()는 키가 문자열일 경우, objectForKey()는 키가 Object일 경우 사용됨
  ValueType valueForKey(const KeyType& key, ValueType defautValue = nullptr) const;

  // 해당키가 존재하면 수정하고, 아니면 추가한다(add if absent, replace if present)
  // Adds a given key-value pair to the dictionary.
  void setValue(ValueType value, const KeyType& key);

  // 해당키가 존재하지 않으면 추가하고, 아니면 아무 동작을 안한다(add if absent)
  void addValue(const KeyType& key, ValueType value);

  // replace if present, else does nothing
  //void replaceValue(SRString key, ValueType value);

  // dict의 속성들을 추가한다.
  void addMulitiple(const SRDictionaryPtr<KeyType, ValueType> dict, bool replaceExisting = true);

  bool containsKey(const KeyType& key);
  bool containsValue(const ValueType& value);

//protected:
//  virtual bool isEqual(const SRObject& obj) const override;

private:
  _SRDictionaryMap<KeyType, ValueType> _map;
};

#include "SRDictionary.inl"

class SRObjectDictionary;
using SRObjectDictionaryPtr = std::shared_ptr<SRObjectDictionary>;

class SRObjectDictionary : public SRDictionary<const SRString, SRObjectPtr> {
  //SR_MAKE_NONCOPYABLE(SRObjectDictionary);
public:
  SR_DECL_CREATE_FUNC(SRObjectDictionary);

  SRObjectDictionary();
  SRObjectDictionary(const SRObjectDictionary& rhs);
  virtual ~SRObjectDictionary() = default;

protected:
  virtual bool isEqual(const SRObject& obj) const override;
};

} // namespace sr
