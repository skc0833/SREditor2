#if 0
#include "SRParagraphStyle.h"
#include "SRString.h"
#include <sstream> // for wostringstream

namespace sr {

SRParagraphStyle::SRParagraphStyle(SRFloat headIndent) {
  _headIndent = headIndent;
}

SRParagraphStyle::SRParagraphStyle(const SRParagraphStyle& rhs) {
  *this = rhs;
}

SRParagraphStyle::~SRParagraphStyle() {
  //delete _data;
}

void SRParagraphStyle::dump(const wchar_t* indent) {
  SR_LOG_DUMP(L"SRParagraphStyle(%d): _headIndent=%f", id(), _headIndent);
}

// e.g, "<UICTFont: 0x7fd3e2f80ec0> font-family: \".HelveticaNeueInterface-MediumP4\"; font-weight: bold; font-style: normal; font-size: 12.00pt"
SRStringUPtr SRParagraphStyle::toString() {
  std::wostringstream ss;
  ss << L"<Paragraph: 0x" << this << L"> head-indent: " << _headIndent;
  SRStringUPtr ptr = std::unique_ptr<SRString>(new SRString(ss.str()));
  return ptr;
}

SRParagraphStyle& SRParagraphStyle::operator=(const SRParagraphStyle& rhs) {
  if (this == &rhs) {
    return *this;
  }
  SRObject::operator=(rhs);
  return *this;
}

bool SRParagraphStyle::operator==(const SRObject& rhs) const {
  if (this == &rhs) {
    return true;
  }
  if (SRObject::operator!=(rhs)) return false;
  const SRParagraphStyle& other = (SRParagraphStyle&)rhs;
  if (_headIndent != other._headIndent) return false;
  return true;
}

bool SRParagraphStyle::operator!=(const SRObject& rhs) const {
  return (!(*this == rhs));
}

} // namespace sr
#endif
