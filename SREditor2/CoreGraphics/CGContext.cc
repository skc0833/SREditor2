#include "CGContext.h"

namespace sr {

CGContext::CGContext(CGContextImplPtr impl) {
  _backing = impl;
  _debugDraw = false;
}

CGContext::~CGContext() {
}

SRRect CGContext::applyCTM(SRRect rect) const {
  rect.origin += SRPoint(getTextMatrix().tx, getTextMatrix().ty);
  return rect;
}

SRPoint CGContext::applyCTM(SRPoint pt) const {
  pt += SRPoint(getTextMatrix().tx, getTextMatrix().ty);
  return pt;
}

SRPoint CGContext::getTextPosition() const {
  return _curState._textPosition;
}

void CGContext::setTextPosition(SRFloat x, SRFloat y) {
  //_textPosition = applyCTM(SRPoint(x, y));
  _curState._textPosition = { x, y };
  //return _backing->setTextPosition(_textPosition.x, _textPosition.y);
}

void CGContext::setFont(UIFontPtr font) {
  return _backing->setFont(font);
}

void CGContext::setFontSize(SRFloat size) {
  return _backing->setFontSize(size);
}

void CGContext::setLineWidth(SRFloat width) {
  _backing->setLineWidth(width);
}

SRFloat CGContext::getLineWidth() {
  return _backing->getLineWidth();
}

void CGContext::setFillColor(SRColor cr) {
  _backing->setFillColor(cr);
}

void CGContext::setStrokeColor(SRColor cr) {
  _backing->setStrokeColor(cr);
}

void CGContext::setStrokePattern(SRPattern pattern) {
  _backing->setStrokePattern(pattern);
}

void CGContext::setFillPattern(SRPattern pattern) {
  _backing->setFillPattern(pattern);
}

// Changes the origin of the user coordinate system in a context.
// @param x The amount to displace the x - axis of the coordinate space, in units of the user space, of the specified context.
// @param y The amount to displace the y - axis of the coordinate space, in units of the user space, of the specified context.
void CGContext::translateBy(SRFloat x, SRFloat y) {
  //_ctm.translatedBy(x, y);
  _curState._ctm.translatedBy(x, y);
}

void CGContext::drawRect(SRRect rect) {
  rect = applyCTM(rect);
  //SRRect r3 = r2.intersection(_curState._clip);
  if (rect.right() >= _curState._clip.right() || rect.bottom() >= _curState._clip.bottom()) {
    int a = 0; // @@ 이런 경우는 없어야겠다!!!
    //rc.size.width = _curState._clip.right() - rc.left();
    //rc.size.height = _curState._clip.bottom() - rc.top();
  }
  _backing->drawRect(rect);
  //_backing->drawRect(applyCTM(rc));
}

void CGContext::drawRect(SRPattern pattern, SRColor cr, SRRect rect) {
  _backing->setStrokePattern(pattern);
  _backing->setStrokeColor(cr);
  drawRect(rect);
}

void CGContext::fillRect(SRRect rect) {
  _backing->fillRect(applyCTM(rect));
}

void CGContext::fillRect(SRColor cr, SRRect rect) {
  _backing->setFillColor(cr);
  fillRect(rect);
}

void CGContext::polyFillOutlined(const std::vector<SRPoint>& points, SRColor crFill, SRColor crOutline) {
  SR_NOT_IMPL(); // applyCTM() 필요
  _backing->polyFillOutlined(points, crFill, crOutline);
}

void CGContext::polyLine(const std::vector<SRPoint>& points, SRColor cr) {
  SR_NOT_IMPL(); // applyCTM() 필요
  _backing->polyLine(points, cr);
}

void CGContext::showGlyphsAtPoint(SRPoint pos, const SRColor& color, const SRGlyph *glyphs, size_t count) {
  pos = applyCTM(pos) + _curState._textPosition;
  _backing->showGlyphsAtPoint(pos.x, pos.y, color, glyphs, count);
}

void CGContext::drawImage(const SRString& path, SRRect rect) {
  rect = applyCTM(rect);
  rect.origin += _curState._textPosition; // 이미지도 글리프로 처리하고 있으므로 _textPosition 을 적용함
  _backing->drawImage(path, rect);
}

void CGContext::setViewportOrg(SRFloat x, SRFloat y, SRPoint* prev) {
  SRPoint pos = applyCTM(SRPoint(x, y));
  _backing->setViewportOrg(pos.x, pos.y, prev);
}

SRPoint CGContext::getViewportOrg() {
  return _backing->getViewportOrg();
}

void CGContext::intersectClipRect(const SRRect& rect) {
  //_backing->intersectClipRect(applyCTM(rect));
  SRRect clip = rect;
  if (_curState._clip.width() > 0 && _curState._clip.height() > 0) {
    clip = rect.intersection(_curState._clip);
  }
  if (rect != clip) {
    int a = 0;
  }
  setClipBox(clip);
}

void CGContext::setClipBox(const SRRect& rect) {
  _curState._clip = rect;
  _backing->setClipBox(applyCTM(rect)); // 필요한가???
}

SRRect CGContext::getClipBox() const {
  SRRect rc = _backing->getClipBox();
  SRRect rc2 = applyCTM(_curState._clip);
  if (rc != rc2) {
    //SR_ASSERT(0);
    int a = 0;
  }
  return _curState._clip;
}

void* CGContext::getSrcDC() const {
  return _backing->getSrcDC();
}

void CGContext::flush(void* destDC, SRRect* rcDest, SRPoint* ptSrc) const {
  _backing->flush(destDC, rcDest, ptSrc);
}

void CGContext::saveGState() {
  //_savedStates.push({ getCTM(), getClipBox() });
  _savedStates.push(_curState);
}

void CGContext::restoreGState() {
  GState state = _savedStates.top();
  //setCTM(state._ctm);
  setClipBox(state._clip);
  _curState = state;
  _savedStates.pop();
}

} // namespace sr
