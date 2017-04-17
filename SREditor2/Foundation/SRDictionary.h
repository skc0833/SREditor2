#pragma once

#include "SRObject.h"
#include "SRString.h"
#include <unordered_map>
#include <sstream> // for wostringstream

namespace sr {

// SRStringPtr �� ����ϸ� �ּҰ��� ���ϰ� �Ǿ� ������ SRStringPtr ��ü�� �˻��ؾ߸� �˻��̵ȴ�(���� ���ڿ��� �˻� �ȵ�)
template<class KeyType, class ValueType> class SRDictionary;

template<class KeyType, class ValueType>
using SRDictionaryPtr = std::shared_ptr<SRDictionary<KeyType, ValueType>>;

template<class KeyType, class ValueType>
using _SRDictionaryMap = std::unordered_map<KeyType, ValueType>;

// [����] https://github.com/apple/swift-corelibs-foundation/blob/cfff4135acfbbfe9ac4f280be21ca0a13ddfdb0d/CoreFoundation/Collections.subproj/CFDictionary.c
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
  // ���� NSDictionary������ valueForKey()�� Ű�� ���ڿ��� ���, objectForKey()�� Ű�� Object�� ��� ����
  ValueType valueForKey(const KeyType& key, ValueType defautValue = nullptr) const;

  // �ش�Ű�� �����ϸ� �����ϰ�, �ƴϸ� �߰��Ѵ�(add if absent, replace if present)
  // Adds a given key-value pair to the dictionary.
  void setValue(ValueType value, const KeyType& key);

  // �ش�Ű�� �������� ������ �߰��ϰ�, �ƴϸ� �ƹ� ������ ���Ѵ�(add if absent)
  void addValue(const KeyType& key, ValueType value);

  // replace if present, else does nothing
  //void replaceValue(SRString key, ValueType value);

  // dict�� �Ӽ����� �߰��Ѵ�.
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
