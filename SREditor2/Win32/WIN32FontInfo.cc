#include "WIN32FontInfo.h"
#include <windows.h>

namespace sr {

WIN32FontInfo::WIN32FontInfo(const SRString& fontName, const SRFloat size)
  : UIFontInfo(fontName, size) {
  setupAttributes(); // _fontName, _size 로부터 나머지 멤버값들이 설정됨
}

WIN32FontInfo::~WIN32FontInfo() {
  if (_hFont) {
    ::DeleteObject(_hFont);
  }
}

//SRUInt WIN32FontInfo::hash() const {
//  return UIFontInfo::hash();
//}
//
//bool WIN32FontInfo::isEqual(const SRObject& obj) const {
//  return UIFontInfo::isEqual(obj);
//}

static int win32_font_weight(LONG tmWeight) {
  int weight;

  // The MS names are a bit different from the NS ones!
  switch (tmWeight) {
  case FW_THIN:
    weight = 1;
    break;
  case FW_EXTRALIGHT:
    weight = 2;
    break;
  case FW_LIGHT:
    weight = 3;
    break;
  case FW_REGULAR:
    weight = 5;
    break;
  case FW_MEDIUM:
    weight = 6;
    break;
  case FW_DEMIBOLD:
    weight = 7;
    break;
  case FW_BOLD:
    weight = 9;
    break;
  case FW_EXTRABOLD:
    weight = 10;
    break;
  case FW_BLACK:
    weight = 12;
    break;
  default:
    // Try to map the range 0 to 1000 into 1 to 14.
    weight = (int)(tmWeight * 14 / 1000);
    break;
  }
  return weight;
}

//const SRString fontStyles[] = { L" Italic", L" Oblique", L" Bold", L" BoldItalic", L" Demibold", L" Normal", L" Kursiv", L" Fett"};
const SRString fontStyles[] = { L" Italic", L" Bold" };

// fontName 의 끝에서 fontStyles 문자열을 제외한 문자열을 리턴
static SRString _win32_font_family(const SRString& fontName) {
  SRString fontFamily;
  int i;
  int max = sizeof(fontStyles) / sizeof(fontStyles[0]);

  fontFamily = fontName;
  for (i = 0; i < max; i++) {
    if (fontFamily.hasSuffix(fontStyles[i])) {
      fontFamily = *fontFamily.substring(fontFamily.length() - fontStyles[i].length());
    }
  }
  SR_LOGD(L"Font Family '%s' for '%s'", fontFamily.c_str(), fontName.c_str());
  return fontFamily;
}

SRBool WIN32FontInfo::setupAttributes() {
  _familyName = _win32_font_family(_fontName);

  HDC hdc = ::CreateCompatibleDC(NULL);

  LOGFONTW logfont;
  memset(&logfont, 0, sizeof(LOGFONT));

  // For the MM_TEXT mapping mode, you can use the following formula to specify a height for a font with a specified point size:
  // https://msdn.microsoft.com/ko-kr/library/windows/desktop/dd145037(v=vs.85).aspx
  // ::ChooseFont() 함수로 테스트해보면 사용자가 선택한 폰트크기로 획득한 lfHeight 값이 아래 식과 동일한 결과가 리턴됨
  logfont.lfHeight = -MulDiv(_size, ::GetDeviceCaps(hdc, LOGPIXELSY), 72); // _size(14) -> -19

  SRRange range;
  range = _fontName.range(L"Bold");
  if (range.length)
    logfont.lfWeight = FW_BOLD;

  range = _fontName.range(L"Italic");
  if (range.length)
    logfont.lfItalic = 1;

  logfont.lfQuality = DEFAULT_QUALITY;
  ::wcsncpy(logfont.lfFaceName, _familyName, LF_FACESIZE);

  _hFont = ::CreateFontIndirectW(&logfont);
  if (!_hFont) {
    SR_LOGE(L"Could not create font %s", _fontName.c_str());
    SR_ASSERT(0);
    ::DeleteDC(hdc);
    return false;
  }

  HFONT old = (HFONT)::SelectObject(hdc, _hFont);
  TEXTMETRICW metric;
  ::GetTextMetricsW(hdc, &metric);
  ::SelectObject(hdc, old);
  ::DeleteDC(hdc);

  _isFixedPitch = TMPF_FIXED_PITCH & metric.tmPitchAndFamily;
  
  _ascender = metric.tmAscent;
  _descender = -metric.tmDescent;

  //_xHeight = _ascender * 0.5;
  //_maximumAdvancement = SRSize((float)metric.tmMaxCharWidth, 0.0);
  //_fontBBox = SRRect((float)(0),
  //  (float)(0 - metric.tmAscent), // @@TODO 음수로 설정되고 있는데 맞나???
  //  (float)metric.tmMaxCharWidth,
  //  (float)metric.tmHeight);

  _weight = win32_font_weight(metric.tmWeight);
  _traits = 0;
  if (_weight >= 9) 
    _traits |= SRBoldFontMask;
  else
    _traits |= SRUnboldFontMask;

  if (metric.tmItalic)
    _traits |= SRItalicFontMask;
  else
    _traits |= SRUnitalicFontMask;

  // FIXME Should come from metric.tmCharSet
  //mostCompatibleStringEncoding = SRISOLatin1StringEncoding;

  return true;
}

SRFloat WIN32FontInfo::advancementForGlyph(SRGlyph glyph) {
  wchar_t u = (wchar_t)glyph;
  ABCFLOAT abc;

  HDC hdc = ::CreateCompatibleDC(NULL);
  HFONT old = (HFONT)::SelectObject(hdc, _hFont);
  ::GetCharABCWidthsFloatW(hdc, u, u, &abc);
  ::SelectObject(hdc, old);
  ::DeleteDC(hdc);

  SRFloat w = abc.abcfA + abc.abcfB + abc.abcfC;
  return w;
}

} // namespace sr
