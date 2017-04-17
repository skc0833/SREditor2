#include "CGContextImpl.h"

namespace sr {

CGContextImpl::CGContextImpl() {
  _curTextPosition.x = _curTextPosition.y = 0;
  _curFont = nullptr;
  _curFontSize = 0;
}

CGContextImpl::~CGContextImpl() {
}

SRPoint CGContextImpl::getTextPosition() {
  return SRPoint(_curTextPosition.x, _curTextPosition.y);
}

void CGContextImpl::setTextPosition(SRFloat x, SRFloat y) {
  _curTextPosition.x = x;
  _curTextPosition.y = y;
}

void CGContextImpl::setFont(UIFontPtr font) {
  _curFont = font;
}

void CGContextImpl::setFontSize(SRFloat size) {
  _curFontSize = size;
}

} // namespace sr
