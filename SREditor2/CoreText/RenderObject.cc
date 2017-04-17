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
  // 현재 렌더의 컨텐츠 영역 밖을 클릭시 컨텐츠 영역 이내로 제한시킴
  SR_ASSERT(_margin == 0); // _margin > 0 인 경우는 테스트 필요함
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
    // 자식 렌더들 범위를 벗어난 경우, 자식 렌더들 이내로 제한시킴(상하 좌표만)
    // e.g, 테이블 셀 하단에 여백이 존재하고 여기를 클릭시 셀 영역에 캐럿을 설정함
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
    // 현재 렌더 이내가 클릭된 경우
    SRPoint orgPoint = point;
    point = fitToContent(point, direction); // 컨텐츠 영역 이내로 제한
    if (_children.size() == 0) {
      // 자식 렌더가 없는 경우, 현재 렌더를 리턴
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
    // 자식 렌더가 클릭되지 않은 경우, nullptr 을 리턴시킴(border, padding 영역이 클릭된 경우임)
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
      // 현재 렌더를 구할 때는 _stickToNextLine를 true로 간주해야 이전 렌더를 찾지 않게 된다.
      // TODO position 위치가 라인 끝 위치일 경우는 처리가 안 될듯함(확인 필요)
      // |a$|b -> 2nd line b(1-1) 앞에 offset(1) 캐럿 -> 1 >= 1 && 1 < 2
      if (offset >= range.location && offset < range.rangeMax())
        return true;
    } else {
      // |a|$b -> 1st line a(0-1) 뒤에 offset(1) 캐럿 -> 1 > 0 && 1 <= 1
      if (offset > range.location && offset <= range.rangeMax())
        return true;
    }
    if (offset == 0 && range.location >= 0) {
      SR_ASSERT(0); // 이런 경우가 존재하나???
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
  if (_parent.lock()) { // _prev, _next 가 없는 경우, 부모에서 찾음
    return _parent.lock()->findRender(direction, position);
  }
  return nullptr;
}

RenderObjectPtr RenderListItem::findRender(UITextLayoutDirection direction, SRTextPosition position) {
  if (direction == up) {
    RenderObjectPtr prev = _prev.lock();
    if (prev && prev->_prev.lock()) {
      return prev->_prev.lock(); // 이전 리스트 아이템을 리턴
    }
  } else if (direction == down) {
    RenderObjectPtr next = _next.lock();
    if (next && next->_next.lock()) {
      return next->_next.lock(); // 이후 리스트 아이템을 리턴
    }
  }
  if (parent()) { // _prev, _next 가 없는 경우, 부모에서 찾음
    return parent()->findRender(direction, position);
  }
  return nullptr;
}

