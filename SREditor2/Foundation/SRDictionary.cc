#include "SRDictionary.h"

namespace sr {

// SRObjectDictionary �� SRDictionary<const SRString, SRObjectPtr> Ÿ������ Value �� �������̹Ƿ�, 
// ����ϴ� ������ Value ����� �����ؾ� �Ѵ�!!!
// (�ٸ� Ű�� ���� Value �� ������ ��� ���� �����. Value �������� ���簡 �ʿ��� �� �ִ�. COW ���� ����)
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
