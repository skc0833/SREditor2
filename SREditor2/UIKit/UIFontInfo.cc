#include "UIFontInfo.h"
#include "UIFont.h"

namespace sr {

UIFontInfo::UIFontInfo(const SRString& fontName, const SRFloat size) {
  _ascender = _descender = 0.0f;
  _weight = 0;
  _traits = 0;
  _isFixedPitch = false;
  _size = size;
  if (_size <= 0) {
    _size = UIFont::systemFontSize();
  }
  if (fontName.length() > 0) {
    _fontName = fontName;
  } else {
    SR_LOGE(L"Warning: Font name is nil, fall back to system font!!!");
    _fontName = UIFont::systemFontName();
  }
}

SRUInt UIFontInfo::hash() const {
  using std::hash;
  using std::wstring;
  SRUInt result = hash<wstring>()(_fontName.data())
    ^ (hash<sr::SRFloat>()(_size) << 1)
    ^ (hash<sr::SRInt>()(_weight) << 2);
  return result;
}

// UIFont::_fontCache.find() ������ ȣ��Ǹ�, Ű�� ����ߴ� _fontName, _size ���� ���ϰ� ó����
bool UIFontInfo::isEqual(const SRObject& obj) const {
  const auto& other = static_cast<const UIFontInfo&>(obj);
  if (hash() != other.hash()) return false; // fast check
  if (_fontName != other._fontName || _size != other._size || _weight != other._weight) {
    SR_ASSERT(0); // ���� hash() �񱳽ÿ� return false �ƾ�� �Ѵ�.
    return false;
  }
  return true;
}

} // namespace sr
