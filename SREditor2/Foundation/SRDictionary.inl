
template<class KeyType, class ValueType>
SRDictionary<KeyType, ValueType>::SRDictionary() {
}

template<class KeyType, class ValueType>
SRDictionary<KeyType, ValueType>::SRDictionary(const SRDictionary<KeyType, ValueType>& rhs) {
  //SR_ASSERT(0);
  const _SRDictionaryMap<KeyType, ValueType>& rhsData = rhs.data();
  // !!!주의!!! _map 자체는 복사되고 있지만, _map 내의 항목들 주소는 동일한 상태임
  // deep copy 가 아니므로 사용하는 곳에서 조심해야 함
  for (const auto& item : rhs.data()) {
    KeyType key = item.first;
    ValueType value = item.second; // needs clone() ???
    setValue(value, key);
  }
}

template<class KeyType, class ValueType>
SRDictionary<KeyType, ValueType>::~SRDictionary() {
}

/* e.g,
    NSBackgroundColor = "UIDeviceWhiteColorSpace 0 1";
    NSFont = "<UICTFont: 0x7fd3e2f80ec0> font-family: \".HelveticaNeueInterface-MediumP4\"; font-weight: bold; font-style: normal; font-size: 12.00pt";
*/
template<class KeyType, class ValueType>
SRStringUPtr SRDictionary<KeyType, ValueType>::toString() const {
  std::wostringstream ss;
  const std::wstring indent = L"  ";
  ss << indent;
  ss << L"<SRDictionary: " << this << L">" << std::endl;
  for (const auto& item : _map) {
    KeyType key = item.first;
    ValueType value = item.second;
    auto k = key.toString();
    auto v = value->toString(); // SRStringUPtr
    ss << indent;
    ss << k->c_str() << L" = \"" << v->c_str() << L"\";" << std::endl;
  }
  return std::unique_ptr<SRString>(new SRString(ss.str()));
}

template<class KeyType, class ValueType>
SRDictionary<KeyType, ValueType>& SRDictionary<KeyType, ValueType>::operator=(SRDictionary<KeyType, ValueType> rhs) {
  std::swap(_map, rhs._map);
  return *this;
}

template<class KeyType, class ValueType>
ValueType SRDictionary<KeyType, ValueType>::valueForKey(const KeyType& key, ValueType defautValue) const {
  //ValueType value = _map[key]; // std::map::operator[] 는 해당키가 없으면 추가한다.
  _SRDictionaryMap<KeyType, ValueType>::const_iterator it = _map.find(key);
  if (it != _map.end()) { // 해당 키 존재여부는 _map.count(key) > 0 으로도 확인 가능함
    ValueType val = it->second; // _map.at(key) 과 동일함, at() 함수는 미존재시 out_of_range exception 가 발생됨
    return val;
  }
  return defautValue;
}

// add if absent. 해당키가 이미 존재하면 아무 동작을 안한다.
template<class KeyType, class ValueType>
void SRDictionary<KeyType, ValueType>::addValue(const KeyType& key, ValueType value) {
  //map[key] = value; // error C2678: 이항 '=' : 왼쪽 피연산자로 'const std::tr1::shared_ptr<_Ty>' 형식을 사용하는 연산자가 없거나 허용되는 변환이 없습니다.
  std::pair<_SRDictionaryMap<KeyType, ValueType>::iterator, bool> ret = _map.insert(std::pair<KeyType, ValueType>(key, value));
  if (ret.second == false) {
    // ret.second 는 insert() 성공 여부임. 실패한 경우는 이미 동일한 키가 존재하는 경우이며, 추가되지 않음
    ValueType oldValue = ret.first->second;
    if (*oldValue != *value) {
      SR_ASSERT(0); // 기존 값과 다른 경우도 존재하나?
    }
    //oldValue->dump();
  }
}

// add if absent, replace if present
template<class KeyType, class ValueType>
void SRDictionary<KeyType, ValueType>::setValue(ValueType value, const KeyType& key) {
  _SRDictionaryMap<KeyType, ValueType>::iterator it = _map.find(key);
  if (it != _map.end()) {
    _map.erase (it);
  }
  addValue(key, value);
}

template<class KeyType, class ValueType>
void SRDictionary<KeyType, ValueType>::removeValueForKey(const KeyType& key) {
  _SRDictionaryMap<KeyType, ValueType>::iterator it = _map.find(key);
  if (it != _map.end()) {
    _map.erase (it);
  } else {
    SR_ASSERT(0);
  }
}

template<class KeyType, class ValueType>
void SRDictionary<KeyType, ValueType>::addMulitiple(const SRDictionaryPtr<KeyType, ValueType> dict, bool replaceExisting) {
  const _SRDictionaryMap<KeyType, ValueType>& rhs = dict->data();
  for (const auto& item : rhs) {
    // rhs 의 항목들을 모두 추가
    KeyType key = item.first;
    ValueType value = item.second;
    //SR_LOGD(L"%s() key=%s, value=0x%p", __FUNCTION__, key.c_str(), value);
    if (containsKey(key)) {
      //SR_ASSERT(0); // 존재하나???
      int a = 0;
    }
    if (replaceExisting) {
      setValue(value, key); // 해당키가 존재하면 수정하고, 아니면 추가한다.
    } else {
      addValue(key, value); // 해당키가 존재하지 않으면 추가하고, 아니면 do nothing.
    }
  }
}

template<class KeyType, class ValueType>
bool SRDictionary<KeyType, ValueType>::containsKey(const KeyType& key) {
  _SRDictionaryMap<KeyType, ValueType>::iterator it = _map.find(key);
  if (it != _map.end()) {
    return true;
  } else {
    return false;
  }
}

template<class KeyType, class ValueType>
bool SRDictionary<KeyType, ValueType>::containsValue(const ValueType& value) {
  SR_ASSERT(0);
  return false;
}
