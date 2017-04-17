#pragma once

#include "SRObject.h"
#include "UIFontInfo.h"
#include <unordered_map>

namespace sr {

class UIFont;
using UIFontPtr = std::shared_ptr<UIFont>;

struct UIFontInfoHash {
  std::size_t operator()(const sr::UIFontInfo& k) const {
    using std::hash;
    using std::wstring;
    return hash<wstring>()(k._fontName.data()) ^ (hash<sr::SRFloat>()(k._size) << 1);
  }
};
struct UIFontInfoEqual {
  std::size_t operator()(const sr::UIFontInfo& lhs, const sr::UIFontInfo& rhs) const {
    if (lhs._fontName != rhs._fontName || lhs._size != rhs._size) return false;
    else return true;
  }
};

class UIFont : public SRObject {
  SR_MAKE_NONCOPYABLE(UIFont);
public:
  SR_DECL_CREATE_FUNC(UIFont);

  // size가 0 일 경우, UIFontInfo() 생성자에서 UIFont::systemFontSize() 크기로 생성중임
  UIFont(const SRString& name, SRFloat size = 0.0f);
  virtual ~UIFont() = default;

  virtual SRUInt hash() const override;
  virtual SRStringUPtr toString() const override;

  SRFloat ascender() const { return fontInfo()->_ascender; }
  SRFloat descender() const { return fontInfo()->_descender; }
  SRFloat lineHeight() const { return (ascender() - descender()); }
  SRFloat advancementForGlyph(SRGlyph glyph) const { return fontInfo()->advancementForGlyph(glyph); }

  UIFontInfoPtr fontInfo() const { return _fontInfo; }

  // Creating Fonts
  //
  static UIFontPtr fontWithName(const SRString& name, SRFloat size = 0.0f);
  static SRFloat systemFontSize();
  static SRString systemFontName(); // 별도로 추가함(UIFontInfo 생성자에서 호출됨)
  static UIFontPtr systemFont(SRFloat size = 0.0f);

  static void test();

protected:
  virtual bool isEqual(const SRObject& obj) const override;

private:
  UIFontInfoPtr _fontInfo;

  // UIFontInfo 키 생성시 사용한 멤버(_fontName, _size)만 비교하기 위해 비교용 functor 를 따로 명시함
  // functor 누락시에는 UIFontInfo::isEqual() 가 호출됨
  static std::unordered_map<UIFontInfo, UIFontPtr, UIFontInfoHash, UIFontInfoEqual> _fontCache;
};
} // namespace sr

// custom specialization of std::hash can be injected in namespace std
template <>
struct std::hash<sr::UIFont> {
  std::size_t operator()(const sr::UIFont& k) const {
    SR_ASSERT(0);
    return k.hash(); // _fontCache.find() 내에서 호출됨. UIFont::hash()를 호출
  }
};

template <>
struct std::equal_to<sr::UIFont> {
  bool operator()(const sr::UIFont& lhs, const sr::UIFont& rhs) const {
    SR_ASSERT(0);
    return lhs == rhs; // _fontCache.find() 내에서 호출됨
  }
};
