#include "CGAffineTransform.h"
#include "SRString.h"
#include <sstream> // for wostringstream

namespace sr {

CGAffineTransform::CGAffineTransform() {
  //a = d = 1;
  //b = c = tx = ty = 0;
  tx = ty = 0;
}

SRStringUPtr CGAffineTransform::toString() const {
  std::wostringstream ss;
  ss << L"<CGAffineTransform: " << this << L">";
  //ss << a << L" ";
  //ss << b << L" ";
  //ss << c << L" ";
  //ss << d << L" ";
  ss << tx << L" ";
  ss << ty << L" ";
  ss << L";";
  return std::make_unique<SRString>(ss.str());
}

// tx, ty 만큼 이동
void CGAffineTransform::translatedBy(SRFloat tx, SRFloat ty) {
  this->tx += tx;
  this->ty += ty;
}

} // namespace sr
