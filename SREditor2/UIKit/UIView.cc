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
  // layout �� _needsLayout �÷��׸� ����
  SR_NOT_IMPL();
  _needsLayout = false;
}

void UIView::setNeedsLayout() {
  // ���� update cycle ����� �ƴϹǷ�, �ٷ� ���̾ƿ����ϰ� ó����
  SR_NOT_IMPL();
  _needsLayout = true;
  layoutIfNeeded();
}

// ��������� ���̾ƿ����Ѵ�. �� �Լ��� ���� ȣ������ ���� layoutIfNeeded()�� setNeedsLayout()�� ȣ���� ����ض�!
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

  // �߰��Ǵ� �ڽĺ並 �θ��κ��� _layoutMargins.left, top ��ŭ ������ ��ġ�� ��ġ��Ų��.
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
  // �θ���� _subviews���� ���� �並 �����Ѵ�.
  UIViewList& subviews = superview()->_subviews;
  //sr::remove_erase(superview()->_subviews, this);
  subviews.erase(std::remove_if(subviews.begin(), subviews.end(), [this](UIViewPtr v) {
    if (v.get() == this) {
    }
    return v.get() == this;
  }), subviews.end());
}

// ����� �������� ��ǥ�� to �� �������� ��ǥ�� ��ȯ��(local coordinate system -> to view's)
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
  // �������� point�� ������ǥ�̹Ƿ� getBounds().origin�� �̹� ����� �־�� �Ѵ�.
  point += getFrame().origin;
  //point -= getBounds().origin;

  UIViewPtr parent = superview();
  while (parent) { // ���� �� ���� point ��ǥ�� window coordinate �� ���Ѵ�.
    point += parent->getFrame().origin;
    parent = parent->superview();
  }
  if (to) {
    SRPoint point2 = to->getFrame().origin;
    parent = to->superview();
    while (parent) { // to �� ���� origin ��ǥ�� window coordinate �� ���Ѵ�.
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

  // ���� ��ǥ��� viewport �̵�(Changes the origin of the user coordinate system in a context.)
  ctx->translateBy(getFrame().origin.x, getFrame().origin.y);

  // Ŭ������ ����� ���� �並 ������ ������� �� �ִ�(������ Ŭ���� ������ ���� ��쵵 ������)
  // ������ 
  // Sets the clipping path to the intersection of the current clipping path with the area 
  // defined by the specified rectangle.
  // ����� �׳� Ŭ���� �������� �����ϰ� ó����
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

  // ��ũ�� ����
  ctx->translateBy(getBounds().origin.x, getBounds().origin.y);

  // ���, ��輱 �׸���
  drawBackground(ctx, SRRect({ 0, 0 }, getBounds().size));
  drawBorder(ctx, SRRect({ 0, 0 }, getBounds().size));

  // ���� _layoutManager->drawBackgroundForGlyphRange() �� drawGlyphsForGlyphRange() ���

  // drawRect ��ǥ(������ ��ǥ)�� ��ũ�� ����(������ ��ǥ ������ �׷��� ������ ��)
  drawRect = drawRect.offsetBy(-getBounds().origin.x, -getBounds().origin.y);

  for (const auto& v : _subviews) {
    SRRect frame = v->getFrame();
    SRRect rc = frame.intersection(drawRect);
    if (!rc.isEmpty()) {
      // drawRect(���� �䳻�� ����)�� �ڽĺ� ���� �� ��ġ�� ������ ������ ��쿡�� ������Ѵ�.
      rc = convertTo(rc, v);
      v->draw(ctx, rc);
    } else {
      // ��ġ�� ������ �����Ƿ� skip!
      //SR_ASSERT(0);
      int a = 0;
    }
  }
  endDraw(ctx);
}

// TODO: apply transform
SRRect UIView::getFrame() const {
  // _center, _bounds �κ��� ������ ���� �����
  // it turns out that frame is not backed by any variable, but rather is just synthesized 
  // out of the bounds and center variables (and the transform if it's non-identity).
  // �������� bounds �� center�κ��� ���ȴ�(transform �� identity �� �ƴϸ� transform �� �����)
  SRFloat left = _center.x - _bounds.width() / 2;
  SRFloat top = _center.y - _bounds.height() / 2;
  SRRect frame(left, top, _bounds.width(), _bounds.height());
  if (frame != _frame) { // �Ҽ��� ���ϰ� �ȸ´� ��찡 ������(int Ÿ������ �ٲ���߰ڴ�)
    int a = 0;
  }
  return frame;
}

// need to set only _center, _bounds
bool UIView::setFrame(const SRRect& frame) {
  // when you change the frame, you're really changing the bounds and center; 
  // if you change the center, you��re updating both the center and the frame.
  // �������� ������ bounds �� center ���� �����̸�, center �� �����ϸ� center �� ������ ��ΰ� ������ �ǰ� �ȴ�.
  if (getFrame() == frame) {
    return true; // ������ �����ϹǷ� �� �̻� ó������ ����
  }
  SRSize prevSize = getFrame().size; // ���ʿ� 0 �ϼ� �ִ�.
  if (prevSize.width < 0 || prevSize.height < 0 || frame.width() < 0 || frame.height() < 0) {
    // ������ �ּ�ȭ�ÿ��� frame.width/height() �� 0 ���� �����Ѵ�.
    SR_ASSERT(0);
  }

  SRSize delta(frame.width() - prevSize.width, frame.height() - prevSize.height);

  //_bounds.origin = SRPoint(0, 0); // ��ũ�� ��ġ�� ������Ŵ
  _bounds.size = frame.size; // _bounds.origin�� ��ũ�� ��ġ��
  _center.x = frame.left() + frame.width() / 2;
  _center.y = frame.top() + frame.height() / 2;

  _frame = frame;

  adjustSubviews(prevSize, delta); // ���� �������� ũ��� �����Ǵ� ũ�� ����
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
  // ������� �߾��� �θ���� center ��ġ�� �����Ѵ�.
  const SRPoint center = superview()->getAlignCenter();
  SRRect frame = getFrame();
  frame.origin.x = center.x - frame.size.width / 2;
  if (frame.origin.x < 0) {
    // frame.origin ��ǥ�� (0, 0) �̻����� ������(���� ���� ũ�⺸�� �����찡 ���� �����)
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
    // ���� ������ ���� ������ ���� ������
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
      // pos�� 0�� ���, ����¡�� �ص� �ѵ��� pos�� ��� 0�� �Ǵ� ��찡 ������(�� ��� right margin�� �����)
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
    // ������ ������ ���� ������ ���� ������
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
    // ���� ������ ���� ������ ���� ������
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
    // ���콺 �̺�Ʈ�� ��쿡�� ���� �� �̳��� ������Ŵ
    SRPoint orgPoint = point;
    // _layoutMargins ���� hit �� ���� �信�� ���޽�Ŵ(e.g, SRDocumentView(20) -> SRPageView(10))
    // right, bottom ��ġ�� width - 1, height - 1 ��ġ��
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
      // �ڽĺ�� ������ ��� ���, �ڽĺ�� �̳��� ���ѽ�Ŵ(���� ��ǥ��)
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

  // ���� �䳻������ ��ǥ�� ȹ��
  //point -= getFrame().origin;

  // needs isUserInteractionEnabled() ?
  if (pointInside(point)) {
    SRPoint fitPoint = fitToContent(event); // _layoutMargins, ���� �ڽĺ�� �̳��� ����
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
