#include "SRDictionary.h"

namespace sr {

// SRObjectDictionary 는 SRDictionary<const SRString, SRObjectPtr> 타입으로 Value 가 포인터이므로, 
// 사용하는 곳에서 Value 변경시 조심해야 한다!!!
// (다른 키도 동일 Value 를 소유할 경우 같이 변경됨. Value 변경전에 복사가 필요할 수 있다. COW 개념 염두)
SRObjectDictionary::SRObjectDictionary() {
}

SRObjectDictionary::SRObjectDictionary(const SRObjectDictionary& rhs)
  : SRDictionary<const SRString, SRObjectPtr>(rhs) {
}

bool SRObjectDictionary::isEqual(const SRObject& obj) const {
  const auto& other = static_cast<const SRObjectDictionary&>(obj);
  const _SRDictionaryMap<const SRString, SRObjectPtr>& lhs = data();
  const _SRDictionaryMap<const SRString, SRObjectPtr>& rhs = other.data();
  if (lhs.size() != rhs.size())
    return false;  // differing sizes, they are not the same

  _SRDictionaryMap<SRString, SRObjectPtr>::const_iterator i, j;
  for (i = lhs.begin(), j = rhs.begin(); i != lhs.end(); ++i, ++j) {
    if (i->first != j->first) {
      return false;
    }
    if (*(i->second) != *(j->second)) {
      return false;
    }
  }
  return true;
}

} // namespace sr
