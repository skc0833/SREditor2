#include "UIFont.h"
#include "../Win32/WIN32FontInfo.h"
#include <sstream> // for wostringstream

namespace sr {

//static
std::unordered_map<UIFontInfo, UIFontPtr, UIFontInfoHash, UIFontInfoEqual> UIFont::_fontCache;

UIFont::UIFont(const SRString& name, SRFloat size) {
  _fontInfo = WIN32FontInfo::create(name, size);
}

SRUInt UIFont::hash() const {
  SR_ASSERT(0);
  using std::hash;
  using std::wstring;
  return hash<wstring>()(fontInfo()->_fontName.data()) ^ (hash<sr::SRFloat>()(fontInfo()->_size) << 1);
}

bool UIFont::isEqual(const SRObject& obj) const {
  const auto& other = static_cast<const UIFont&>(obj);
  return (*_fontInfo == *(other._fontInfo));
}

/* e.g,
    NSBackgroundColor = "UIDeviceWhiteColorSpace 0 1";
    NSFont = "<UICTFont: 0x7fd3e2f80ec0> font-family: \".HelveticaNeueInterface-MediumP4\"; font-weight: bold; font-style: normal; font-size: 12.00pt";
*/
SRStringUPtr UIFont::toString() const {
  std::wostringstream ss;
  ss << L"<UIFont: " << this << L">";
  ss << L" font-family: " << fontInfo()->_familyName.c_str() << L";";
  ss << L" font-weight: " << fontInfo()->_weight << L";";
  ss << L" font-size: " << fontInfo()->_size << L"pt;";
  ss << L" traits: " << fontInfo()->_traits << L";";
  // ...
  return std::make_unique<SRString>(ss.str());
}

//static
UIFontPtr UIFont::fontWithName(const SRString& name, SRFloat size) {
  // WIN32FontInfo �� Ű�� ����ϰ� �Ǹ�, �Ź� WIN32FontInfo ��ü ������ ����� ���� ��� �ȴ�(���� ��Ʈ���� ������)
  UIFontInfo key(name, size);

  auto it = _fontCache.find(key);
  if (it != _fontCache.end()) {
    // ��Ʈ ĳ���� �����ϸ� �̸� ����
    UIFontPtr font = it->second; // std::static_pointer_cast<UIFont>(_fontCache.at(key)) �� ����
    return font;
  } else {
    // �ƴϸ� ���� �����Ͽ� ��Ʈ ĳ���� ���� �� ����
    UIFontPtr value = UIFont::create(name, size);
    std::pair<std::unordered_map<UIFontInfo, UIFontPtr>::iterator, bool> result = _fontCache.insert(
      std::pair<UIFontInfo, UIFontPtr>(key, value));
    SR_ASSERT(result.second);
    return value;
  }
}

//static
SRFloat UIFont::systemFontSize() {
  return WIN32FontInfo::systemFontSize();
}

//static
SRString UIFont::systemFontName() {
  return WIN32FontInfo::systemFontName();
}

//static
UIFontPtr UIFont::systemFont(SRFloat size) {
  auto ptr = UIFont::fontWithName(systemFontName(), size);
  return ptr;
}

//static
void UIFont::test() {
  static bool __first = true;
  if (__first) {
    // _fontCache.size() �񱳸� ���� ���ʿ� �ѹ��� �׽�Ʈ��(���߿� ��Ʈ�� �����ɼ� �����Ƿ�)
    __first = false;
    SR_ASSERT(_fontCache.size() == 0);
    UIFontPtr font1 = UIFont::fontWithName(L"Courier New", 14.0);
    UIFontPtr font2 = UIFont::fontWithName(L"Arial");
    SR_ASSERT(_fontCache.size() == 2);
    UIFontPtr font3 = UIFont::fontWithName(L"Courier New", UIFont::systemFontSize() + 2.0f); // 14
    SR_ASSERT(_fontCache.size() == 2);
    UIFontPtr font4 = UIFont::fontWithName(L"Courier New"); // UIFont::systemFontSize() -> 12
    SR_ASSERT(_fontCache.size() == 3);
  }
}

} // namespace sr
