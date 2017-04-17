#pragma once

#include "../UIKit/UIFontInfo.h"

namespace sr {

class WIN32FontInfo;
using WIN32FontInfoPtr = std::shared_ptr<WIN32FontInfo>;

class WIN32FontInfo final : public UIFontInfo { // WIN32FontInfo 로부터 더이상 상속을 방지함(final)
  SR_MAKE_NONCOPYABLE(WIN32FontInfo);
public:
  SR_DECL_CREATE_FUNC(WIN32FontInfo);

  WIN32FontInfo(const SRString& fontName, const SRFloat size = 0.0);
  virtual ~WIN32FontInfo();

  //virtual SRUInt hash() const override;

  virtual SRFloat advancementForGlyph(SRGlyph glyph);
  virtual void* getDeviceFontInfo() { return _hFont; }

  static SRString systemFontName() { return SRString(L"굴림체"); } // Helvetica
  static SRFloat systemFontSize() { return 12; }

//protected:
//  virtual bool isEqual(const SRObject& obj) const override;

private:
  SRBool setupAttributes();
  void* _hFont; // for win32
};

} // namespace sr
