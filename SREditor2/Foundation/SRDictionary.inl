
template<class KeyType, class ValueType>
SRDictionary<KeyType, ValueType>::SRDictionary() {
}

template<class KeyType, class ValueType>
SRDictionary<KeyType, ValueType>::SRDictionary(const SRDictionary<KeyType, ValueType>& rhs) {
  //SR_ASSERT(0);
  const _SRDictionaryMap<KeyType, ValueType>& rhsData = rhs.data();
  // !!!����!!! _map ��ü�� ����ǰ� ������, _map ���� �׸�� �ּҴ� ������ ������
  // deep copy �� �ƴϹǷ� ����ϴ� ������ �����ؾ� ��
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
  //ValueType value = _map[key]; // std::map::operator[] �� �ش�Ű�� ������ �߰��Ѵ�.
  _SRDictionaryMap<KeyType, ValueType>::const_iterator it = _map.find(key);
  if (it != _map.end()) { // �ش� Ű ���翩�δ� _map.count(key) > 0 ���ε� Ȯ�� ������
    ValueType val = it->second; // _map.at(key) �� ������, at() �Լ��� ������� out_of_range exception �� �߻���
    return val;
  }
  return defautValue;
}

// add if absent. �ش�Ű�� �̹� �����ϸ� �ƹ� ������ ���Ѵ�.
template<class KeyType, class ValueType>
void SRDictionary<KeyType, ValueType>::addValue(const KeyType& key, ValueType value) {
  //map[key] = value; // error C2678: ���� '=' : ���� �ǿ����ڷ� 'const std::tr1::shared_ptr<_Ty>' ������ ����ϴ� �����ڰ� ���ų� ���Ǵ� ��ȯ�� �����ϴ�.
  std::pair<_SRDictionaryMap<KeyType, ValueType>::iterator, bool> ret = _map.insert(std::pair<KeyType, ValueType>(key, value));
  if (ret.second == false) {
    // ret.second �� insert() ���� ������. ������ ���� �̹� ������ Ű�� �����ϴ� ����̸�, �߰����� ����
    ValueType oldValue = ret.first->second;
    if (*oldValue != *value) {
      SR_ASSERT(0); // ���� ���� �ٸ� ��쵵 �����ϳ�?
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
    // rhs �� �׸���� ��� �߰�
    KeyType key = item.first;
    ValueType value = item.second;
    //SR_LOGD(L"%s() key=%s, value=0x%p", __FUNCTION__, key.c_str(), value);
    if (containsKey(key)) {
      //SR_ASSERT(0); // �����ϳ�???
      int a = 0;
    }
    if (replaceExisting) {
      setValue(value, key); // �ش�Ű�� �����ϸ� �����ϰ�, �ƴϸ� �߰��Ѵ�.
    } else {
      addValue(key, value); // �ش�Ű�� �������� ������ �߰��ϰ�, �ƴϸ� do nothing.
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
