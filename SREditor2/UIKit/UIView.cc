#include "UIView.h"
#include "UIWindow.h"
#include "../CoreGraphics/CGContext.h"
#include <algorithm>
#include <windows.h> // for ::SetCursor
#undef max
#undef min

namespace sr {

UIView::UIView(const SRRect& frame)
 : _autoresizingMask(UIViewAutoresizingNone), _backgroundColor(1.0f, 1.0f, 1.0f)
  , _borderColor(0.0f, 0.0f, 0.0f), _borderWidth(0.0f), _needsLayout(false), _autoCenterInSuperview(false) {
  setFrame(frame);
}

UIView::~UIView() {
}

bool UIView::isEqual(const SRObject& obj) const {
  SR_ASSERT(0);
  return SRObject::isEqual(obj);
}

void UIView::layoutIfNeeded() {
  // layout 후 _needsLayout 플래그를 끈다
  SR_NOT_IMPL();
  _needsLayout = false;
}

void UIView::setNeedsLayout() {
  // 현재 update cycle 방식이 아니므로, 바로 레이아웃팅하게 처리함
  SR_NOT_IMPL();
  _needsLayout = true;
  layoutIfNeeded();
}

// 하위뷰들을 레이아웃팅한다. 이 함수를 직접 호출하지 말고 layoutIfNeeded()나 setNeedsLayout()를 호출해 사용해라!
void UIView::layoutSubviews() {
  SR_NOT_IMPL();
}

void UIView::addSubview(const UIViewPtr view) {
  view->removeFromSuperview();
  view->_superview = this->shared_from_this();
  _subviews.push_back(view);

  // set window
  SR_ASSERT(window());
  view->setWindow(window());

  // 추가되는 자식뷰를 부모뷰로부터 _layoutMargins.left, top 만큼 떨어진 위치에 위치시킨다.
  SRPoint o = view->getFrame().origin;
  if (o.x < _layoutMargins.left) {
    o.x += _layoutMargins.left;
  }
  if (o.y < _layoutMargins.top) {
    o.y += _layoutMargins.top;
  }
  view->setOrigin(o);
}

void UIView::removeSubviews() {
  _subviews.clear();
}

void UIView::removeFromSuperview() {
  if (superview() == nullptr || superview()->_subviews.empty()) {
    return;
  }
  // 부모뷰의 _subviews에서 현재 뷰를 삭제한다.
  UIViewList& subviews = superview()->_subviews;
  //sr::remove_erase(superview()->_subviews, this);
  subviews.erase(std::remove_if(subviews.begin(), subviews.end(), [this](UIViewPtr v) {
    if (v.get() == this) {
    }
    return v.get() == this;
  }), subviews.end());
}

// 현재뷰 내에서의 좌표를 to 뷰 내에서의 좌표로 변환함(local coordinate system -> to view's)
// rect: A rectangle specified in the local coordinate system (bounds) of the receiver.
SRRect UIView::convertTo(SRRect rect, UIViewPtr to) const {
  //rect.origin += getFrame().origin;
  rect.origin = convertTo(rect.origin, to);
  return rect;
}

SRPoint UIView::convertTo(SRPoint point, UIViewPtr to) const {
  if (typeid(*this) == typeid(UIWindow)) {
    int a = 0;
  }
  // 전달인자 point는 로컬좌표이므로 getBounds().origin는 이미 적용돼 있어야 한다.
  point += getFrame().origin;
  //point -= getBounds().origin;

  UIViewPtr parent = superview();
  while (parent) { // 현재 뷰 내의 point 좌표의 window coordinate 를 구한다.
    point += parent->getFrame().origin;
    parent = parent->superview();
  }
  if (to) {
    SRPoint point2 = to->getFrame().origin;
    parent = to->superview();
    while (parent) { // to 뷰 내의 origin 좌표의 window coordinate 를 구한다.
      point2 += parent->getFrame().origin;
      parent = parent->superview();
    }
    point -= point2; // convert point to to view's coordinate system
    //point += point2; // test
  }
  return point;
}

void UIView::beginDraw(CGContextPtr ctx) const {
  ctx->saveGState();

  // 로컬 좌표계로 viewport 이동(Changes the origin of the user coordinate system in a context.)
  ctx->translateBy(getFrame().origin.x, getFrame().origin.y);

  // 클리핑을 해줘야 현재 뷰를 온전히 드로잉할 수 있다(이전에 클리핑 영역이 작은 경우도 존재함)
  // 원래는 
  // Sets the clipping path to the intersection of the current clipping path with the area 
  // defined by the specified rectangle.
  // 현재는 그냥 클리핑 영역으로 설정하게 처리함
  ctx->setClipBox(SRRect({ 0, 0 }, getBounds().size));
}

void UIView::endDraw(CGContextPtr ctx) const {
  ctx->restoreGState();
}

void UIView::drawBackground(CGContextPtr ctx, SRRect rect) const {
  ctx->fillRect(backgroundColor(), rect);
}

void UIView::drawBorder(CGContextPtr ctx, SRRect rect) const {
  if (borderWidth() > 0) {
    SRFloat oldLineWidth = ctx->getLineWidth();
    SRFloat border = borderWidth();
    ctx->setLineWidth(border);
    ctx->drawRect(SR_PS_INSIDEFRAME, borderColor(), rect);
    ctx->setLineWidth(oldLineWidth); // must restore previous value
  }
}

void UIView::draw(CGContextPtr ctx, SRRect drawRect) const {
  beginDraw(ctx);

  // 스크롤 적용
  ctx->translateBy(getBounds().origin.x, getBounds().origin.y);

  // 배경, 경계선 그리기
  drawBackground(ctx, SRRect({ 0, 0 }, getBounds().size));
  drawBorder(ctx, SRRect({ 0, 0 }, getBounds().size));

  // 추후 _layoutManager->drawBackgroundForGlyphRange() 와 drawGlyphsForGlyphRange() 고려

  // drawRect 좌표(윈도우 좌표)에 스크롤 적용(윈도우 좌표 내에서 그려질 영역이 됨)
  drawRect = drawRect.offsetBy(-getBounds().origin.x, -getBounds().origin.y);

  for (const auto& v : _subviews) {
    SRRect frame = v->getFrame();
    SRRect rc = frame.intersection(drawRect);
    if (!rc.isEmpty()) {
      // drawRect(현재 뷰내의 영역)와 자식뷰 영역 중 겹치는 영역이 존재할 경우에만 드로잉한다.
      rc = convertTo(rc, v);
      v->draw(ctx, rc);
    } else {
      // 겹치는 영역이 없으므로 skip!
      //SR_ASSERT(0);
      int a = 0;
    }
  }
  endDraw(ctx);
}

// TODO: apply transform
SRRect UIView::getFrame() const {
  // _center, _bounds 로부터 프레임 영역 계산함
  // it turns out that frame is not backed by any variable, but rather is just synthesized 
  // out of the bounds and center variables (and the transform if it's non-identity).
  // 프레임은 bounds 와 center로부터 계산된다(transform 이 identity 가 아니면 transform 도 적용됨)
  SRFloat left = _center.x - _bounds.width() / 2;
  SRFloat top = _center.y - _bounds.height() / 2;
  SRRect frame(left, top, _bounds.width(), _bounds.height());
  if (frame != _frame) { // 소숫점 이하가 안맞는 경우가 존재함(int 타입으로 바꿔줘야겠다)
    int a = 0;
  }
  return frame;
}

// need to set only _center, _bounds
bool UIView::setFrame(const SRRect& frame) {
  // when you change the frame, you're really changing the bounds and center; 
  // if you change the center, you’re updating both the center and the frame.
  // 프레임을 변경은 bounds 와 center 값의 변경이며, center 를 변경하면 center 와 프레임 모두가 변경이 되게 된다.
  if (getFrame() == frame) {
    return true; // 영역이 동일하므로 더 이상 처리하지 않음
  }
  SRSize prevSize = getFrame().size; // 최초에 0 일수 있다.
  if (prevSize.width < 0 || prevSize.height < 0 || frame.width() < 0 || frame.height() < 0) {
    // 윈도우 최소화시에는 frame.width/height() 가 0 으로 진입한다.
    SR_ASSERT(0);
  }

  SRSize delta(frame.width() - prevSize.width, frame.height() - prevSize.height);

  //_bounds.origin = SRPoint(0, 0); // 스크롤 위치는 유지시킴
  _bounds.size = frame.size; // _bounds.origin는 스크롤 위치임
  _center.x = frame.left() + frame.width() / 2;
  _center.y = frame.top() + frame.height() / 2;

  _frame = frame;

  adjustSubviews(prevSize, delta); // 현재 프레임의 크기와 변동되는 크기 전달
  return false;
}

SRRect UIView::getBounds() const {
  return _bounds;
}

SRRect UIView::getContentsRect() const {
  return _bounds;
}

void UIView::setSize(const SRSize& size) {
  SRRect frame(getFrame().origin, size);
  setFrame(frame);
}

void UIView::setOrigin(const SRPoint& origin) {
  SRRect frame(origin, getFrame().size);
  setFrame(frame);
}

SRPoint UIView::getAlignCenter() const {
  return _center;
}

void UIView::alignCenter() {
  // 현재뷰의 중앙을 부모뷰의 center 위치로 설정한다.
  const SRPoint center = superview()->getAlignCenter();
  SRRect frame = getFrame();
  frame.origin.x = center.x - frame.size.width / 2;
  if (frame.origin.x < 0) {
    // frame.origin 좌표를 (0, 0) 이상으로 설정함(현재 뷰의 크기보다 윈도우가 작은 경우임)
    //SR_ASSERT(0);
    frame.origin.x = 0; // getFrame().origin.x;
  }
  frame.origin.y = center.y - frame.size.height / 2;
  if (frame.origin.y < 0) {
    //SR_ASSERT(0);
    frame.origin.y = 0; // getFrame().origin.y;
  }
  setFrame(frame);
}

void UIView::sizeToFit() {
  SRSize size = sizeThatFits();
  size.width += _layoutMargins.right;
  size.height += _layoutMargins.bottom;
  setSize(size);
}

SRSize UIView::sizeThatFits(SRSize size) {
  SRRect totalRect;
  for (const auto& view : _subviews) {
    totalRect += view->getFrame();
  }
  return totalRect.size;
}

static void doResize(unsigned mask, float& pos, float& size, float parentSize, float delta) {
  switch (mask) {
  case UIViewAutoresizingNone: // 0
    break;

  case UIViewAutoresizingFlexibleLeftMargin:
    pos += delta;
    break;

  case UIViewAutoresizingFlexibleWidth:
    size += delta;
    break;

  case (UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleWidth): {
    // 왼쪽 마진과 폭이 비율에 따라 조절됨
    float totalSize = 0.0f;
    float newSize;

    totalSize += pos;
    totalSize += size;
    newSize = totalSize + delta;

    if (totalSize == 0.0f) {
      pos += size;
    } else {
      float newPos = pos * newSize / totalSize;
      size = ((pos + size) * newSize / totalSize) - newPos;
      pos = newPos;
    }
  } break;

  case UIViewAutoresizingFlexibleRightMargin:
    break;

  case (UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin): {
    float totalSize = 0.0f;
    float newSize;

    totalSize += pos; // left margin
    totalSize += parentSize - (pos + size); // right margin
    newSize = totalSize + delta; // flexible portions = totalSize(left margin + right margin) + delta
    //SR_LOGD(L"+++ pos=%.1f, size=%.1f, parentSize=%.1f, delta=%.1f", pos, size, parentSize, delta);

    // distribute the size difference propotionally among the flexible portions.
    if (totalSize == 0.0f) {
      pos = newSize / 2.0f; // if no margins left, just distribute equally(this view aligned center in parent view)
    } else {
      // pos가 0인 경우, 리사징을 해도 한동안 pos가 계속 0이 되는 경우가 존재함(이 경우 right margin만 변경됨)
      pos = pos * newSize / totalSize; // set pos according to delta
    }
	  if (pos == 0) {
      // workaround: fix after pos set to 0, it keeps 0(only right margin moves)
      //pos = newSize / 2.0f;
	  }
    //SR_LOGD(L"--- pos=%.1f, totalSize=%.1f, newSize=%.1f", pos, totalSize, newSize);
    break;
  }

  case (UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleWidth): {
    // 오른쪽 마진과 폭이 비율에 따라 조절됨
    float totalSize = 0.0f;
    float lastSize = parentSize - (pos + size);
    float newSize;

    totalSize += size;
    totalSize += lastSize;
    newSize = totalSize + delta;

    if (lastSize <= 0.0f || size == 0.0f) {
      size = parentSize + delta - pos;
    } else {
      float newPos = pos * newSize / totalSize;
      size = ((pos + size) * newSize / totalSize) - newPos;
    }
  } break;

  case (UIViewAutoresizingFlexibleLeftMargin | UIViewAutoresizingFlexibleRightMargin | UIViewAutoresizingFlexibleWidth): {
    // 양쪽 마진과 폭이 비율에 따라 조절됨
    float totalSize = 0.0f;
    float lastSize = parentSize - (pos + size);
    float newSize;

    totalSize += pos;
    totalSize += size;
    totalSize += lastSize;
    newSize = totalSize + delta;

    if (totalSize == 0.0f) {
      pos = (parentSize + delta) / 3.0f;
      size = (parentSize + delta) / 3.0f;
    } else {
      float newPos = pos * newSize / totalSize;
      size = ((pos + size) * newSize / totalSize) - newPos;
      pos = newPos;
    }
  } break;

  default:
    SR_ASSERT(0);
    break;
  }
}

void UIView::adjustSubviews(SRSize parentSize, SRSize delta) {
  //SR_LOGD(L">>> UIView::adjustSubviews() parentSize=%.1f-%.1f, delta=%.1f-%.1f", parentSize.width, parentSize.height, delta.width, delta.height);
  for (auto it = _subviews.begin(); it != _subviews.end(); ++it) {
    const UIViewPtr& view = *it;
    SRRect childFrame = view->getFrame();
    SRUInt mask = view->getAutoresizingMask();

    //SR_LOGD(L"+++ childFrame.origin.x=%.1f, childFrame.size.width=%.1f", childFrame.origin.x, childFrame.size.width);
    if (view->getAutoCenterInSuperview()) {
      view->alignCenter();
      childFrame = view->getFrame();
    } else {
      doResize(mask & 7, childFrame.origin.x, childFrame.size.width, parentSize.width, delta.width);
      doResize((mask >> 3) & 7, childFrame.origin.y, childFrame.size.height, parentSize.height, delta.height);
    }
    //SR_LOGD(L"--- childFrame.origin.x=%.1f, childFrame.size.width=%.1f", childFrame.origin.x, childFrame.size.width);

    SR_ASSERT(childFrame.size.width > 0.0f && childFrame.size.height > 0.0f);
    view->setFrame(childFrame);
  }
}

void UIView::addConstraint(const UILayoutConstraint& constraint) {
  SR_NOT_IMPL();
  //_constraints.push_back(constraint);
}

SRPoint UIView::fitToContent(UIEvent event) const {
  SRPoint point = event.mouseLocation();
  if (event.isMouseEvent()) {
    // 마우스 이벤트일 경우에만 현재 뷰 이내로 보정시킴
    SRPoint orgPoint = point;
    // _layoutMargins 영역 hit 시 안쪽 뷰에게 전달시킴(e.g, SRDocumentView(20) -> SRPageView(10))
    // right, bottom 위치는 width - 1, height - 1 위치임
    point.x = std::max(point.x, _layoutMargins.left);
    point.x = std::min(point.x, (getBounds().size.width - 1) - _layoutMargins.right);
    point.y = std::max(point.y, _layoutMargins.top);
    point.y = std::min(point.y, (getBounds().size.height - 1) - _layoutMargins.bottom);
    if (orgPoint != point) {
      //if (!event.isMouseEvent()) {
      //  int a = 0;
      //}
      int a = 0;
    }
    if (getSubviews().size() > 0) {
      // 자식뷰들 범위를 벗어난 경우, 자식뷰들 이내로 제한시킴(상하 좌표만)
      SRPoint orgPoint2 = point;
      auto head = getSubviews().front();
      auto tail = getSubviews().back();
      point.y = std::max(point.y, head->getFrame().top());
      point.y = std::min(point.y, tail->getFrame().bottom() - 1);
      if (orgPoint2 != point) {
        int a = 0;
        SR_ASSERT(0);
      }
    }
  }
  return point;
}

// point: A point specified in the receiver's local coordinate system (bounds).
const UIViewPtr UIView::hitTest(UIEvent event) const {
  const SRPoint& point = event._xy;

  // 현재 뷰내에서의 좌표를 획득
  //point -= getFrame().origin;

  // needs isUserInteractionEnabled() ?
  if (pointInside(point)) {
    SRPoint fitPoint = fitToContent(event); // _layoutMargins, 상하 자식뷰들 이내로 보정
    for (const auto& subview : _subviews) {
      SRPoint convertedPoint = convertTo(fitPoint, subview); // have to pass as subview's local coordinate system
      event._xy = convertedPoint;
      const UIViewPtr hitTestView = subview->hitTest(event);
      if (hitTestView) {
        return hitTestView;
      }
    }
    //const UIViewPtr thisView = getPtr<UIView>(this);
    //std::shared_ptr<const UIView> thisView = this->shared_from_this(); // ok!
    const UIViewPtr ptr = std::const_pointer_cast<UIView>(this->shared_from_this());
    return ptr;
  }
  return nullptr;
}

// used to determine which subview should receive a touch event.
// point: A point that is in the receiver's local coordinate system (bounds).
SRBool UIView::pointInside(SRPoint point) const {
  SRRect bounds({ 0, 0 }, getBounds().size); // local area
  return bounds.containsPoint(point);
}

void UIView::mouseDown(UIEvent event) {
  SR_ASSERT(0);
}

void UIView::mouseUp(UIEvent event) {
  SR_ASSERT(0);
}

void UIView::mouseMoved(UIEvent event) {
  SR_ASSERT(0);
}

void UIView::keyDown(UIEvent event) {
  SR_ASSERT(0);
}

void UIView::keyUp(UIEvent event) {
  SR_ASSERT(0);
}

void UIView::cursorUpdate(UIEvent event) {
  //SR_LOGD(L"[cursor] UIView::cursorUpdate() IDC_ARROW");
  ::SetCursor(::LoadCursor(NULL, IDC_ARROW));
}

void UIView::scrollWheel(UIEvent event) {
  SR_ASSERT(0);
}

UITextPosition UIView::closestPosition(SRPoint point, UITextLayoutDirection direction, SRTextPosition position) const {
  SR_ASSERT(0);
  return UITextPosition();
}

SRRect UIView::caretRect(UITextPosition position) const {
  SR_ASSERT(0);
  for (const auto& item : _subviews) {
    SRRect r = item->caretRect(position);
    if (r.size.width > 0) {
      return r;
    }
  }
  SR_ASSERT(0);
  return SRRect();
}

void UIView::replace(UITextRange range, const SRString& text) const {
  SR_ASSERT(0);
}

UITextPosition UIView::position(UITextPosition from, UITextLayoutDirection direction, SRIndex offset) const {
  SR_ASSERT(0);
  return UITextPosition();
}

UITextPosition UIView::position(UITextRange range, UITextLayoutDirection farthest) const {
  SR_ASSERT(0);
  return UITextPosition();
}

} // namespace sr
