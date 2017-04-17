#include "SRDocumentView.h"
#include "../UIKit/UIWindow.h"
#include "../Win32/WIN32GraphicContextImpl.h"
#undef max // 윈도우 헤더파일 어디에선가 정의되고 있어서 std::max() 호출부에서 컴파일 에러 발생중임
#undef min
#include <algorithm>

namespace sr {

static const std::vector<std::pair<SRFloat, SRRange>>& _lineList(const SRDocumentView* docView) {
  //SRPageViewPtr pageView = std::dynamic_pointer_cast<SRPageView>(docView->getSubviews().at(0));
  //SRColumnViewPtr columnView = std::dynamic_pointer_cast<SRColumnView>(pageView->getSubviews().at(0));
  SRColumnViewPtr columnView = docView->firstColumnView();
  SRTextContainerPtr tv = columnView->textContainer();
  return tv->rootRender()->_pageBreakPos;
}

SRColumnViewPtr SRDocumentView::firstColumnView() const {
  SRPageViewPtr pageView = std::dynamic_pointer_cast<SRPageView>(getSubviews().at(0));
  SRColumnViewPtr columnView = std::dynamic_pointer_cast<SRColumnView>(pageView->getSubviews().at(0));
  return columnView;
}

SRDocumentViewPtr SRColumnView::getDocumentView() const {
  SRPageViewPtr pageView = std::dynamic_pointer_cast<SRPageView>(superview());
  SRDocumentViewPtr docView = std::dynamic_pointer_cast<SRDocumentView>(pageView->superview());
  return docView;
}

void SRDocumentView::paginate(const SRSize& pageSize, const SRSize& totalSize) {
  _pageRects.clear();
  _totalPageSize = totalSize;
  const std::vector<std::pair<SRFloat, SRRange>>& lines = _lineList(this);
  SRFloat prevY = 0;
  SRFloat curY = 0;
  SRRect pageRect;
  pageRect.size.width = pageSize.width;
  SRFloat& nextPageStart = pageRect.origin.y;
  SRFloat nextPageEnd = pageSize.height;
  for (int i = 0; i < lines.size(); ++i) {
    auto lineInfo = lines.at(i);
    curY = lineInfo.first;
    if ((i > 0 && (curY) > nextPageEnd)/* || (i == lines.size() - 1)*/) {
      if (i == lines.size() - 1) {
        int a = 0;
      }
      // 현재 페이지 하단을 넘어가는 경우, 페이지를 추가함
      pageRect.size.height = prevY - nextPageStart;
      if (pageRect.size.height > pageSize.height) {
        //SR_ASSERT(0);
        int a = 0;
      }
      _pageRects.push_back(pageRect);
      // 다음 페이지 정보 설정(시작, 끝 위치)
      nextPageStart = prevY;
      nextPageEnd = (prevY + pageSize.height);
    }
    prevY = curY; // 다음 페이지가 시작할 위치를 저장함
  }

  if (curY > nextPageStart) {
    // last page
    //rect.size.height = curY - rect.origin.y;
    pageRect.size.height = totalSize.height - nextPageStart; // 남은 높이를 전부 할당한다.
    if (pageRect.size.height > pageSize.height) {
      // 남은 높이가 페이지 크기를 넘는 경우도 발생하고 있다.
      // (마지막 라인이 페이지 크기보다 작고, 마지막 라인 이후에도 컨텐츠가 있을 경우임)
      // 일단 이 경우에는 그냥 한 페이지에 그려지게 함(약간의 오차 발생하게 됨)
      //SR_ASSERT(0); // @@ 발생되고 있음. 확인필요!!!
    }
    _pageRects.push_back(pageRect);
  }
}

// declared for debug
void SRDocumentView::draw(CGContextPtr ctx, SRRect rect) const {
  // 자식뷰들 그리기
  SR_PROF_TIME_START(L"SRDocumentView::draw");
  UIView::draw(ctx, rect);
  SR_PROF_TIME_END(L"SRDocumentView::draw");
}

const UIViewPtr SRDocumentView::hitTest(UIEvent event) const {
  // UIWindow::hitTest()에서 바로 호출되고 있음
  if (event.isMouseEvent()) {
    // 마우스 이벤트의 경우, SRDocumentView 영역 이외를 클릭해도 SRDocumentView 이내에 캐럿을 설정함
    SRPoint& point = event._xy;
    SRPoint orgPoint = point;

    // 현재 문서뷰 이내로 좌표를 보정시킴
    SRRect rect({ 0, 0 }, getBounds().size); // local area
    point.x = std::max(point.x, rect.left());
    point.x = std::min(point.x, rect.right() - 1);
    point.y = std::max(point.y, rect.top());
    point.y = std::min(point.y, rect.bottom() - 1);
    if (orgPoint != point) {
      //SR_ASSERT(0);
      int a = 0;
    }

    // 클릭된 위치보다 뒤에 있는 다음 페이지 뷰를 찾는다.
    auto it = std::find_if(_subviews.begin(), _subviews.end(), [&point](UIViewPtr v) {
      return point.y < v->getFrame().top();
    });
    if (it == _subviews.begin()) {
      // 첫 페이지 이전 위치를 클릭한 경우, 첫 페이지 top 위치로 설정
      point.y = _subviews.front()->getFrame().top();
    } else {
      UIViewPtr prevView = *(--it);
      if (point.y >= prevView->getFrame().bottom()) {
        // 페이지 사이를 클릭시, 이전 페이지 bottom - 1 위치로 설정(like ms-word)
        point.y = prevView->getFrame().bottom() - 1;
      }
    }
  }

  return UIView::hitTest(event);
}

UITextPosition SRDocumentView::position(UITextRange range, UITextLayoutDirection farthest) const {
  SRColumnViewPtr firstColView = firstColumnView();
  UITextPosition result = firstColView->position(range, farthest);
  return result;
}

// 캐럿 위치로부터 렌더루트내의 캐럿 좌표를 획득 후, 윈도우 좌표로 리턴
SRRect SRDocumentView::caretRect(UITextPosition position) const {
  SRColumnViewPtr firstColView = firstColumnView();

  // 렌더루트내의 caretRect 위치 획득(첫번째 컬럼뷰를 사용함)
  SRRect caretRect = firstColView->caretRect(position);

  // 렌더루트내의 caretRect 위치를 윈도우 좌표로 변환하기 위해 캐럿이 위치할 컬럼뷰를 조회
  const auto it = std::find_if(_pageRects.begin(), _pageRects.end(), [&caretRect](SRRect item) {
    // 캐럿이 현재 페이지(컬럼) 영역 이내에 존재하는지 체크
    return caretRect.top() < item.bottom();
  });

  // 렌더루트 좌표를 윈도우 좌표로 변환하기 위해, 찾은 컬럼뷰 로컬 좌표를 구한다.
  SRRect targetColRect = *it;
  caretRect.origin -= targetColRect.origin; // 렌더루트 좌표 -> 현재 컬럼뷰 로컬 좌표

  if (caretRect.origin.y < 0) {
    // 이전 페이지(컬럼)에 캐럿이 위치해야 하는 경우, 캐럿 위치를 바꾸지 않는다(like ms-word)
    // 하나의 테이블 컬럼이 여러 페이지에 걸쳐 존재하는 경우, 클릭시 캐럿이 이전 페이지에 위치해야 하는 경우가 존재함
    return caretRect;
  }

  // 현재 컬럼뷰 로컬 좌표 -> 윈도우 좌표
  int colIndex = it - _pageRects.begin(); // 캐럿이 위치할 컬럼뷰 인덱스

  // 캐럿이 위치할 페이지뷰 검색(페이지내의 컬럼뷰 개수로 계산함)
  const auto pageViews = getSubviews();
  auto pageIter = std::find_if(pageViews.begin(), pageViews.end(), [&colIndex](UIViewPtr pageView) {
    if (colIndex < pageView->getSubviews().size()) {
      return true;
    }
    colIndex -= pageView->getSubviews().size();
    return false;
  });
  const auto pageView = std::dynamic_pointer_cast<SRPageView>(*pageIter);
  const auto columnView = std::dynamic_pointer_cast<SRColumnView>(pageView->getSubviews().at(colIndex));
  SRPoint orgPoint = caretRect.origin;
  caretRect.origin = columnView->convertTo(caretRect.origin); // 윈도우 좌표를 리턴
  if (orgPoint != caretRect.origin) {
    int a = 0;
  }
  return caretRect;
}

void SRDocumentView::replace(UITextRange range, const SRString& text) const {
  SRColumnViewPtr firstColView = firstColumnView();
  firstColView->replace(range, text);
}

// declared for debug
void SRPageView::draw(CGContextPtr ctx, SRRect rect) const {
  // 자식뷰들 그리기
  UIView::draw(ctx, rect);
}

SRRect SRPageView::getContentsRect() const {
  return _bounds;
}

const UIViewPtr SRPageView::hitTest(UIEvent event) const {
  SR_ASSERT(_subviews.size() > 0);
  SR_ASSERT(getBounds().origin.x == 0 && getBounds().origin.y == 0);
  SRPoint point = event.mouseLocation();

  if (pointInside(point)) {
    // 컬럼 사이 클릭시 이전 컬럼 위치로 설정함
    auto it = std::find_if(_subviews.begin(), _subviews.end(), [&point](UIViewPtr v) {
      return point.x < v->getFrame().left();
    });
    if (it == _subviews.begin()) {
      // 첫 컬럼 이전 위치를 클릭한 경우, 첫 컬럼 left 위치로 설정
      if (event.isMouseEvent()) {
        event._xy.x = _subviews.front()->getFrame().left();
      }
    } else {
      UIViewPtr prevView = *(--it);
      if (point.x >= prevView->getFrame().right()) {
        // 컬럼 사이를 클릭시, 이전 컬럼 right - 1 위치로 설정(righ, bottom is exclusive!)
        if (event.isMouseEvent()) {
          event._xy.x = prevView->getFrame().right() - 1;
        }
      }
    }
  } else {
    // 현재 페이지 범위가 아닌 경우(mouseMove()에서 호출될 경우, 진입하고 있음)
    //SR_ASSERT(0);
    return nullptr;
  }
  return UIView::hitTest(event);
}

void SRColumnView::draw(CGContextPtr ctx, SRRect rect) const {
  beginDraw(ctx);

  // 현재 뷰의 스크롤 적용
  SR_ASSERT(getBounds().origin.x == 0 && getBounds().origin.y == 0); // 현재는 UIScrollView의 경우에만 _bounds.origin을 설정함
  ctx->translateBy(-getBounds().origin.x, -getBounds().origin.y);

  // 텍스트뷰가 포함한 루트 렌더를 텍스트뷰의 frame 위치에 드로잉한다.
  static CGContextPtr ctxRender = nullptr;
  CGAffineTransform ctm = ctx->getTextMatrix();
//#define __forceFullRedrawForSelection
#ifndef __forceFullRedrawForSelection
  if (_partialRect.top() == 0) {
#endif
    // 첫 페이지일 경우 전체 렌더 영역을 드로잉 후, ctxRender 에 저장함
    SRRect rcSrc({ 0, 0 }, getDocumentView()->getTotalPageSize()); // 루트 렌더 전체 영역임
    SRRect rcDest({ ctm.tx, ctm.ty }, _partialRect.size);
    void* destDC = ctx->getSrcDC(); // ctxRender는 ctx(윈도우 dc)의 _srcDC(메모리 dc)에 드로잉하게 설정
    ctxRender = CGContext::create(WIN32GraphicContextImpl::create(nullptr, rcSrc, rcDest, destDC));
    ctxRender->_selectedRange = ctx->_selectedRange;
    ctxRender->setDebugDraw(ctx->debugDraw());

    // draw background, border
    UIView::drawBackground(ctxRender, rcSrc);
    UIView::drawBorder(ctxRender, rcSrc);
    textContainer()->rootRender()->draw(ctxRender);
    //textContainer()->rootRender()->beginDraw(ctxRender);
    textContainer()->drawExclusionPath(ctxRender);
    //textContainer()->rootRender()->endDraw(ctxRender);
    ctxRender->flush(); // ctx(윈도우 dc)의 _srcDC(메모리 dc)에 드로잉 수행
#ifndef __forceFullRedrawForSelection
  } else {
#else
  if (_partialRect.top() != 0) {
#endif
    // 두 번째 페이지부터는 첫 페이지 드로잉시 저장했던 ctxRender 에서 페이지 영역만 발췌해서 드로잉
    SRRect rcSrc = _partialRect; // 현재 컬럼이 드로잉할 페이지 영역
    SRRect rcDest({ ctm.tx, ctm.ty }, _partialRect.size);
    void* destDC = ctx->getSrcDC(); // ctxRender는 ctx(윈도우 dc)의 _srcDC(메모리 dc)에 드로잉하게 설정
    ctxRender->flush(destDC, &rcDest, &rcSrc.origin);
  }
  endDraw(ctx);
  //return true;
}

SRRect SRColumnView::caretRect(UITextPosition position) const {
  // 렌더루트 내에서의 캐럿 좌표를 리턴시키고, SRDocumentView에서 윈도우 좌표로 변환해서 처리하게함
  SRRect caretRect = textContainer()->rootRender()->caretRect(position);
  return caretRect;
}

UITextPosition SRColumnView::position(UITextRange range, UITextLayoutDirection farthest) const {
  UITextPosition position = range._range[0];
  SRRange lineRange = textContainer()->rootRender()->lineRange(position);
  UITextPosition result;
  switch (farthest) {
  case left:
    result._offset = lineRange.location;
    break;
  case right:
  {
    std::wstring str = textContainer()->layoutManager()->textStorage()->getString()->data();
    result._offset = lineRange.rangeMax();
    if (result._offset > 0 && str.at(result._offset - 1) == '\n') {
      //result._stickToNextLine = false; // 현재 라인의 끝에 캐럿이 표시되게 함
      result._offset -= 1;
    } else {
      result._stickToNextLine = false; // 현재 라인의 끝에 캐럿이 표시되게 함
    }
  }
  break;
  default:
    SR_ASSERT(0);
  }
  return result;
}

UITextPosition SRColumnView::closestPosition(SRPoint point, UITextLayoutDirection direction, SRTextPosition position) const {
  // 현재 뷰 좌표 -> 렌더루트 좌표
  point += _partialRect.origin;
  SRPoint orgPoint = point;

  // 현재 뷰의 _partialRect 이내로 좌표를 보정시킨다(페이지 사이 클릭시 처리를 위함)
  point.x = std::max(point.x, _partialRect.left());
  point.x = std::min(point.x, _partialRect.right() - 1);
  point.y = std::max(point.y, _partialRect.top());
  point.y = std::min(point.y, _partialRect.bottom() - 1);
  if (orgPoint != point) {
    int a = 0;
  }

  return UITextView::closestPosition(point, direction, position);
}

void SRColumnView::mouseDown(UIEvent event) {
  SRPoint point = event.mouseLocation();
  //SR_LOGD(L"SRColumnView(%p)::mouseDown() point=[%.1f, %.1f] scroll=[%.1f, %.1f]"
  //  , this, point.x, point.y, window()->contentOffset().x, window()->contentOffset().y);

  SRTextPosition textPos = closestPosition(point);
  if (textPos._offset == SRNotFound) {
    //SR_LOGD(L"return invalid caret position!");
    return;
  }
  SRRect rcCaret = caretRect(textPos); // 렌더루트 내에서의 캐럿 좌표 리턴
  if (rcCaret.origin.y < _partialRect.top()) {
    // 이전 페이지에 캐럿이 위치해야 하는 경우, 캐럿 위치를 바꾸지 않는다(like ms-word)
    // 하나의 테이블 컬럼이 여러 페이지에 걸쳐 존재하는 경우, 클릭시 캐럿이 이전 페이지에 위치해야 하는 경우가 존재함
    return;
  }

  window()->hideCaret();
  if (event.isShiftPressed()) {
    // 선택 영역 저장
    window()->setTextPositions(nullptr, &textPos);
    window()->invalidateRect(NULL, false);
  } else {
    // 캐럿 표시
    if (window()->hasSelection()) {
      // 선택 영역이 있었으면 선택 영역을 지우기 위해 다시 드로잉함
      window()->invalidateRect(NULL, false);
    }
    window()->setTextPositions(&textPos, &textPos);
    window()->updateCaret();
    window()->showCaret();
  }
}

void SRColumnView::mouseUp(UIEvent event) {
}

void SRColumnView::mouseMoved(UIEvent event) {
  SRPoint point = event.mouseLocation();
  //SR_LOGD(L"SRColumnView(%p)::mouseMoved() point=[%.1f, %.1f] scroll=[%.1f, %.1f]"
  //  , this, point.x, point.y, window()->contentOffset().x, window()->contentOffset().y);

  SRTextPosition textPos = closestPosition(point);
  if (textPos._offset == SRNotFound) {
    SR_LOGD(L"return invalid caret position!");
    return;
  }

  if (event.pressedMouseButtons() == LEFT_MOUSE_BUTTON) {
    const UITextPosition* prePos = window()->textPositions();
    if (prePos[1]._offset != textPos._offset) {
      // 선택 영역이 발생한 경우, 캐럿을 숨기고 선택 영역 표시를 위해 화면을 갱신시킨다.
      window()->hideCaret();
      window()->setTextPositions(nullptr, &textPos);
      window()->invalidateRect(NULL, false);
    }
  }
}

void SRColumnView::cursorUpdate(UIEvent event) {
  //SR_LOGD(L"[cursor] SRColumnView::cursorUpdate() IDC_IBEAM");
  ::SetCursor(::LoadCursor(NULL, IDC_IBEAM));
}

} // namespace sr
