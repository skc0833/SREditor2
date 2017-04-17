#include "RenderObject.h"
#include <algorithm>

namespace sr {

static const SRColor crBorderLight(0.5, 0.5, 0.5);
static const SRColor crBorderDark(0, 0, 0);

static int __render_cnt = 0;
int __sr_debug_render_cnt() {
  return __render_cnt;
}

RenderObject::RenderObject(RenderObjectPtr parent, SRRect rect)
  : _parent(parent), _margin(0.0f), _border(0.0f), _padding(0.0f) {
  setRect(rect);
  ++__render_cnt;
}

RenderObject::~RenderObject() {
  --__render_cnt;
}

SRPoint RenderObject::fitToContent(SRPoint point, UITextLayoutDirection direction) const {
  // ���� ������ ������ ���� ���� Ŭ���� ������ ���� �̳��� ���ѽ�Ŵ
  SR_ASSERT(_margin == 0); // _margin > 0 �� ���� �׽�Ʈ �ʿ���
  SRPoint orgPoint = point;
  SRRect rect = { { 0, 0 }, getRect().size };
  rect = rect.insetBy(_margin + _border + _padding, _margin + _border + _padding);
  point.x = std::max(point.x, rect.left());
  point.x = std::min(point.x, rect.right() - 1);
  point.y = std::max(point.y, rect.top());
  point.y = std::min(point.y, rect.bottom() - 1);
  if (orgPoint != point) {
    int a = 0;
  }

  if (_children.size() > 0) {
    // �ڽ� ������ ������ ��� ���, �ڽ� ������ �̳��� ���ѽ�Ŵ(���� ��ǥ��)
    // e.g, ���̺� �� �ϴܿ� ������ �����ϰ� ���⸦ Ŭ���� �� ������ ĳ���� ������
    orgPoint = point;
    const auto& head = _children.front();
    const auto& tail = _children.back();
    point.y = std::max(point.y, head->getRect().top());
    point.y = std::min(point.y, tail->getRect().bottom() - 1);
    if (orgPoint != point) {
      int a = 0;
    }
  }
  return point;
}

SRPoint RenderObject::convertTo(SRPoint point, RenderObjectPtr to) const {
  point += getRect().origin;
  RenderObjectPtr parentObj = parent();
  while (parentObj) { // find point's window coordinate
    point += parentObj->getRect().origin;
    parentObj = parentObj->parent();
  }
  if (to) {
    SRPoint point2 = to->getRect().origin;
    parentObj = to->parent();
    while (parentObj) { // find to view's origin window coordinate
      point2 += parentObj->getRect().origin;
      parentObj = parentObj->parent();
    }
    point -= point2; // convert point to to view's coordinate system
  }
  return point;
}

const RenderObjectPtr RenderObject::hitTest(SRPoint point, UITextLayoutDirection direction) {
  if (pointInside(point)) {
    // ���� ���� �̳��� Ŭ���� ���
    SRPoint orgPoint = point;
    point = fitToContent(point, direction); // ������ ���� �̳��� ����
    if (_children.size() == 0) {
      // �ڽ� ������ ���� ���, ���� ������ ����
      const std::shared_ptr<RenderObject> thisObj = this->shared_from_this(); // ok!
      return thisObj;
    }
    for (const auto& item : _children) {
      SRPoint convertedPoint = convertTo(point, item); // convert local coordinate
      const RenderObjectPtr hitTestObj = item->hitTest(convertedPoint, direction);
      if (hitTestObj) {
        return hitTestObj;
      }
    }
    // �ڽ� ������ Ŭ������ ���� ���, nullptr �� ���Ͻ�Ŵ(border, padding ������ Ŭ���� �����)
    return nullptr;
    //const std::shared_ptr<RenderObject> thisObj = this->shared_from_this(); // ok!
    //return thisObj;
  }
  return nullptr;
}

SRBool RenderObject::pointInside(SRPoint point) const {
  SRRect rect({0, 0}, getRect().size);
  return rect.containsPoint(point);
}

RenderObjectPtr RenderObject::getRenderByIndex(SRTextPosition position) {
  const RenderObjectList& children = getChildren();
  const SRIndex offset = position._offset;
  if (children.size() == 0) {
    if (!(offset >= getRange().location && offset <= getRange().rangeMax())) {
      SR_ASSERT(0);
    }
    const std::shared_ptr<RenderObject> thisObj = this->shared_from_this(); // ok!
    return thisObj;
  }
  auto it = std::find_if(children.begin(), children.end(), [&position](RenderObjectPtr obj) {
    SRRange range = obj->getRange();
    const SRIndex offset = position._offset;
    //if (position._stickToNextLine) {
    if (true) {
      // ���� ������ ���� ���� _stickToNextLine�� true�� �����ؾ� ���� ������ ã�� �ʰ� �ȴ�.
      // TODO position ��ġ�� ���� �� ��ġ�� ���� ó���� �� �ɵ���(Ȯ�� �ʿ�)
      // |a$|b -> 2nd line b(1-1) �տ� offset(1) ĳ�� -> 1 >= 1 && 1 < 2
      if (offset >= range.location && offset < range.rangeMax())
        return true;
    } else {
      // |a|$b -> 1st line a(0-1) �ڿ� offset(1) ĳ�� -> 1 > 0 && 1 <= 1
      if (offset > range.location && offset <= range.rangeMax())
        return true;
    }
    if (offset == 0 && range.location >= 0) {
      SR_ASSERT(0); // �̷� ��찡 �����ϳ�???
      return true;
    }
    return false;
  });
  SR_ASSERT(it != children.end());
  RenderObjectPtr obj = *it;
  return obj->getRenderByIndex(position);
}

RenderObjectPtr RenderObject::findRender(UITextLayoutDirection direction, SRTextPosition position) {
  if (direction == up) {
    if (_prev.lock()) {
      return _prev.lock();
    }
  } else if (direction == down) {
    if (_next.lock()) {
      return _next.lock();
    }
  }
  if (_parent.lock()) { // _prev, _next �� ���� ���, �θ𿡼� ã��
    return _parent.lock()->findRender(direction, position);
  }
  return nullptr;
}

RenderObjectPtr RenderListItem::findRender(UITextLayoutDirection direction, SRTextPosition position) {
  if (direction == up) {
    RenderObjectPtr prev = _prev.lock();
    if (prev && prev->_prev.lock()) {
      return prev->_prev.lock(); // ���� ����Ʈ �������� ����
    }
  } else if (direction == down) {
    RenderObjectPtr next = _next.lock();
    if (next && next->_next.lock()) {
      return next->_next.lock(); // ���� ����Ʈ �������� ����
    }
  }
  if (parent()) { // _prev, _next �� ���� ���, �θ𿡼� ã��
    return parent()->findRender(direction, position);
  }
  return nullptr;
}

RenderObjectPtr RenderTableCell::findRender(UITextLayoutDirection direction, SRTextPosition position) {
  RenderTablePtr table = std::dynamic_pointer_cast<RenderTable>(parent());
  if (direction == up) {
    SRIndex findRow = _row - 1;
    if (findRow >= 0) {
      // ���� ���̺� �� ����� ���� ������ �̵�
      const RenderObjectList& cells = table->getChildren();
      auto it = std::find_if(cells.begin(), cells.end(), [&findRow](RenderObjectPtr obj) {
        RenderTableCellPtr c = std::dynamic_pointer_cast<RenderTableCell>(obj);
        return c->_row <= findRow; // ���� �� ù ���� ����
      });
      SR_ASSERT(it != cells.end());
      return *it;
    } else {
      // ���� ���̺� ���� ���� ���, ���̺� ���� ������ ����
      //return table->_prev.lock();
      return table->findRender(direction, position);
    }
  } else if (direction == down) {
    SRIndex findRow = _row + _rowSpan;
    if (findRow < table->_numberOfRows) {
      // ���� ���̺� �� ����� ���� ������ �̵�
      const RenderObjectList& cells = table->getChildren();
      auto it = std::find_if(cells.begin(), cells.end(), [&findRow](RenderObjectPtr obj) {
        RenderTableCellPtr c = std::dynamic_pointer_cast<RenderTableCell>(obj);
        return c->_row >= findRow; // ���� �� ù ���� ����
      });
      SR_ASSERT(it != cells.end());
      return *it;
    } else {
      // ���� ���̺� ���� ���� ���, ���̺� ���� ������ ����
      //return table->_next.lock();
      return table->findRender(direction, position);
    }
  }
  SR_ASSERT(0);
  return nullptr;
}

SRTextPosition RenderRoot::closestPosition(SRPoint point, UITextLayoutDirection direction, SRTextPosition position) {
  SRPoint orgPoint = point;
  if (direction == up) {
    // ���� ������ ã�´�.
    SR_ASSERT(position._offset != SRNotFound);
    auto curRender = getRenderByIndex(position);

    SRRect r = curRender->getRectInRoot();
    if (r.bottom() <= point.y) {
      // �̹� ��ġ�� ���� ������ ���, �׳� ���Ͻ�Ŵ
      return position;
    }
    if (r.top() <= point.y && point.y < r.bottom()) {
      // ���� ���� ���� �̳��� ���
      SRPoint convertedPoint = convertTo(point, curRender); // convert to obj's local coordinate space
      point = curRender->fitToContent(convertedPoint, direction); // skip margin area
      if (point.x < 0 || point.y < 0) {
        SR_ASSERT(0);
      }
      SRTextPosition textPos = curRender->closestPosition(point, direction, position);
      if (textPos._offset != position._offset) {
        // ���� ���������� ���� ��ġ�� ã�� ��� ����
        // ĳ�� ��ġ - 1 ��ġ�� ã�⶧���� ���� ������ ã�� �Ǵ� ��찡 �߻���(e.g, �̹����� �����ϴ� ���)
        return textPos;
      } else {
        //SR_ASSERT(0);
      }
      // textPos�� ������ _stickToNextLine ���� �ٸ� �� �����Ƿ� �������ش�.
      // (�ȱ׷��� ���� �̵� ���ϴ� ��찡 �߻��ϰ� ����)
      position._stickToNextLine = textPos._stickToNextLine;
    }

    // ã������ ��ǥ�� ���� ���� ���� ��ġ�� ���, ���� �������� ã��
    auto aboveRender = curRender->findRender(direction, position);
    if (!aboveRender)
      return position;
    SRPoint aboveBottom = { 0, aboveRender->getRect().height() - 1 }; // y �� �����
    aboveBottom = aboveRender->fitToContent(aboveBottom, direction); // ������ �������� �̵�
    point.y = aboveRender->convertTo(aboveBottom).y; // ���� point�� ���� ��ġ�� ã����
    //return closestPosition(point, none, position);
  } else if (direction == down) {
    // ���� ������ ã�´�.
    SR_ASSERT(position._offset != SRNotFound);
    auto curRender = getRenderByIndex(position);

    SRRect r = curRender->getRectInRoot();
    if (r.top() > point.y) {
      // �̹� ��ġ�� ���� ������ ���, �׳� ���Ͻ�Ŵ
      return position;
    }
    if (r.top() <= point.y && point.y < r.bottom()) {
      // ���� ���� ���� �̳��� ���
      SRPoint convertedPoint = convertTo(point, curRender); // convert to obj's local coordinate space
      point = curRender->fitToContent(convertedPoint, direction); // skip margin area
      if (point.x < 0 || point.y < 0) {
        SR_ASSERT(0);
      }
      SRTextPosition textPos = curRender->closestPosition(point, direction, position);
      if (textPos._offset != position._offset) {
        // ���� ���������� ���� ��ġ�� ã�� ��� ����
        // ĳ�� ��ġ - 1 ��ġ�� ã�⶧���� ���� ������ ã�� �Ǵ� ��찡 �߻���(e.g, �̹����� �����ϴ� ���)
        return textPos;
      } else {
        //SR_ASSERT(0);
      }
      // textPos�� ������ _stickToNextLine ���� �ٸ� �� �����Ƿ� �������ش�.
      // (�ȱ׷��� ���� �̵� ���ϴ� ��찡 �߻��ϰ� ����)
      position._stickToNextLine = textPos._stickToNextLine;

      //// ���� ���� ���� �̳��� ���
      //SRTextPosition tp = closestPosition(point, none, position);
      //if (tp._offset != position._offset) {
      //  // ���� ���������� ���� ��ġ�� ã�� ��� ����
      //  return tp;
      //} else {
      //  // ���̺� �� ������ ���左�Ǵ� ��� �߻�����
      //  //SR_ASSERT(0); // ĳ���� ���� bottom ��ġ�� ��ġ�ϹǷ� �̷� ���� ��������.
      //  int a = 0;
      //}
    }

    // ã������ ��ǥ�� ���� ���� ���� ��ġ�� ���, ���� �������� ã��
    auto belowRender = curRender->findRender(direction, position);
    if (!belowRender)
      return position;
    SRPoint belowTop = { 0, 0 }; // y �� �����
    belowTop = belowRender->fitToContent(belowTop, direction); // ������ �������� �̵�
    point.y = belowRender->convertTo(belowTop).y; // ���� point�� ���� ��ġ�� ã����
    //return closestPosition(point, none, position);
  }

  RenderObjectPtr obj = hitTest(point, direction);
  if (obj) {
    if (obj.get() == this) {
      SR_ASSERT(0);
    }
    SRPoint convertedPoint = convertTo(point, obj); // convert to obj's local coordinate space
    point = obj->fitToContent(convertedPoint, direction); // skip margin area
    if (point.x < 0 || point.y < 0) {
      SR_ASSERT(0);
    }
    SRTextPosition textPos = obj->closestPosition(point);
    // convert to this local coordinate space
    //textPos._xy = obj->convertTo(textPos._xy, this->shared_from_this());
    //textPos._render = obj.get();
    return textPos;
  }
  // if not found caret position, previous caret position used
  //return SRTextPosition();
  return position;
}

SRRect RenderObject::caretRect(SRTextPosition position) const {
  SRIndex index = position._offset;
  SR_ASSERT(index != SRNotFound);
  auto it = std::find_if(_children.begin(), _children.end(), [&index, &position](RenderObjectPtr child) {
    if (position._stickToNextLine)
      return index < child->getRange().rangeMax(); // ���� ���� ĳ���� ���� ������ ���� ��ġ�� ǥ��
    else
      return index <= child->getRange().rangeMax();
  });
  RenderObjectPtr obj = *it; // should be only RenderTextFrame
  return obj->caretRect(position);
}

SRRange RenderObject::lineRange(SRTextPosition position) const {
  SRIndex index = position._offset;
  auto it = std::find_if(_children.begin(), _children.end(), [&index, &position](RenderObjectPtr child) {
    if (position._stickToNextLine)
      return index < child->getRange().rangeMax(); // ���� ���� ĳ���� ���� ������ ���� ��ġ�� ǥ��
    else
      return index <= child->getRange().rangeMax();
  });
  RenderObjectPtr obj = *it;
  return obj->lineRange(position);
}

void RenderObject::setRect(SRRect rect) {
  if (rect.width() < 0 || rect.height() < 0) {
    SR_ASSERT(0);
  }
  _rect = rect;
}

SRRect RenderObject::getRectInRoot() const {
  SRPoint pt;
  RenderObjectPtr v = _parent.lock();
  pt = v ? pt + v->getRect().origin : pt;
  while (v && v->parent()) {
    v = v->parent();
    pt += v->getRect().origin;
  }
  SRRect rect = getRect();
  rect.origin += pt;
  return rect;
}

void RenderObject::setRange(SRRange range) {
  SR_ASSERT(range.location >= 0 && range.length >= 0);
  _astrRange = range;
}

void RenderObject::beginDraw(CGContextPtr ctx) const {
  ctx->saveGState();
  SRRect prevClip = ctx->getClipBox();
  SRRect rcClip = getRect();
  ctx->setClipBox(rcClip); // �ݵ�� �ʿ���!!!
  ctx->translateBy(getRect().origin.x, getRect().origin.y);
}

void RenderObject::endDraw(CGContextPtr ctx) const {
  ctx->restoreGState();
}

void RenderObject::draw(CGContextPtr ctx) const {
  SR_ASSERT(_margin == 0); // _margin > 0 �� ���� �׽�Ʈ �ʿ���
  beginDraw(ctx);
  SRRect rcRender = { { 0,0 }, getRect().size };

#ifdef SR_DEBUG_DRAW
  if (ctx->debugDraw()) {
    // ��輱 �׸���
    ctx->setStrokePattern(SR_DOT);
    ctx->setStrokeColor(SRColor(1, 0, 0));
    ctx->drawRect(rcRender);
    ctx->setStrokePattern(SR_SOLID);
  }
#endif

  SRFloat margin = _margin;
  SRFloat border = _border;

  if (border > 0) {
    rcRender = rcRender.insetBy(margin, margin);
    if (rcRender.width() <= 0 || rcRender.height() <= 0) {
      //SR_ASSERT(0);
      int a = 0;
    }
    //ctx->setStrokeColor(SRColor(0, 0, 0));
    SRFloat oldLineWidth = ctx->getLineWidth();
    ctx->setLineWidth(border);
    // The rectangle that is drawn excludes the bottom and right edges.
    //ctx->drawRect(rcRender); // bottom, right ��輱 �ٷ� �������� �׷���
    ctx->drawRect(SR_PS_INSIDEFRAME, SRColor(0, 0, 0), rcRender);
    ctx->setLineWidth(oldLineWidth); // �������� ������ ���Ŀ� ��� �����
  }

  for (auto it = _children.begin(); it != _children.end(); ++it) {
    RenderObjectPtr render = *it;
    render->draw(ctx);
  }

  endDraw(ctx);
}

void RenderObject::addChild(RenderObjectPtr child) {
  RenderObjectPtr prev;
  if (_children.size() > 0) {
    prev = _children.back();
    prev->_next = child;
    child->_prev = prev;
  }
  _children.push_back(child);
  SR_ASSERT(child->parent().get() == this);
  // �θ� ������ �ڽ��� ũ�⸸ŭ �÷��ش�.
  SRRect thisRect = getRect();
  SRRect childRect = child->getRect();
  childRect = childRect.offsetBy(thisRect.origin);

  thisRect += childRect; // thisRect ũ�⸦ childRect ũ�⸸ŭ �ø�(origin �� ����)
  setRect(thisRect);

  // �θ��� ���ڿ� ������ �÷��ش�.
  SRRange childRange = child->getRange();
  if (childRange.length > 0) {
    // ����Ʈ ����� ���, childRange.length �� 0 ���� ȣ��ǰ� ������, �� ��쿡�� ���ڿ� ������ �������� �ʰ���
    SRRange thisRange = getRange();
    if (thisRange.rangeMax() != childRange.location) {
      //SR_ASSERT(0); // TODO ǥ ���� ������ ���๮�� ������ �����ϰ� ����. Ȯ���ʿ�.
      int a = 0;
    }
    thisRange.length = childRange.rangeMax() - thisRange.location;
    setRange(thisRange);
  }
}

void RenderObject::removeFromParent() {
  if (parent() == nullptr || parent()->getChildren().empty()) {
    return;
  }
  RenderObjectList& children = parent()->getChildren();
  children.erase(std::remove_if(children.begin(), children.end(), [this](RenderObjectPtr child) {
    if (child.get() == this) {
      int a = 0;
    }
    return child.get() == this;
  }), children.end());
}

// SRLayoutManager::layoutText()���� �θ� ���� margin, border, padding�� ����� rect�� ���޵�
RenderTextFrame::RenderTextFrame(RenderObjectPtr parent, SRRect rect)
  : RenderObject(parent, rect) {
}

void RenderTextFrame::setFrame(CTFramePtr frame) {
  _frame = frame;

  // RenderRoot::_pageBreakPos ����� �߰��Ǵ� �����ӳ��� ��� ���� ���̸� �߰��Ѵ�
  // �������� �����ϴ� �������� ���� ���� ��� ���Ǹ� ���� ���� ���̴� _pageBreakPos �� �������� �����Ŵ
  std::vector<std::pair<SRFloat, SRRange>>& pages = root()->_pageBreakPos;
  //SRFloat yPos = getRect().top();
  // getRect()�� ����ϰ� �Ǹ�, ���̺���� ��ġ�� ����� ó������ �ʰ� �ȴ�.
#if 1
  SRFloat top = getRectInRoot().top();
  const CTLineArray& lines = frame->getLines();
  for (auto it = lines.begin(); it != lines.end(); ++it) {
    SRFloat yPos = top + frame->_lineOrigins[it - lines.begin()].y;
    CTLinePtr line = *it;
    yPos += line->getLineSize().height;
    pages.push_back(std::pair<SRFloat, SRRange>(yPos, line->getStringRange()));
  }
#else
  SRFloat yPos = getRectInRoot().top();
  yPos += frame->_frameRect.origin.y; // �׻� 0 �� ���ϵǰ� ����
  for (auto& line : frame->getLines()) {
    const SRSize szLine = line->getLineSize();
    const float lineHeight = szLine.height;
    yPos += lineHeight;
    pages.push_back(std::pair<SRFloat, SRRange>(yPos, line->getStringRange()));
  }
#endif
  // ���� ���̸� ���Ľ�Ű��, �ߺ��Ǵ� ���� ���̴� �����Ѵ�.
  struct comp {
    bool operator()(std::pair<SRFloat, SRRange> l, std::pair<SRFloat, SRRange> r) {
      return (l.first < r.first);
    }
  } compobj;
  std::sort(pages.begin(), pages.end(), compobj);
  pages.erase(std::unique(pages.begin(), pages.end(), [](std::pair<SRFloat, SRRange> l, std::pair<SRFloat, SRRange> r) {
    return l.first == r.first;
  }), pages.end());
}

void RenderTextFrame::draw(CGContextPtr ctx) const {
  RenderObject::draw(ctx);
  beginDraw(ctx);
  _frame->draw(ctx);
  endDraw(ctx);
}

SRTextPosition RenderTextFrame::closestPosition(SRPoint point, UITextLayoutDirection direction, SRTextPosition position) {
  SRTextPosition pos = _frame->closestPosition(point);
  return pos;
}

SRRect RenderTextFrame::caretRect(SRTextPosition position) const {
  SRRect rect = _frame->caretRect(position);
  rect.origin += getRectInRoot().origin; // convert root coordinate
  return rect;
}

SRRange RenderTextFrame::lineRange(SRTextPosition position) const {
  return _frame->lineRange(position);
}

RenderTable::RenderTable(RenderObjectPtr parent, SRInt row, SRInt col, SRRect rect)
  : RenderObject(parent, rect), _numberOfRows(row), _numberOfColumns(col) {
}

void RenderTable::draw(CGContextPtr ctx) const {
  RenderObject::draw(ctx);
}

SRPoint RenderTable::fitToContent(SRPoint point, UITextLayoutDirection direction) const {
  SR_ASSERT(point.x >= 0 && point.y >= 0);
  if (point.x == 0) {
    // ���̺� ���� ������ Ŭ���ÿ��� ���̺� ������ �������� ������(like ms-word)
    return RenderObject::fitToContent(point, direction);
  }
  if (direction == down || direction == up) {
    // ���� ���� ĳ�� �̵���, ���̺� ������ ��ġ�� �̵���Ŵ
    return RenderObject::fitToContent(point, direction);
  }
  // _border, _padding ���� Ŭ���� ĳ���� �������� �ʰ� ��(like ms-word)
  return point;
}

RenderTableCell::RenderTableCell(RenderObjectPtr parent, SRInt row, SRInt rowSpan
  , SRInt column, SRInt columnSpan, SRRect rect)
  : RenderObject(parent, rect), _row(row), _rowSpan(rowSpan)
  , _column(column), _columnSpan(columnSpan) {
}

void RenderTableCell::draw(CGContextPtr ctx) const {
  RenderObject::draw(ctx);
}

RenderList::RenderList(RenderObjectPtr parent, SRRect rect)
  : RenderObject(parent, rect) {
}

SRPoint RenderList::fitToContent(SRPoint point, UITextLayoutDirection direction) const {
  // ����Ʈ ��� ���� Ŭ����, ����Ʈ ������ ��ġ�� ������
  SRPoint orgPoint = point;
  SRRect headerRect = getChildren().front()->getRect();
  point.x = std::max(point.x, headerRect.right());
  return point;
}

RenderListHeader::RenderListHeader(RenderObjectPtr parent, SRRect rect, SRString marker)
  : RenderObject(parent, rect), _marker(marker) {
}

// ���� List �� ��쿡�� ȣ������
void RenderListHeader::draw(CGContextPtr ctx) const {
  RenderObject::draw(ctx);

  beginDraw(ctx);
  SRRect rcRender = getRect();
  SRPoint pt = { 0, 0 }; // rcRender.origin;
  SRColor crText(0, 0, 0); // default color
                           //UIFontPtr font = UIFont::fontWithName(L"Courier New", 14.0); // Arial 12 -> 25CF(Black Circle) �ణ ��׷���
  //UIFontPtr font = UIFont::fontWithName(L"���� ���", 14.0); // Arial 12 -> 25CF(Black Circle) �ణ ��׷���
  UIFontPtr font = UIFont::fontWithName(L"Courier New", 12.0);
  ctx->setFont(font);
  //pt.y += rcRender.height();
  pt.y += font->ascender();

  //pt.x += 269; // @@test
  ctx->showGlyphsAtPoint(pt, crText, (const SRGlyph *)_marker.c_str(), _marker.length());
  endDraw(ctx);
}

RenderListItem::RenderListItem(RenderObjectPtr parent, SRRect rect)
  : RenderObject(parent, rect) {
}

#if 0
void RenderTableCellBlock::draw(CGContextPtr ctx) {
  RenderObject::draw(ctx);
  SRRect rect = ctx->getClipBox();
  CGAffineTransform ctm = ctx->getCTM();
  CGAffineTransform oldCtm = ctm;
  ctm.translatedBy(getRect().origin.x, getRect().origin.y); // ��ũ�ѵ� ������ ��ǥ ó�� _rect ���� ��ġ�� �׷����� ó����
  ctx->setCTM(ctm); // �ʿ��Ѱ�???

  if (ctm.tx != 0 || ctm.ty != 0) {
    //SR_ASSERT(0);
  }

#if 0
  // ũ�� �������� ���, ����Ʈ�� cellpadding="1" cellspacing="2" �� �׷���(border �β� �������)
  SRFloat border = 3;
  SRRect borderRect = getRect().offsetBy(ctm.tx, ctm.ty); // ������ ��ǥ�� ��ȯ

  if (border > 1) {
    SRRect internalRect = borderRect;
    border -= 1;
    internalRect = internalRect.insetBy(border, border);

    // ���� ������������ �缱���� �»��/���ϴ� ��輱�� �׷����� ó����(like chrome)
    // �»�� ��輱
    std::vector<SRPoint> points;
    points.push_back(SRPoint(borderRect.left(), borderRect.bottom()));
    points.push_back(SRPoint(borderRect.left(), borderRect.top()));
    points.push_back(SRPoint(borderRect.right(), borderRect.top()));
    points.push_back(SRPoint(internalRect.right(), internalRect.top()));
    points.push_back(SRPoint(internalRect.left(), internalRect.top()));
    points.push_back(SRPoint(internalRect.left(), internalRect.bottom()));
    ctx->polyFillOutlined(points, crBorderLight, crBorderLight);
    // ���ϴ� ��輱
    points.clear();
    points.push_back(SRPoint(borderRect.right(), borderRect.top()));
    points.push_back(SRPoint(borderRect.right(), borderRect.bottom()));
    points.push_back(SRPoint(borderRect.left(), borderRect.bottom()));
    points.push_back(SRPoint(internalRect.left(), internalRect.bottom()));
    points.push_back(SRPoint(internalRect.right(), internalRect.bottom()));
    points.push_back(SRPoint(internalRect.right(), internalRect.top()));
    ctx->polyFillOutlined(points, crBorderDark, crBorderDark);
  }
  else if (border == 1) {
    // �»�� ��輱
    std::vector<SRPoint> points;
    points.push_back(SRPoint(borderRect.left(), borderRect.bottom()));
    points.push_back(SRPoint(borderRect.left(), borderRect.top()));
    points.push_back(SRPoint(borderRect.right(), borderRect.top()));
    ctx->polyLine(points, crBorderLight);
    // ���ϴ� ��輱
    points.clear();
    points.push_back(SRPoint(borderRect.right(), borderRect.top()));
    points.push_back(SRPoint(borderRect.right(), borderRect.bottom()));
    points.push_back(SRPoint(borderRect.left(), borderRect.bottom()));
    ctx->polyLine(points, crBorderDark);
  }
#endif

  //_frame->draw(ctx);

  ctx->setCTM(oldCtm);
}
#endif

} // namespace sr