RenderObjectPtr RenderTableCell::findRender(UITextLayoutDirection direction, SRTextPosition position) {
  RenderTablePtr table = std::dynamic_pointer_cast<RenderTable>(parent());
  if (direction == up) {
    SRIndex findRow = _row - 1;
    if (findRow >= 0) {
      // 이전 테이블 행 존재시 이전 행으로 이동
      const RenderObjectList& cells = table->getChildren();
      auto it = std::find_if(cells.begin(), cells.end(), [&findRow](RenderObjectPtr obj) {
        RenderTableCellPtr c = std::dynamic_pointer_cast<RenderTableCell>(obj);
        return c->_row <= findRow; // 이전 행 첫 셀을 리턴
      });
      SR_ASSERT(it != cells.end());
      return *it;
    } else {
      // 이전 테이블 행이 없을 경우, 테이블 이전 렌더를 리턴
      //return table->_prev.lock();
      return table->findRender(direction, position);
    }
  } else if (direction == down) {
    SRIndex findRow = _row + _rowSpan;
    if (findRow < table->_numberOfRows) {
      // 다음 테이블 행 존재시 다음 행으로 이동
      const RenderObjectList& cells = table->getChildren();
      auto it = std::find_if(cells.begin(), cells.end(), [&findRow](RenderObjectPtr obj) {
        RenderTableCellPtr c = std::dynamic_pointer_cast<RenderTableCell>(obj);
        return c->_row >= findRow; // 이후 행 첫 셀을 리턴
      });
      SR_ASSERT(it != cells.end());
      return *it;
    } else {
      // 다음 테이블 행이 없을 경우, 테이블 다음 렌더를 리턴
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
    // 현재 라인을 찾는다.
    SR_ASSERT(position._offset != SRNotFound);
    auto curRender = getRenderByIndex(position);

    SRRect r = curRender->getRectInRoot();
    if (r.bottom() <= point.y) {
      // 이미 위치상 이전 렌더인 경우, 그냥 리턴시킴
      return position;
    }
    if (r.top() <= point.y && point.y < r.bottom()) {
      // 현재 렌더 높이 이내인 경우
      SRPoint convertedPoint = convertTo(point, curRender); // convert to obj's local coordinate space
      point = curRender->fitToContent(convertedPoint, direction); // skip margin area
      if (point.x < 0 || point.y < 0) {
        SR_ASSERT(0);
      }
      SRTextPosition textPos = curRender->closestPosition(point, direction, position);
      if (textPos._offset != position._offset) {
        // 현재 렌더내에서 이전 위치를 찾은 경우 리턴
        // 캐럿 위치 - 1 위치로 찾기때문에 현재 렌더를 찾게 되는 경우가 발생함(e.g, 이미지가 존재하는 경우)
        return textPos;
      } else {
        //SR_ASSERT(0);
      }
      // textPos에 구해진 _stickToNextLine 값이 다를 수 있으므로 복사해준다.
      // (안그러면 위로 이동 못하는 경우가 발생하고 있음)
      position._stickToNextLine = textPos._stickToNextLine;
    }

    // 찾으려는 좌표가 현재 렌더 이전 위치인 경우, 이전 렌더에서 찾음
    auto aboveRender = curRender->findRender(direction, position);
    if (!aboveRender)
      return position;
    SRPoint aboveBottom = { 0, aboveRender->getRect().height() - 1 }; // y 만 사용함
    aboveBottom = aboveRender->fitToContent(aboveBottom, direction); // 컨텐츠 영역으로 이동
    point.y = aboveRender->convertTo(aboveBottom).y; // 이후 point로 다음 위치를 찾게함
    //return closestPosition(point, none, position);
  } else if (direction == down) {
    // 현재 라인을 찾는다.
    SR_ASSERT(position._offset != SRNotFound);
    auto curRender = getRenderByIndex(position);

    SRRect r = curRender->getRectInRoot();
    if (r.top() > point.y) {
      // 이미 위치상 이후 렌더인 경우, 그냥 리턴시킴
      return position;
    }
    if (r.top() <= point.y && point.y < r.bottom()) {
      // 현재 렌더 높이 이내인 경우
      SRPoint convertedPoint = convertTo(point, curRender); // convert to obj's local coordinate space
      point = curRender->fitToContent(convertedPoint, direction); // skip margin area
      if (point.x < 0 || point.y < 0) {
        SR_ASSERT(0);
      }
      SRTextPosition textPos = curRender->closestPosition(point, direction, position);
      if (textPos._offset != position._offset) {
        // 현재 렌더내에서 이전 위치를 찾은 경우 리턴
        // 캐럿 위치 - 1 위치로 찾기때문에 현재 렌더를 찾게 되는 경우가 발생함(e.g, 이미지가 존재하는 경우)
        return textPos;
      } else {
        //SR_ASSERT(0);
      }
      // textPos에 구해진 _stickToNextLine 값이 다를 수 있으므로 복사해준다.
      // (안그러면 위로 이동 못하는 경우가 발생하고 있음)
      position._stickToNextLine = textPos._stickToNextLine;

      //// 현재 렌더 높이 이내인 경우
      //SRTextPosition tp = closestPosition(point, none, position);
      //if (tp._offset != position._offset) {
      //  // 현재 렌더내에서 이후 위치를 찾은 경우 리턴
      //  return tp;
      //} else {
      //  // 테이블 셀 내에서 워드랩되는 경우 발생중임
      //  //SR_ASSERT(0); // 캐럿은 라인 bottom 위치에 위치하므로 이런 경우는 없을듯함.
      //  int a = 0;
      //}
    }

    // 찾으려는 좌표가 현재 렌더 이후 위치인 경우, 이후 렌더에서 찾음
    auto belowRender = curRender->findRender(direction, position);
    if (!belowRender)
      return position;
    SRPoint belowTop = { 0, 0 }; // y 만 사용함
    belowTop = belowRender->fitToContent(belowTop, direction); // 컨텐츠 영역으로 이동
    point.y = belowRender->convertTo(belowTop).y; // 이후 point로 다음 위치를 찾게함
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
      return index < child->getRange().rangeMax(); // 라인 끝의 캐럿을 다음 라인의 시작 위치에 표시
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
      return index < child->getRange().rangeMax(); // 라인 끝의 캐럿을 다음 라인의 시작 위치에 표시
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
  ctx->setClipBox(rcClip); // 반드시 필요함!!!
  ctx->translateBy(getRect().origin.x, getRect().origin.y);
}

void RenderObject::endDraw(CGContextPtr ctx) const {
  ctx->restoreGState();
}

void RenderObject::draw(CGContextPtr ctx) const {
  SR_ASSERT(_margin == 0); // _margin > 0 인 경우는 테스트 필요함
  beginDraw(ctx);
  SRRect rcRender = { { 0,0 }, getRect().size };

#ifdef SR_DEBUG_DRAW
  if (ctx->debugDraw()) {
    // 경계선 그리기
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
    //ctx->drawRect(rcRender); // bottom, right 경계선 바로 이전까지 그려짐
    ctx->drawRect(SR_PS_INSIDEFRAME, SRColor(0, 0, 0), rcRender);
    ctx->setLineWidth(oldLineWidth); // 원복하지 않으면 이후에 계속 적용됨
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
  // 부모 영역을 자식의 크기만큼 늘려준다.
  SRRect thisRect = getRect();
  SRRect childRect = child->getRect();
  childRect = childRect.offsetBy(thisRect.origin);

  thisRect += childRect; // thisRect 크기를 childRect 크기만큼 늘림(origin 은 동일)
  setRect(thisRect);

  // 부모의 문자열 범위를 늘려준다.
  SRRange childRange = child->getRange();
  if (childRange.length > 0) {
    // 리스트 헤더의 경우, childRange.length 이 0 으로 호출되고 있으며, 이 경우에는 문자열 범위를 수정하지 않게함
    SRRange thisRange = getRange();
    if (thisRange.rangeMax() != childRange.location) {
      //SR_ASSERT(0); // TODO 표 셀의 마지막 개행문자 삭제시 진입하고 있음. 확인필요.
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

// SRLayoutManager::layoutText()에서 부모 블럭의 margin, border, padding가 적용된 rect가 전달됨
RenderTextFrame::RenderTextFrame(RenderObjectPtr parent, SRRect rect)
  : RenderObject(parent, rect) {
}

void RenderTextFrame::setFrame(CTFramePtr frame) {
  _frame = frame;

  // RenderRoot::_pageBreakPos 멤버에 추가되는 프레임내의 모든 라인 높이를 추가한다
  // 수평으로 존재하는 프레임의 라인 높이 계산 편의를 위해 라인 높이는 _pageBreakPos 에 누적시켜 저장시킴
  std::vector<std::pair<SRFloat, SRRange>>& pages = root()->_pageBreakPos;
  //SRFloat yPos = getRect().top();
  // getRect()를 사용하게 되면, 테이블등의 위치가 제대로 처리되지 않게 된다.
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
  yPos += frame->_frameRect.origin.y; // 항상 0 이 리턴되고 있음
  for (auto& line : frame->getLines()) {
    const SRSize szLine = line->getLineSize();
    const float lineHeight = szLine.height;
    yPos += lineHeight;
    pages.push_back(std::pair<SRFloat, SRRange>(yPos, line->getStringRange()));
  }
#endif
  // 라인 높이를 정렬시키고, 중복되는 라인 높이는 제거한다.
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
    // 테이블 왼쪽 영역을 클릭시에는 테이블 컨텐츠 영역으로 조정함(like ms-word)
    return RenderObject::fitToContent(point, direction);
  }
  if (direction == down || direction == up) {
    // 상하 라인 캐럿 이동시, 테이블 컨텐츠 위치로 이동시킴
    return RenderObject::fitToContent(point, direction);
  }
  // _border, _padding 영역 클릭시 캐럿을 변경하지 않게 함(like ms-word)
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
  // 리스트 헤더 영역 클릭시, 리스트 아이템 위치로 설정함
  SRPoint orgPoint = point;
  SRRect headerRect = getChildren().front()->getRect();
  point.x = std::max(point.x, headerRect.right());
  return point;
}

RenderListHeader::RenderListHeader(RenderObjectPtr parent, SRRect rect, SRString marker)
  : RenderObject(parent, rect), _marker(marker) {
}

// 현재 List 일 경우에만 호출중임
void RenderListHeader::draw(CGContextPtr ctx) const {
  RenderObject::draw(ctx);

  beginDraw(ctx);
  SRRect rcRender = getRect();
  SRPoint pt = { 0, 0 }; // rcRender.origin;
  SRColor crText(0, 0, 0); // default color
                           //UIFontPtr font = UIFont::fontWithName(L"Courier New", 14.0); // Arial 12 -> 25CF(Black Circle) 약간 찌그러짐
  //UIFontPtr font = UIFont::fontWithName(L"맑은 고딕", 14.0); // Arial 12 -> 25CF(Black Circle) 약간 찌그러짐
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
  ctm.translatedBy(getRect().origin.x, getRect().origin.y); // 스크롤된 윈도우 좌표 처리 _rect 시작 위치에 그려지게 처리함
  ctx->setCTM(ctm); // 필요한가???

  if (ctm.tx != 0 || ctm.ty != 0) {
    //SR_ASSERT(0);
  }

#if 0
  // 크롬 브라우저의 경우, 디폴트로 cellpadding="1" cellspacing="2" 로 그려짐(border 두께 상관없음)
  SRFloat border = 3;
  SRRect borderRect = getRect().offsetBy(ctm.tx, ctm.ty); // 윈도우 좌표로 변환

  if (border > 1) {
    SRRect internalRect = borderRect;
    border -= 1;
    internalRect = internalRect.insetBy(border, border);

    // 직각 교차점에서는 사선으로 좌상단/우하단 경계선이 그려지게 처리함(like chrome)
    // 좌상단 경계선
    std::vector<SRPoint> points;
    points.push_back(SRPoint(borderRect.left(), borderRect.bottom()));
    points.push_back(SRPoint(borderRect.left(), borderRect.top()));
    points.push_back(SRPoint(borderRect.right(), borderRect.top()));
    points.push_back(SRPoint(internalRect.right(), internalRect.top()));
    points.push_back(SRPoint(internalRect.left(), internalRect.top()));
    points.push_back(SRPoint(internalRect.left(), internalRect.bottom()));
    ctx->polyFillOutlined(points, crBorderLight, crBorderLight);
    // 우하단 경계선
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
    // 좌상단 경계선
    std::vector<SRPoint> points;
    points.push_back(SRPoint(borderRect.left(), borderRect.bottom()));
    points.push_back(SRPoint(borderRect.left(), borderRect.top()));
    points.push_back(SRPoint(borderRect.right(), borderRect.top()));
    ctx->polyLine(points, crBorderLight);
    // 우하단 경계선
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
