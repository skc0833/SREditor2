#pragma once

#include "SRObject.h"
#include "SRString.h"

namespace sr {

enum {
  SRItalicFontMask = 1,
  SRBoldFontMask = 2,
  SRUnboldFontMask = 4,
  //SRNonStandardCharacterSetFontMask = 8,
  //SRNarrowFontMask = 16,
  //SRExpandedFontMask = 32,
  //SRCondensedFontMask = 64,
  //SRSmallCapsFontMask = 128,
  //SRPosterFontMask = 256,
  //SRCompressedFontMask = 512,
  //SRFixedPitchFontMask = 1024,
  SRUnitalicFontMask = 1 << 24
};
typedef SRUInt SRFontTraitMask;

class UIFontInfo;
using UIFontInfoPtr = std::shared_ptr<UIFontInfo>;

class UIFontInfo : public SRObject {
  //SR_MAKE_NONCOPYABLE(UIFontInfo); // UIFont::_fontCache �� �߰��� Ű�� ����ǹǷ� ��������� �ʿ���
public:
  UIFontInfo(const SRString& fontName, const SRFloat size = 0.0);
  virtual ~UIFontInfo() = default;

  //UIFontInfo(UIFontInfo&&) = default; // ��������� ���� �ʿ�� �����ؼ� �������!
  //UIFontInfo& operator=(UIFontInfo&&) = default;
  UIFontInfo(const UIFontInfo&) = default;    // default ����(*this = rhs;)�� ���(deep copy �ʿ�����Ƿ�)
  UIFontInfo& operator=(const UIFontInfo&) = default; // ���� ����

  virtual SRUInt hash() const override;

  virtual SRFloat advancementForGlyph(SRGlyph glyph) {
    SR_ASSERT(0); // override �ž� �Ѵ�.
    return 0.0f;
  }
  virtual void* getDeviceFontInfo() {
    SR_ASSERT(0);
    return nullptr;
  }

protected:
  virtual bool isEqual(const SRObject& obj) const override;

public:
  //// metrics of the font
  //NSString *fontName;
  //NSString *familyName;
  //SRFloat matrix[6];
  //SRFloat italicAngle;
  //SRFloat underlinePosition;
  //SRFloat underlineThickness;
  //SRFloat capHeight;
  //SRFloat xHeight; // baseline �� �ҹ����� mean line(baseline�� cap height�� ����)���� �Ÿ�. ���� x ������ ���̿� ����.
  //SRFloat descender;
  //SRFloat ascender;
  //NSSize maximumAdvancement;
  //NSSize minimumAdvancement;
  //NSString *encodingScheme;
  //NSStringEncoding mostCompatibleStringEncoding;
  //NSRect fontBBox;
  //BOOL isFixedPitch;
  //BOOL isBaseFont;
  //int weight;
  //NSFontTraitMask traits;
  //unsigned numberOfGlyphs;
  //NSCharacterSet *coveredCharacterSet;
  //NSFontDescriptor *fontDescriptor;

  SRString _fontName;
  SRString _familyName; // _fontName �ڿ��� " Italic" ���� ���̻縦 �� ���ڿ���
  SRFloat _ascender;
  SRFloat _descender;
  SRFloat _size;

  // not used
  SRInt _weight;
  SRFontTraitMask _traits;
  SRBool _isFixedPitch;
};

} // namespace sr

template <>
struct std::hash<sr::UIFontInfo> {
  std::size_t operator()(const sr::UIFontInfo& k) const {
    return k.hash(); // UIFont::_fontCache.find() ������ ȣ���
  }
};

template <>
struct std::equal_to<sr::UIFontInfo> {
  bool operator()(const sr::UIFontInfo& lhs, const sr::UIFontInfo& rhs) const {
    return lhs == rhs; // UIFont::_fontCache.find() ������ ȣ���
  }
};