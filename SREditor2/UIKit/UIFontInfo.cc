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

// UIFont::_fontCache.find() 내에서 호출되며, 키로 사용했던 _fontName, _size 값만 비교하게 처리함
bool UIFontInfo::isEqual(const SRObject& obj) const {
  const auto& other = static_cast<const UIFontInfo&>(obj);
  if (hash() != other.hash()) return false; // fast check
  if (_fontName != other._fontName || _size != other._size || _weight != other._weight) {
    SR_ASSERT(0); // 위에 hash() 비교시에 return false 됐어야 한다.
    return false;
  }
  return true;
}

} // namespace sr
