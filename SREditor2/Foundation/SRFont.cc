#include "SRFont.h"
#include "SRString.h"
#include <sstream> // for wostringstream

namespace sr {

SRFont::SRFont(SRString fontName) {
  _fontName = fontName;
}

SRFont::SRFont(const SRFont& rhs) {
  *this = rhs;
}

SRFont::~SRFont() {
}

void SRFont::dump(const wchar_t* indent) {
  SR_LOG_DUMP(L"SRFont(%d): _fontName=%s", id(), _fontName.c_str());
}

// e.g, "<UICTFont: 0x7fd3e2f80ec0> font-family: \".HelveticaNeueInterface-MediumP4\"; font-weight: bold; font-style: normal; font-size: 12.00pt"
SRStringUPtr SRFont::toString() {
  std::wostringstream ss;
  ss << L"<Font: 0x" << this << L"> font-family: '" << _fontName.c_str() << L"'";
  //ss << L"<Font: 0x";
  //ss << this;
  //ss << L"> font-family: '";
  //ss << _fontName.c_str();
  //ss << L"'";
  //std::wstring str = ss.str();

  SRStringUPtr ptr = std::unique_ptr<SRString>(new SRString(ss.str()));
  return ptr;
}

SRFont& SRFont::operator=(const SRFont& rhs) {
  if (this == &rhs) {
    return *this;
  }
  SRObject::operator=(rhs);
  return *this;
}

bool SRFont::operator==(const SRObject& rhs) const {
  if (this == &rhs) {
    return true;
  }
  if (SRObject::operator!=(rhs)) return false;
  return true;
}

bool SRFont::operator!=(const SRObject& rhs) const {
  return (!(*this == rhs));
}

} // namespace sr
