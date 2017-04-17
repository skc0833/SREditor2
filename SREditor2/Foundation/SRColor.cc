#include "SRColor.h"
#include "SRString.h"
#include <sstream> // for wostringstream

//typedef unsigned long       DWORD;
//typedef DWORD   COLORREF;
//#define RGB(r,g,b)          ((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((DWORD)(BYTE)(b))<<16)))

namespace sr {

SRColor::SRColor(SRFloat r, SRFloat g, SRFloat b, SRFloat a) {
  _r = r;
  _g = g;
  _b = b;
  _a = a;
}

SRColor::SRColor(const SRColor& rhs) {
  _r = rhs._r;
  _g = rhs._g;
  _b = rhs._b;
  _a = rhs._a;
}

bool SRColor::isEqual(const SRObject& obj) const {
  const auto& other = static_cast<const SRColor&>(obj);
  if (_r != other._r || _g != other._g || _b != other._b || _a != other._a) {
    return false;
  }
  return true;
}

SRStringUPtr SRColor::toString() const {
  std::wostringstream ss;
  ss << L"<SRColor: " << this << L">";
  ss << L" " << _r << L", " << _g << L", " << _b << L", " << _a << L";";
  return std::make_unique<SRString>(ss.str());
}

} // namespace sr
